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
#include "hal/cpu_hal.h"
#include "hal/rmt_ll.h"
#include "hal/interrupt_controller_hal.h"

#if CONFIG_ESP_SYSTEM_CHECK_INT_LEVEL_5
#define INT_LEVEL_FLAG ESP_INTR_FLAG_LEVEL4
#else
#define INT_LEVEL_FLAG ESP_INTR_FLAG_LEVEL5
#endif

// RMT driver implementation
struct NeoEsp32RmtDriverState {
    rmt_item32_t rmtBit0, rmtBit1;
    uint32_t resetDuration;

    const byte* txDataStart;    // data array
    const byte* txDataEnd;      // one past end
    const byte* txDataCurrent;      // current location
    size_t rmtOffset;

    uint32_t txMaxCycles;
    uint32_t lastTxInterruptCycleCount;

    size_t overruns;
    struct overrun_info {
        uint32_t last, current, index, isr;
    } overrun_debug[32];
};

static intr_handle_t isrHandle = nullptr;
static NeoEsp32RmtDriverState** driverState = nullptr;
constexpr size_t rmtBatchSize =  RMT_MEM_ITEM_NUM / 2;

// Ensure the assembly code is linked by requiring a unique symbol from it
extern "C" int ld_include_hli_vectors_rmt;
const void* RmtForceAssemblyLink = &ld_include_hli_vectors_rmt;

// Fill the RMT buffer memory
static inline void IRAM_ATTR RmtFillBuffer(uint8_t channel, NeoEsp32RmtDriverState& state, size_t reserve) {
    // We assume that (rmtToWrite % 8) == 0
    size_t rmtToWrite = rmtBatchSize - reserve;
    rmt_item32_t* dest =(rmt_item32_t*)  &RMTMEM.chan[channel].data32[state.rmtOffset + reserve]; // write directly in to RMT memory
    const byte* psrc = state.txDataCurrent;
    const byte* end = state.txDataEnd;

    if (psrc != end) {
        // No lookups in the main loop
        const rmt_item32_t rmtBit0 = state.rmtBit0;
        const rmt_item32_t rmtBit1 = state.rmtBit1;

        while (rmtToWrite > 0) {
            uint8_t data = *psrc;
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                *dest = (data & 0x80) ? rmtBit1 : rmtBit0;
                dest++;
                data <<= 1;
            }
            rmtToWrite -= 8;
            psrc++;

            if (psrc == end) {
                break;
            }
        }

        state.txDataCurrent = psrc;
    }

    if (rmtToWrite > 0) {
        // Add end event
        *dest = rmt_item32_t {{{ .duration0 = 0, .level0 = state.rmtBit0.level1, .duration1 = 0, .level1 = 0 }}};
    }

    state.rmtOffset ^= rmtBatchSize;
}

static void IRAM_ATTR RmtStartWrite(uint8_t channel, NeoEsp32RmtDriverState& state) {
    // Reset context state
    state.rmtOffset = 0;
    state.lastTxInterruptCycleCount = 0;

    // Fill the first part of the buffer with a reset event
    // Use 8 words to stay aligned with the buffer fill
    rmt_item32_t fill = {{{ .duration0 = 1, .level0 = state.rmtBit0.level1, .duration1 = 1, .level1 = state.rmtBit0.level1 }}};
    rmt_item32_t* dest = (rmt_item32_t*) &RMTMEM.chan[channel].data32[0];
    for (auto i = 0; i < 7; ++i) dest[i] = fill;
    fill.duration1 = state.resetDuration - 17;
    dest[7] = fill;

    // Fill the remaining buffer with real data
    RmtFillBuffer(channel, state, 8);
    RmtFillBuffer(channel, state, 0);

    // Start operation
    rmt_ll_tx_reset_pointer(&RMT, channel);
    rmt_ll_tx_start(&RMT, channel);
}


extern "C" void IRAM_ATTR NeoEsp32RmtMethodIsr(void *arg) {
    // Tx threshold interrupt
    uint32_t status_processed = 0;
    uint32_t status = rmt_ll_get_tx_thres_interrupt_status(&RMT);
    while (status) {
        uint8_t channel = __builtin_ffs(status) - 1;
                
        if (driverState[channel] && ((status_processed & (1<<channel)) == 0)) {            
            NeoEsp32RmtDriverState& state = *driverState[channel];
            uint32_t ccount = cpu_hal_get_cycle_count();
            if ((state.lastTxInterruptCycleCount == 0) || ((ccount - state.lastTxInterruptCycleCount) < state.txMaxCycles)) {
                // Normal case
                RmtFillBuffer(channel, state, 0);
                state.lastTxInterruptCycleCount = cpu_hal_get_cycle_count();
            } else {
                // Overrun
                // Reset RMT
                rmt_ll_tx_stop(&RMT, channel);

                if (state.overruns < 32) {
                    auto& ord = state.overrun_debug[state.overruns];
                    ord.last = state.lastTxInterruptCycleCount;
                    ord.current = ccount;
                    ord.index = state.txDataCurrent - state.txDataStart;
                }
                // Reset the context
                state.txDataCurrent = state.txDataStart;
                state.overruns++;
                RmtStartWrite(channel, state);
            }            
            status_processed |= (1<<channel);
        } else {
            // Danger - either not our channel, or we've processed this channel without leaving the loop!
            // Indicates 100% cpu load doing nothing but processing RMTs
            // Stop.
            rmt_ll_tx_stop(&RMT, channel);
        }
        rmt_ll_clear_tx_thres_interrupt(&RMT, channel);
        status = rmt_ll_get_tx_thres_interrupt_status(&RMT);
    }
};


esp_err_t NeoEsp32RmtHiMethodDriver::Install(rmt_channel_t channel, uint32_t rmtBit0, uint32_t rmtBit1, uint32_t reset) {
    esp_err_t err = ESP_OK;
    if (!driverState) {
        // First time init
        driverState = reinterpret_cast<NeoEsp32RmtDriverState**>(heap_caps_calloc(RMT_CHANNEL_MAX, sizeof(NeoEsp32RmtDriverState*), MALLOC_CAP_INTERNAL));
        if (!driverState) return ESP_ERR_NO_MEM;

        /* TODO: Ensure no RMT interrupts are pending ? */
        // Ensure our driver is linked


        // Bind interrupt handler
        // For "high level" interrupts, we have to allocate separately from binding the handler
#if !defined(CONFIG_ESP_SYSTEM_CHECK_INT_LEVEL_5) || !defined(CONFIG_BTDM_CTRL_HLI)
        err = esp_intr_alloc(ETS_RMT_INTR_SOURCE, INT_LEVEL_FLAG | ESP_INTR_FLAG_IRAM, nullptr, nullptr, &isrHandle);
        if (err != ESP_OK) {
            Serial.printf("Couldn't assign RMT IRQ: %d\n", err);
            heap_caps_free(driverState);
            driverState = nullptr;
            return err;
        }
#else
        // We must use the NMI as Levels 4 and 5 are already in use.  Hope nobody minds.
        // 14 is the magic number of the NMI destiatno.
        intr_matrix_set(cpu_hal_get_core_id(), ETS_RMT_INTR_SOURCE, 14);
#endif
    }

    if (driverState[channel] != nullptr) {
        return ESP_ERR_INVALID_STATE;   // already in use
    }

    NeoEsp32RmtDriverState* state = reinterpret_cast<NeoEsp32RmtDriverState*>(heap_caps_calloc(1, sizeof(NeoEsp32RmtDriverState), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (state == nullptr) {
        return ESP_ERR_NO_MEM;
    }

    state->rmtBit0.val = rmtBit0;
    state->rmtBit1.val = rmtBit1;
    state->resetDuration = reset;

    // Overrun detection threshold
    state->txMaxCycles = ((2 * rmtBatchSize) * NeoEsp32RmtSpeed::CyclesPerSample(rmtBit0)) - 1000;

    // Initialize hardware
    rmt_ll_tx_stop(&RMT, channel);
    rmt_ll_tx_enable_loop(&RMT, channel, false);
    rmt_ll_tx_set_limit(&RMT, channel, rmtBatchSize);

    driverState[channel] = state;

    rmt_ll_enable_tx_thres_interrupt(&RMT, channel, true);

    Serial.printf("RMT %d install\n", channel);
    return err;
}

esp_err_t  NeoEsp32RmtHiMethodDriver::Uninstall(rmt_channel_t channel) {
    if (!driverState || !driverState[channel]) return ESP_ERR_INVALID_ARG;

    NeoEsp32RmtDriverState* state = driverState[channel];

    esp_err_t result = WaitForTxDone(channel, 10000 / portTICK_PERIOD_MS);
    if (result == ESP_OK) { // TODO - can we allow this to fail???
        rmt_ll_enable_tx_thres_interrupt(&RMT, channel, false);
        driverState[channel] = nullptr;
        heap_caps_free(state);

        // TODO: turn off the driver ISR and release global state if none are left
        Serial.printf("RMT %d uninstall\n", channel);
    };
    return result;
}

esp_err_t  NeoEsp32RmtHiMethodDriver::Write(rmt_channel_t channel, const uint8_t *src, size_t src_size) {
    if (!driverState || !driverState[channel]) return ESP_ERR_INVALID_ARG;

    NeoEsp32RmtDriverState& state = *driverState[channel];
    esp_err_t result = WaitForTxDone(channel, 10000 / portTICK_PERIOD_MS);

    if (state.overruns) {
        Serial.printf("RMT %d handled overruns %d -- threshold %d\n", channel, state.overruns, state.txMaxCycles);
        for(unsigned i = 0; i < state.overruns; ++i) {
            if (i >= 32) break;
            auto& ord = state.overrun_debug[i];
            Serial.printf("RMT %d: %u - %u %u -> %u %u @= %d\n", channel, ord.last, ord.isr, ord.current, ord.current - ord.last, ord.current - ord.isr, ord.index);
        }
        state.overruns = 0;
    }

    if (result == ESP_OK) {
        state.txDataStart = src;
        state.txDataCurrent = src;
        state.txDataEnd = src + src_size;
        state.overruns = 0;
        RmtStartWrite(channel, state);

        Serial.printf("RMT %d write begin, %d/%d sent, status %08X\n", channel, state.txDataCurrent - state.txDataStart, state.txDataEnd - state.txDataStart, rmt_ll_tx_get_channel_status(&RMT, channel));
    }
    return result;
}

esp_err_t NeoEsp32RmtHiMethodDriver::WaitForTxDone(rmt_channel_t channel, TickType_t wait_time) {
    if (!driverState || !driverState[channel]) return ESP_ERR_INVALID_ARG;

    NeoEsp32RmtDriverState& state = *driverState[channel];
    // yield-wait until wait_time
    unsigned loop_count = 0;
    esp_err_t rv = ESP_OK;
    uint32_t status;
    while(1) {
        status = rmt_ll_tx_get_channel_status(&RMT, channel);
        if ((state.txDataCurrent == state.txDataEnd) && ((status & 0x07000000) == 0)) break; /* Stopped state while not at end could mean we caught it restarting on an error */
        if (wait_time == 0) { rv = ESP_ERR_TIMEOUT; break; };

        ++loop_count;

        TickType_t sleep = std::min(wait_time, 5U);
        vTaskDelay(sleep);
        wait_time -= sleep;
    };

    if (loop_count > 0) {
        Serial.printf("RMT %d wait %d: %u, %d/%d sent, status %08X, overruns %d\n", channel, loop_count, rv, state.txDataCurrent - state.txDataStart, state.txDataEnd - state.txDataStart, status, state.overruns);
    }
    return rv;
}



#endif