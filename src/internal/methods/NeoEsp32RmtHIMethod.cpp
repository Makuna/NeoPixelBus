/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp32.

A BIG thanks to Andreas Merkle for the investigation and implementation of
a workaround to the GCC bug that drops method attributes from template methods

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by donating (see https://github.com/Makuna/NeoPixelBus)

-------------------------------------------------------------------------
This file is part of the Makuna/NeoPixelBus library.

NeoPixelBus is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

NeoPixelBus is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32) && defined(__XTENSA__)

#include "../NeoSettings.h"
#include "../NeoBusChannel.h"
#include "NeoEsp32RmtHIMethod.h"
#include "soc/soc.h"
#include "soc/rmt_reg.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
#include "hal/rmt_ll.h"
#else
/* Shims for older core; we can safely assume original ESP32 */
#include "soc/rmt_struct.h"

__attribute__((always_inline))
static inline void rmt_ll_tx_reset_pointer(rmt_dev_t *dev, uint32_t channel)
{
    dev->conf_ch[channel].conf1.mem_rd_rst = 1;
    dev->conf_ch[channel].conf1.mem_rd_rst = 0;
}

__attribute__((always_inline))
static inline void rmt_ll_tx_start(rmt_dev_t *dev, uint32_t channel)
{
    dev->conf_ch[channel].conf1.tx_start = 1;
}

__attribute__((always_inline))
static inline void rmt_ll_tx_stop(rmt_dev_t *dev, uint32_t channel)
{
    RMTMEM.chan[channel].data32[0].val = 0;
    dev->conf_ch[channel].conf1.tx_start = 0;
    dev->conf_ch[channel].conf1.mem_rd_rst = 1;
    dev->conf_ch[channel].conf1.mem_rd_rst = 0;
}

__attribute__((always_inline))
static inline void rmt_ll_tx_enable_pingpong(rmt_dev_t *dev, uint32_t channel, bool enable)
{
    dev->apb_conf.mem_tx_wrap_en = enable;
}

__attribute__((always_inline))
static inline void rmt_ll_tx_enable_loop(rmt_dev_t *dev, uint32_t channel, bool enable)
{
    dev->conf_ch[channel].conf1.tx_conti_mode = enable;
}

__attribute__((always_inline))
static inline uint32_t rmt_ll_tx_get_channel_status(rmt_dev_t *dev, uint32_t channel)
{
    return dev->status_ch[channel];
}

__attribute__((always_inline))
static inline void rmt_ll_tx_set_limit(rmt_dev_t *dev, uint32_t channel, uint32_t limit)
{
    dev->tx_lim_ch[channel].limit = limit;
}

__attribute__((always_inline))
static inline void rmt_ll_enable_interrupt(rmt_dev_t *dev, uint32_t mask, bool enable)
{
    if (enable) {
        dev->int_ena.val |= mask;
    } else {
        dev->int_ena.val &= ~mask;
    }
}

__attribute__((always_inline))
static inline void rmt_ll_enable_tx_end_interrupt(rmt_dev_t *dev, uint32_t channel, bool enable)
{
    dev->int_ena.val &= ~(1 << (channel * 3));
    dev->int_ena.val |= (enable << (channel * 3));
}

__attribute__((always_inline))
static inline void rmt_ll_enable_tx_err_interrupt(rmt_dev_t *dev, uint32_t channel, bool enable)
{
    dev->int_ena.val &= ~(1 << (channel * 3 + 2));
    dev->int_ena.val |= (enable << (channel * 3 + 2));
}

__attribute__((always_inline))
static inline void rmt_ll_enable_tx_thres_interrupt(rmt_dev_t *dev, uint32_t channel, bool enable)
{
    dev->int_ena.val &= ~(1 << (channel + 24));
    dev->int_ena.val |= (enable << (channel + 24));
}

__attribute__((always_inline))
static inline void rmt_ll_clear_tx_end_interrupt(rmt_dev_t *dev, uint32_t channel)
{
    dev->int_clr.val = (1 << (channel * 3));
}

__attribute__((always_inline))
static inline void rmt_ll_clear_tx_err_interrupt(rmt_dev_t *dev, uint32_t channel)
{
    dev->int_clr.val = (1 << (channel * 3 + 2));
}

__attribute__((always_inline))
static inline void rmt_ll_clear_tx_thres_interrupt(rmt_dev_t *dev, uint32_t channel)
{
    dev->int_clr.val = (1 << (channel + 24));
}


__attribute__((always_inline))
static inline uint32_t rmt_ll_get_tx_thres_interrupt_status(rmt_dev_t *dev)
{
    uint32_t status =  dev->int_st.val;
    return (status & 0xFF000000) >> 24;
}

#endif

#if defined(CONFIG_BTDM_CTRL_HLI)
// Espressif's bluetooth driver offers a helpful sharing layer; bring in the interrupt management calls
#include "hal/interrupt_controller_hal.h"
extern "C" esp_err_t hli_intr_register(intr_handler_t handler, void* arg, uint32_t intr_reg, uint32_t intr_mask);

#else /* !CONFIG_BTDM_CTRL_HLI*/

// Link our high-priority ISR handler
extern "C" void ld_include_hli_vectors_rmt();   // an object with an address, but no space

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32S3)
#include "soc/periph_defs.h"
#endif

#if CONFIG_ESP_SYSTEM_CHECK_INT_LEVEL_5
#define INT_LEVEL_FLAG ESP_INTR_FLAG_LEVEL4
#else
#define INT_LEVEL_FLAG ESP_INTR_FLAG_LEVEL5
#endif

#endif  /* CONFIG_BTDM_CTRL_HLI */


// RMT driver implementation
struct NeoEsp32RmtHIChannelState {
    uint32_t rmtBit0, rmtBit1;
    uint32_t resetDuration;

    const byte* txDataStart;    // data array
    const byte* txDataEnd;      // one past end
    const byte* txDataCurrent;      // current location
    size_t rmtOffset;
};

// Global variables
#ifndef CONFIG_BTDM_CTRL_HLI
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)     // New IDF
static intr_handle_t isrHandle = nullptr;
#endif
#endif /* CONFIG_BTDM_CTRL_HLI */
static NeoEsp32RmtHIChannelState** driverState = nullptr;
constexpr size_t rmtBatchSize =  RMT_MEM_ITEM_NUM / 2;
static uint32_t isr_count = 0;

// Fill the RMT buffer memory
// This is implemented using many arguments instead of passing the structure object to ensure we do only one lookup
// All the arguments are passed in registers, so they don't need to be looked up again
static void IRAM_ATTR RmtFillBuffer(uint8_t channel, const byte** src_ptr, const byte* end, uint32_t bit0, uint32_t bit1, size_t* offset_ptr, size_t reserve) {
    // We assume that (rmtToWrite % 8) == 0
    size_t rmtToWrite = rmtBatchSize - reserve;
    rmt_item32_t* dest =(rmt_item32_t*)  &RMTMEM.chan[channel].data32[*offset_ptr + reserve]; // write directly in to RMT memory
    const byte* psrc = *src_ptr;

    *offset_ptr ^= rmtBatchSize;

    if (psrc != end) {
        while (rmtToWrite > 0) {
            uint8_t data = *psrc;
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                dest->val = (data & 0x80) ? bit1 : bit0;
                dest++;
                data <<= 1;
            }
            rmtToWrite -= 8;
            psrc++;

            if (psrc == end) {
                break;
            }
        }

        *src_ptr = psrc;
    }

    if (rmtToWrite > 0) {
        // Add end event
        rmt_item32_t bit0_val = {{.val = bit0 }};
        *dest = rmt_item32_t {{{ .duration0 = 0, .level0 = bit0_val.level1, .duration1 = 0, .level1 = 0 }}};
    }
}

static void IRAM_ATTR RmtStartWrite(uint8_t channel, NeoEsp32RmtHIChannelState& state) {
    // Reset context state
    state.rmtOffset = 0;

    // Fill the first part of the buffer with a reset event
    // FUTURE: we could do timing analysis with the last interrupt on this channel
    // Use 8 words to stay aligned with the buffer fill logic
    uint32_t idle_lvl = (rmt_item32_t  {{.val = state.rmtBit0}}).level1;
    rmt_item32_t fill = {{{ .duration0 = 1, .level0 = idle_lvl, .duration1 = 1, .level1 = idle_lvl }}};
    rmt_item32_t* dest = (rmt_item32_t*) &RMTMEM.chan[channel].data32[0];
    for (auto i = 0; i < 7; ++i) dest[i] = fill;
    fill.duration1 = state.resetDuration - 17;
    dest[7] = fill;

    // Fill the remaining buffer with real data
    RmtFillBuffer(channel, &state.txDataCurrent, state.txDataEnd, state.rmtBit0, state.rmtBit1, &state.rmtOffset, 8);
    RmtFillBuffer(channel, &state.txDataCurrent, state.txDataEnd, state.rmtBit0, state.rmtBit1, &state.rmtOffset, 0);

    // Start operation
    rmt_ll_clear_tx_thres_interrupt(&RMT, channel);
    rmt_ll_tx_reset_pointer(&RMT, channel);
    rmt_ll_tx_start(&RMT, channel);
}

extern "C" void IRAM_ATTR NeoEsp32RmtMethodIsr(void *arg) {
    // Tx threshold interrupt
    ++isr_count;
    uint32_t status = rmt_ll_get_tx_thres_interrupt_status(&RMT);
    while (status) {
        uint8_t channel = __builtin_ffs(status) - 1;                
        if (driverState[channel]) {            
            // Normal case
            NeoEsp32RmtHIChannelState& state = *driverState[channel];
            RmtFillBuffer(channel, &state.txDataCurrent, state.txDataEnd, state.rmtBit0, state.rmtBit1, &state.rmtOffset, 0);
        } else {
            // Danger - another driver got invoked?
            rmt_ll_tx_stop(&RMT, channel);
        }
        rmt_ll_clear_tx_thres_interrupt(&RMT, channel);
        status = rmt_ll_get_tx_thres_interrupt_status(&RMT);
    }
};

// Wrapper around the register analysis defines
// For all currently supported chips, this is constant for all channels; but this is not true of *all* ESP32
static inline bool _RmtStatusIsTransmitting(rmt_channel_t channel, uint32_t status) {
    uint32_t v;
    switch(channel) {
#ifdef RMT_STATE_CH0        
        case 0: v = (status >> RMT_STATE_CH0_S) & RMT_STATE_CH0_V; break;
#endif
#ifdef RMT_STATE_CH1
        case 1: v = (status >> RMT_STATE_CH1_S) & RMT_STATE_CH1_V; break;
#endif
#ifdef RMT_STATE_CH2
        case 2: v = (status >> RMT_STATE_CH2_S) & RMT_STATE_CH2_V; break;
#endif
#ifdef RMT_STATE_CH3        
        case 3: v = (status >> RMT_STATE_CH3_S) & RMT_STATE_CH3_V; break;
#endif        
#ifdef RMT_STATE_CH4
        case 4: v = (status >> RMT_STATE_CH4_S) & RMT_STATE_CH4_V; break;
#endif
#ifdef RMT_STATE_CH5
        case 5: v = (status >> RMT_STATE_CH5_S) & RMT_STATE_CH5_V; break;
#endif
#ifdef RMT_STATE_CH6
        case 6: v = (status >> RMT_STATE_CH6_S) & RMT_STATE_CH6_V; break;
#endif
#ifdef RMT_STATE_CH7
        case 7: v = (status >> RMT_STATE_CH7_S) & RMT_STATE_CH7_V; break;
#endif
        default: v = 0;
    }

    return v != 0;
}


esp_err_t NeoEsp32RmtHiMethodDriver::Install(rmt_channel_t channel, uint32_t rmtBit0, uint32_t rmtBit1, uint32_t reset) {
    esp_err_t err = ESP_OK;
    if (!driverState) {
        // First time init
        driverState = reinterpret_cast<NeoEsp32RmtHIChannelState**>(heap_caps_calloc(RMT_CHANNEL_MAX, sizeof(NeoEsp32RmtHIChannelState*), MALLOC_CAP_INTERNAL));
        if (!driverState) return ESP_ERR_NO_MEM;
        
        // Bind interrupt handler
#if defined(CONFIG_BTDM_CTRL_HLI)
        // Bluetooth driver has taken the empty high-priority interrupt.  Fortunately, it allows us to
        // hook up another handler.
        err = hli_intr_register(NeoEsp32RmtMethodIsr, nullptr, (uintptr_t) &RMT.int_st, 0xFF000000);
        // 25 is the magic number of the bluetooth ISR on ESP32 - see soc/soc.h.
        intr_matrix_set(cpu_hal_get_core_id(), ETS_RMT_INTR_SOURCE, 25);
        intr_cntrl_ll_enable_interrupts(1<<25);
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)     // New IDF
        // Our custom ISR is bound by the linker; passing a pointer to an address that source file here guarantees that we link it in
        err = esp_intr_alloc(ETS_RMT_INTR_SOURCE, INT_LEVEL_FLAG | ESP_INTR_FLAG_IRAM, nullptr, (void*) &ld_include_hli_vectors_rmt, &isrHandle);
#else   // IDF 3
        // Old IDF doesn't allow us to register the interrupt; it's flagged as reserved.
        // Ensure all interruptss are cleared first
        RMT.int_ena.val = 0;
        RMT.int_clr.val = 0xFFFFFFFF;

        static volatile const void*  __attribute__((used)) pleaseLinkAssembly = (void*) ld_include_hli_vectors_rmt;

        intr_matrix_set(xPortGetCoreID(), ETS_RMT_INTR_SOURCE, 26);
        ESP_INTR_ENABLE(26);
#endif

        if (err != ESP_OK) {
            Serial.printf("Couldn't assign RMT IRQ: %d\n", err);
            heap_caps_free(driverState);
            driverState = nullptr;
            return err;
        }
    }

    if (driverState[channel] != nullptr) {
        return ESP_ERR_INVALID_STATE;   // already in use
    }

    NeoEsp32RmtHIChannelState* state = reinterpret_cast<NeoEsp32RmtHIChannelState*>(heap_caps_calloc(1, sizeof(NeoEsp32RmtHIChannelState), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (state == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    // Store timing information
    state->rmtBit0 = rmtBit0;
    state->rmtBit1 = rmtBit1;
    state->resetDuration = reset;

    // Initialize hardware
    rmt_ll_tx_stop(&RMT, channel);
    rmt_ll_tx_reset_pointer(&RMT, channel);
    rmt_ll_enable_tx_err_interrupt(&RMT, channel, false);
    rmt_ll_enable_tx_end_interrupt(&RMT, channel, false);
    rmt_ll_enable_tx_thres_interrupt(&RMT, channel, false);
    rmt_ll_clear_tx_err_interrupt(&RMT, channel);
    rmt_ll_clear_tx_end_interrupt(&RMT, channel);
    rmt_ll_clear_tx_thres_interrupt(&RMT, channel);
    
    rmt_ll_tx_enable_loop(&RMT, channel, false);
    rmt_ll_tx_enable_pingpong(&RMT, channel, true);
    rmt_ll_tx_set_limit(&RMT, channel, rmtBatchSize);

    driverState[channel] = state;

    rmt_ll_enable_tx_thres_interrupt(&RMT, channel, true);

    //Serial.printf("RMT %d install\n", channel);
    return err;
}

esp_err_t  NeoEsp32RmtHiMethodDriver::Uninstall(rmt_channel_t channel) {
    if (!driverState || !driverState[channel]) return ESP_ERR_INVALID_ARG;

    NeoEsp32RmtHIChannelState* state = driverState[channel];

    WaitForTxDone(channel, 10000 / portTICK_PERIOD_MS);

    // Done or not, we're out of here
    rmt_ll_tx_stop(&RMT, channel);
    rmt_ll_enable_tx_thres_interrupt(&RMT, channel, false);
    driverState[channel] = nullptr;
    heap_caps_free(state);

    // TODO: turn off the driver ISR and release global state if none are left
    //Serial.printf("RMT %d uninstall\n", channel);

    return ESP_OK;
}

esp_err_t  NeoEsp32RmtHiMethodDriver::Write(rmt_channel_t channel, const uint8_t *src, size_t src_size) {
    if (!driverState || !driverState[channel]) return ESP_ERR_INVALID_ARG;

    NeoEsp32RmtHIChannelState& state = *driverState[channel];
    esp_err_t result = WaitForTxDone(channel, 10000 / portTICK_PERIOD_MS);

    if (result == ESP_OK) {
        state.txDataStart = src;
        state.txDataCurrent = src;
        state.txDataEnd = src + src_size;
        RmtStartWrite(channel, state);
        //Serial.printf("RMT %d write begin, %d/%d sent, status %08X, ic %d\n", channel, state.txDataCurrent - state.txDataStart, state.txDataEnd - state.txDataStart, rmt_ll_tx_get_channel_status(&RMT, channel), isr_count);
    }
    return result;
}

esp_err_t NeoEsp32RmtHiMethodDriver::WaitForTxDone(rmt_channel_t channel, TickType_t wait_time) {
    if (!driverState || !driverState[channel]) return ESP_ERR_INVALID_ARG;

    NeoEsp32RmtHIChannelState& state = *driverState[channel];
    // yield-wait until wait_time
    esp_err_t rv = ESP_OK;
    //uint32_t loop_count = 0;
    uint32_t status;
    while(1) {
        status = rmt_ll_tx_get_channel_status(&RMT, channel);
        if (!_RmtStatusIsTransmitting(channel, status)) break;
        if (wait_time == 0) { rv = ESP_ERR_TIMEOUT; break; };

        //++loop_count;
        TickType_t sleep = std::min(wait_time, 5U);
        vTaskDelay(sleep);
        wait_time -= sleep;
    };
/*
    if (loop_count) {
        Serial.printf("RMT %d wait %d: %d/%d sent, status %08X, ic %d\n", channel, rv, state.txDataCurrent - state.txDataStart, state.txDataEnd - state.txDataStart, status, isr_count);
    }
*/        
    return rv;
}



#endif