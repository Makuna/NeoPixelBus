// WARNING:  This file contains code that is more than likely already 
// exposed from the Esp32 Arduino API.  It will be removed once integration is complete.
//
// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#if defined(ARDUINO_ARCH_ESP32)

#include "sdkconfig.h" // this sets useful config symbols, like CONFIG_IDF_TARGET_ESP32C3

#include <string.h>
#include <stdio.h>
#include "stdlib.h"

// ESP32 C3, S3, C6, and H2 I2S is not supported yet due to significant changes to interface
#if !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32H2)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "FractionClk.h"

#if ESP_IDF_VERSION_MAJOR>=4
#include "esp_intr_alloc.h"
#else
#include "esp_intr.h"
#endif

#include "rom/lldesc.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/io_mux_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/i2s_struct.h"

#if defined(CONFIG_IDF_TARGET_ESP32)
/* included here for ESP-IDF v4.x compatibility */
#include "soc/dport_reg.h"
#endif

#include "driver/gpio.h"
#include "driver/i2s.h"

#if ESP_IDF_VERSION_MAJOR>=5
#include "rom/gpio.h"
#include "esp_private/periph_ctrl.h"
#endif

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#include "driver/dac.h"
#endif



#include "Esp32_i2s.h"
#include "esp32-hal.h"

esp_err_t i2sSetClock(uint8_t bus_num, uint8_t div_num, uint8_t div_b, uint8_t div_a, uint8_t bck, uint8_t bits_per_sample);
esp_err_t i2sSetSampleRate(uint8_t bus_num,
    uint16_t dmaBitPerDataBit,
    uint16_t nsBitSendTime,
    bool parallel_mode,
    size_t bytesPerSample);

#define MATRIX_DETACH_OUT_SIG 0x100

#if ESP_IDF_VERSION_MAJOR<=5
#define I2S_BASE_CLK (160000000L)
#endif

#define I2S_DMA_BLOCK_COUNT_DEFAULT      0
// 50us reset will calculate out as a 16 byte silence/reset buffer
// The latest DMA buffer model this library uses will allow the 
// silence blocks to as small as they can be (4 bytes) 
// this also allows the primary data to send less of that silence since
// the silence state control blocks (2 front, 1 back) will consume 12 bytes
// of that reset time already
#define I2S_DMA_SILENCE_SIZE     4 // must be in 4 byte increments
#define I2S_DMA_SILENCE_BLOCK_COUNT_FRONT  2 // two front
#define I2S_DMA_SILENCE_BLOCK_COUNT_BACK  1 // one back, required for non parallel

// compatibility shim between versions of the IDF
// note that I2S_NUM_MAX is an enum element, so we check for
// existence of the new SOC_I2S_NUM
//
#if defined(SOC_I2S_NUM)
#define NEO_I2S_COUNT  (SOC_I2S_NUM)
#else
#define NEO_I2S_COUNT  (I2S_NUM_MAX)
#endif

typedef struct 
{
    i2s_dev_t* bus;
    int8_t  ws;
    int8_t  bck;
    int8_t  out;
    int8_t  in;

    intr_handle_t isr_handle;
    lldesc_t* dma_items;
    size_t dma_count;

    volatile uint32_t is_sending_data;
} i2s_bus_t;

// is_sending_data values
#define I2s_Is_Idle 0
#define I2s_Is_Pending 1
#define I2s_Is_Sending 2

#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
// (NEO_I2S_COUNT == 2)
static i2s_bus_t I2S[NEO_I2S_COUNT] = 
{
    {&I2S0, -1, -1, -1, -1, NULL, NULL, I2S_DMA_BLOCK_COUNT_DEFAULT, I2s_Is_Idle},
    {&I2S1, -1, -1, -1, -1, NULL, NULL, I2S_DMA_BLOCK_COUNT_DEFAULT, I2s_Is_Idle}
};
#else
static i2s_bus_t I2S[NEO_I2S_COUNT] = 
{
    {&I2S0, -1, -1, -1, -1, NULL, NULL, I2S_DMA_BLOCK_COUNT_DEFAULT, I2s_Is_Idle}
};
#endif

void IRAM_ATTR i2sDmaISR(void* arg);

inline void dmaItemInit(lldesc_t* item, uint8_t* posData, size_t sizeData, lldesc_t* itemNext)
{
    item->eof = 0;
    item->owner = 1;
    item->sosf = 0;
    item->offset = 0;
    item->buf = posData;
    item->size = sizeData;
    item->length = sizeData;
    item->qe.stqe_next = itemNext;
}

bool i2sInitDmaItems(uint8_t bus_num, 
    uint8_t* data, 
    size_t dataSize, 
    bool parallel_mode, 
    size_t bytesPerSample)
{
    if (bus_num >= NEO_I2S_COUNT) 
    {
        return false;
    }

    // silenceSize is minimum data size to loop i2s for reset
    // it is not the actual reset time, but parallel mode needs
    // a bit more time since the data is x8/x16
    size_t silenceSize = parallel_mode ? 
        I2S_DMA_SILENCE_SIZE * 8 * bytesPerSample : I2S_DMA_SILENCE_SIZE;
    size_t dmaCount = I2S[bus_num].dma_count;

    if (I2S[bus_num].dma_items == NULL) 
    {
        I2S[bus_num].dma_items = (lldesc_t*)heap_caps_malloc(dmaCount * sizeof(lldesc_t), MALLOC_CAP_DMA);
        if (I2S[bus_num].dma_items == NULL) 
        {
            log_e("MEM ERROR!");
            return false;
        }
    }

    lldesc_t* itemFirst = &I2S[bus_num].dma_items[0];
    lldesc_t* item = itemFirst;
//    lldesc_t* itemsEnd = itemFirst + I2S[bus_num].dma_count;
    lldesc_t* itemNext = item + 1;
    // The primary data to map will excludes the time to process through the
    // 3 silence control blocks.
    // That data at the end is already silent reset time and no need to send it twice as
    // part of the data and the control blocks
    // makes the reset more accurate
    size_t dataLeft = dataSize - (silenceSize * 
            (I2S_DMA_SILENCE_BLOCK_COUNT_FRONT + I2S_DMA_SILENCE_BLOCK_COUNT_BACK));
    uint8_t* pos = data;
    // at the end of the data is the encoded silence reset, use it
    uint8_t* posSilence = data + dataSize - silenceSize;

    // front two are silent items used for looping to micmic single fire
    //  default to looping
    dmaItemInit(item, posSilence, silenceSize, itemNext);
    dmaItemInit(itemNext, posSilence, silenceSize, item);
    item = itemNext;
    itemNext++;

    // init blocks with avialable data
    //
    while (dataLeft)
    {
        item = itemNext;
        itemNext++;

        size_t blockSize = dataLeft;
        if (blockSize > I2S_DMA_MAX_DATA_LEN)
        {
            blockSize = I2S_DMA_MAX_DATA_LEN;
        }
        dataLeft -= blockSize;

        dmaItemInit(item, pos, blockSize, itemNext);

        pos += blockSize;
    }

    // last data item is EOF to manage send state using EOF ISR
    item->eof = 1;

    // last block, the back silent item, loops to front
    item = itemNext;
    dmaItemInit(item, posSilence, silenceSize, itemFirst);

    return true;
}

bool i2sDeinitDmaItems(uint8_t bus_num) 
{
    if (bus_num >= NEO_I2S_COUNT) 
    {
        return false;
    }

    heap_caps_free(I2S[bus_num].dma_items);
    I2S[bus_num].dma_items = NULL;

    return true;
}

esp_err_t i2sSetClock(uint8_t bus_num, 
        uint8_t div_num, 
        uint8_t div_b,   
        uint8_t div_a,   
        uint8_t bck,     
        uint8_t bits)    
{
    if (bus_num >= NEO_I2S_COUNT || div_a > 63 || div_b > 63 || bck > 63) 
    {
        return ESP_FAIL;
    }

    log_i("i2sSetClock bus %u,\n clkm_div_num %u,\n clk_div_a %u,\n clk_div_b %u,\n bck_div_num %u,\n bits_mod %u,\n i2sClkBase %u",
        bus_num,
        div_num,
        div_a,
        div_b,
        bck,
        bits,
        I2S_BASE_CLK);

    i2s_dev_t* i2s = I2S[bus_num].bus;

    typeof(i2s->clkm_conf) clkm_conf;

    clkm_conf.val = 0;

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
    clkm_conf.clk_sel = 2; // APPL = 1 APB = 2
    clkm_conf.clk_en = 1; // examples of i2s show this being set if sel is set to 2
#else
    clkm_conf.clk_en = 1;
    clkm_conf.clka_en = 0;
#endif

    clkm_conf.clkm_div_a = div_a;
    clkm_conf.clkm_div_b = div_b;
    clkm_conf.clkm_div_num = div_num;
    i2s->clkm_conf.val = clkm_conf.val;

    typeof(i2s->sample_rate_conf) sample_rate_conf;
    sample_rate_conf.val = 0;
    sample_rate_conf.tx_bck_div_num = bck;
//    sample_rate_conf.rx_bck_div_num = 4;
    sample_rate_conf.tx_bits_mod = bits;
//    sample_rate_conf.rx_bits_mod = bits;
    i2s->sample_rate_conf.val = sample_rate_conf.val;

    return ESP_OK;
}

void i2sSetPins(uint8_t bus_num, 
        int8_t out, 
        int8_t parallel,
        int8_t busSampleSize, 
        bool invert)
{
    if (bus_num >= NEO_I2S_COUNT) 
    {
        return;
    }

    if (out >= 0) 
    {
        uint32_t i2sSignal;

        pinMode(out, OUTPUT);

#if defined(CONFIG_IDF_TARGET_ESP32S2)

        // S2 only has one bus
        //  single output I2S0O_DATA_OUT23_IDX
        //  in parallel mode
        //  8bit mode   : I2S0O_DATA_OUT16_IDX ~I2S0O_DATA_OUT23_IDX
        //  16bit mode  : I2S0O_DATA_OUT8_IDX ~I2S0O_DATA_OUT23_IDX
        //  24bit mode  : I2S0O_DATA_OUT0_IDX ~I2S0O_DATA_OUT23_IDX
        i2sSignal = I2S0O_DATA_OUT23_IDX;
        if (parallel != -1)
        {
            i2sSignal -= ((busSampleSize * 8) - 1);
            i2sSignal += parallel;
        }

#else
        i2sSignal = I2S0O_DATA_OUT0_IDX;

        if (bus_num == 1)
        {
            i2sSignal = I2S1O_DATA_OUT0_IDX;
        }

        if (parallel == -1)
        {
            i2sSignal += 23; // yup, single channel is on 23
        }
        else
        {
            if (busSampleSize == 2)
            {
                i2sSignal += 8;  // yup, 16 bits starts at 8
            }
            i2sSignal += parallel; // add the parallel channel index
        }

#endif
        //log_i("i2sSetPins bus %u, i2sSignal %u, pin %u, mux %u",
        //    bus_num,
        //    i2sSignal,
        //    out,
        //    parallel);
        gpio_matrix_out(out, i2sSignal, invert, false);
    } 
}

/* not used, but left around for reference
void i2sSetClkWsPins(uint8_t bus_num,
    int8_t outClk,
    bool invertClk,
    int8_t outWs,
    bool invertWs)
{

    if (bus_num >= NEO_I2S_COUNT)
    {
        return;
    }

    uint32_t i2sSignalClk = I2S0O_BCK_OUT_IDX;
    uint32_t i2sSignalWs = I2S0O_WS_OUT_IDX;

#if !defined(CONFIG_IDF_TARGET_ESP32S2)
    if (bus_num == 1)
    {
        i2sSignalClk = I2S1O_BCK_OUT_IDX;
        i2sSignalWs = I2S1O_WS_OUT_IDX;
    }
#endif

    if (outClk >= 0)
    {
        pinMode(outClk, OUTPUT);
        gpio_matrix_out(outClk, i2sSignalClk, invertClk, false);
    }

    if (outWs >= 0)
    {
        pinMode(outWs, OUTPUT);
        gpio_matrix_out(outWs, i2sSignalWs, invertWs, false);
    }
}
*/

bool i2sWriteDone(uint8_t bus_num) 
{
    if (bus_num >= NEO_I2S_COUNT) 
    {
        return false;
    }

    return (I2S[bus_num].is_sending_data == I2s_Is_Idle);
}

void i2sInit(uint8_t bus_num, 
        bool parallel_mode,
        size_t bytesPerSample, 
        uint16_t dmaBitPerDataBit,
        uint16_t nsBitSendTime,
        i2s_tx_chan_mod_t chan_mod, 
        i2s_tx_fifo_mod_t fifo_mod, 
        size_t dma_count, 
        uint8_t* data, 
        size_t dataSize)
{
    if (bus_num >= NEO_I2S_COUNT) 
    {
        return;
    }

    I2S[bus_num].dma_count = dma_count + 
            I2S_DMA_SILENCE_BLOCK_COUNT_FRONT +
            I2S_DMA_SILENCE_BLOCK_COUNT_BACK;

    if (!i2sInitDmaItems(bus_num, data, dataSize, parallel_mode, bytesPerSample))
    {
        return;
    }

#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
// (NEO_I2S_COUNT == 2)
    if (bus_num) 
    {
        periph_module_enable(PERIPH_I2S1_MODULE);
    } 
    else 
#endif
    {
        periph_module_enable(PERIPH_I2S0_MODULE);
    }

    esp_intr_disable(I2S[bus_num].isr_handle);
    i2s_dev_t* i2s = I2S[bus_num].bus;
    i2s->out_link.stop = 1;
    i2s->conf.tx_start = 0;
    i2s->int_ena.val = 0;
    i2s->int_clr.val = 0xFFFFFFFF;
    i2s->fifo_conf.dscr_en = 0;

    // reset i2s
    i2s->conf.tx_reset = 1;
    i2s->conf.tx_reset = 0;
    i2s->conf.rx_reset = 1;
    i2s->conf.rx_reset = 0;

    // reset dma
    i2s->lc_conf.in_rst = 1;
    i2s->lc_conf.in_rst = 0;
    i2s->lc_conf.out_rst = 1;
    i2s->lc_conf.out_rst = 0;

    // reset fifo
    i2s->conf.rx_fifo_reset = 1;
    i2s->conf.rx_fifo_reset = 0;
    i2s->conf.tx_fifo_reset = 1;
    i2s->conf.tx_fifo_reset = 0;


    // set parallel (LCD) mode
    {
        typeof(i2s->conf2) conf2;
        conf2.val = 0;
        conf2.lcd_en = parallel_mode;
        if ((parallel_mode) && (bytesPerSample == 1))
        {
            conf2.lcd_tx_wrx2_en = 1;
            conf2.lcd_tx_sdx2_en = 0;
        }
     
        i2s->conf2.val = conf2.val;
    }

    // Enable and configure DMA
    {
        typeof(i2s->lc_conf) lc_conf;
        lc_conf.val = 0;
        lc_conf.out_eof_mode = 1;
        i2s->lc_conf.val = lc_conf.val;
    }

#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
    i2s->pdm_conf.pcm2pdm_conv_en = 0;
    i2s->pdm_conf.pdm2pcm_conv_en = 0;
#endif
    // SET_PERI_REG_BITS(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_SOC_CLK_SEL, 0x1, RTC_CNTL_SOC_CLK_SEL_S);

    {
        typeof(i2s->fifo_conf) fifo_conf;

        fifo_conf.val = 0;
//        fifo_conf.rx_fifo_mod_force_en = 1; //?
        fifo_conf.tx_fifo_mod_force_en = 1;
        fifo_conf.tx_fifo_mod = fifo_mod; //  0-right&left channel;1-one channel
 //       fifo_conf.rx_fifo_mod = fifo_mod; //  0-right&left channel;1-one channel
 //       fifo_conf.rx_data_num = 32; //Thresholds.
        fifo_conf.tx_data_num = 32;

        i2s->fifo_conf.val = fifo_conf.val;
    }

    { 
        typeof(i2s->conf1) conf1;
        conf1.val = 0;

//        conf1.tx_pcm_conf = 1;
//        conf1.rx_pcm_bypass = 1;

        conf1.tx_stop_en = 0;
        conf1.tx_pcm_bypass = 1;
        i2s->conf1.val = conf1.val;
    }

    {
        typeof(i2s->conf_chan) conf_chan;
        conf_chan.val = 0;
        conf_chan.tx_chan_mod = chan_mod; //  0-two channel;1-right;2-left;3-righ;4-left
//        conf_chan.rx_chan_mod = chan_mod; //  0-two channel;1-right;2-left;3-righ;4-left
        i2s->conf_chan.val = conf_chan.val;
    }

    {
        typeof(i2s->conf) conf;
        conf.val = 0;
        conf.tx_msb_shift = !parallel_mode; // 0:DAC/PCM, 1:I2S
        conf.tx_right_first = 1; // parallel_mode? but no?
        conf.tx_short_sync = 0;
 //       conf.tx_msb_right = 1;
#if defined(CONFIG_IDF_TARGET_ESP32S2)
        conf.tx_dma_equal = parallel_mode;
#endif
        i2s->conf.val = conf.val;
    }

    i2s->timing.val = 0;

#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
//    i2s->pdm_conf.rx_pdm_en = 0;
    i2s->pdm_conf.tx_pdm_en = 0;
#endif
   
    
    i2sSetSampleRate(bus_num, 
            dmaBitPerDataBit,
            nsBitSendTime, 
            parallel_mode, 
            bytesPerSample);

    /* */
    //Reset FIFO/DMA -> needed? Doesn't dma_reset/fifo_reset do this?
    i2s->lc_conf.in_rst=1; i2s->lc_conf.out_rst=1; i2s->lc_conf.ahbm_rst=1; i2s->lc_conf.ahbm_fifo_rst=1;
    i2s->lc_conf.in_rst=0; i2s->lc_conf.out_rst=0; i2s->lc_conf.ahbm_rst=0; i2s->lc_conf.ahbm_fifo_rst=0;
    i2s->conf.tx_reset=1; i2s->conf.tx_fifo_reset=1; i2s->conf.rx_fifo_reset=1;
    i2s->conf.tx_reset=0; i2s->conf.tx_fifo_reset=0; i2s->conf.rx_fifo_reset=0;
    /* */

    //  enable intr in cpu // 
    int i2sIntSource;

#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
//    (NEO_I2S_COUNT == 2)
    if (bus_num == 1) 
    {
        i2sIntSource = ETS_I2S1_INTR_SOURCE;
    }
    else
#endif
    {
        i2sIntSource = ETS_I2S0_INTR_SOURCE;
    }

    esp_intr_alloc(i2sIntSource, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL1, &i2sDmaISR, &I2S[bus_num], &I2S[bus_num].isr_handle);
    //  enable send intr
    i2s->int_ena.out_eof = 1;
    i2s->int_ena.out_dscr_err = 1;

/*  ??? */
    // Enable and configure DMA
    {
        typeof(i2s->lc_conf) lc_conf;
        lc_conf.val = 0;
        lc_conf.out_data_burst_en = 0;  // ?
        lc_conf.indscr_burst_en = 0;    // ?
        i2s->lc_conf.val = lc_conf.val;
    }
/* */
    i2s->fifo_conf.dscr_en = 1;// enable dma
    i2s->out_link.start = 0;
    i2s->out_link.addr = (uint32_t)(&I2S[bus_num].dma_items[0]); // loads dma_struct to dma
    i2s->out_link.start = 1; // starts dma
    i2s->conf.tx_start = 1;// Start I2s module

    esp_intr_enable(I2S[bus_num].isr_handle);
}

void i2sDeinit(uint8_t bus_num) 
{
    i2sDeinitDmaItems(bus_num);
}

esp_err_t i2sSetSampleRate(uint8_t bus_num, 
        uint16_t dmaBitPerDataBit,
        uint16_t nsBitSendTime,
        bool parallel_mode, 
        size_t bytesPerSample)
{
    const double I2sClkMhz = (double)I2S_BASE_CLK / 1000000; // 160000000 = 160.0
    const size_t bits_per_sample = bytesPerSample * 8;

    if (bus_num >= NEO_I2S_COUNT) 
    {
        return ESP_FAIL;
    }

    uint8_t bck = 4; // must be 2+ due to ESP32S2 adjustment below
    double clkSampleAdj = 1.0;

    // parallel mode needs a higher sample rate
    //
    if (!parallel_mode)
    {
        // non-parallel uses two values in the sample
        // so the clock calcs need to adjust for this
        // as it makes output faster
        clkSampleAdj *= 2.0;
    }

    double clkmdiv = (double)nsBitSendTime / bytesPerSample / dmaBitPerDataBit / bck / 1000.0 * I2sClkMhz * clkSampleAdj;


    if (clkmdiv > 256.0) 
    {
        log_e("rate is too low");
        return ESP_FAIL;
    } 
    else if (clkmdiv < 2.0) 
    {
        log_e("rate is too fast, clkmdiv = %f (%u, %u, %u)",
            clkmdiv,
            nsBitSendTime,
            parallel_mode,
            bytesPerSample);
        return ESP_FAIL;
    }

#if defined(CONFIG_IDF_TARGET_ESP32S2)
    // ESP32S2 is just different 
    if (parallel_mode && bytesPerSample == 1)
    {
        bck /= 2;
    }
#endif

    // calc integer and franctional for more precise timing
    // 
    uint8_t clkmInteger = clkmdiv;
    double clkmFraction = (clkmdiv - clkmInteger);
    uint8_t divB = 0;
    uint8_t divA = 0;

    UnitDecimalToFractionClks(&divB, &divA, clkmFraction, 0.000001);

    i2sSetClock(bus_num, 
        clkmInteger, 
        divB,
        divA,
        bck, 
        bits_per_sample);

    return ESP_OK;
}

void IRAM_ATTR i2sDmaISR(void* arg)
{
    i2s_bus_t* i2s = (i2s_bus_t*)(arg);

    if (i2s->bus->int_st.out_eof) 
    {
        if (i2s->is_sending_data != I2s_Is_Idle)
        {
            // the second item (last of the two front silent items) is 
            // silent looping item
            lldesc_t* itemLoop = &i2s->dma_items[0];
            lldesc_t* itemLoopBreaker = itemLoop + 1;
            // set to loop on silent items
            itemLoopBreaker->qe.stqe_next = itemLoop;

            i2s->is_sending_data = I2s_Is_Idle;
        }
    }

    i2s->bus->int_clr.val = i2s->bus->int_st.val;
}

bool i2sWrite(uint8_t bus_num) 
{
    if (bus_num >= NEO_I2S_COUNT) 
    {
        return false;
    }

    // the second item (last of the two front silent items) is 
    // silent looping item
    lldesc_t* itemLoopBreaker = &I2S[bus_num].dma_items[1]; 
    lldesc_t* itemLoopNext = itemLoopBreaker + 1;

    // set to NOT loop on silent items
    itemLoopBreaker->qe.stqe_next = itemLoopNext;

    I2S[bus_num].is_sending_data = I2s_Is_Sending;

    return true;
}

#ifdef NEOPIXELBUS_I2S_DEBUG
void DumpI2sPrimary(const char* label, uint32_t val)
{
    printf("%s %08x\n", label, val);
}

void DumpI2sSecondary(const char* label, uint32_t val)
{
    printf("    %s %u\n", label, val);
}

void DumpI2s_sample_rate_conf(const char* label, i2s_dev_t* bus)
{
    typeof(bus->sample_rate_conf) val;

    val.val = bus->sample_rate_conf.val;

    DumpI2sPrimary(label, val.val);

    DumpI2sSecondary("tx_bck_div_num : ", val.tx_bck_div_num);
    DumpI2sSecondary("rx_bck_div_num : ", val.rx_bck_div_num);
    DumpI2sSecondary("tx_bits_mod : ", val.tx_bits_mod);
    DumpI2sSecondary("rx_bits_mod : ", val.rx_bits_mod);
}

void DumpI2s_conf1(const char* label, i2s_dev_t* bus)
{
    typeof(bus->conf1) val;
    val.val = bus->conf1.val;

    DumpI2sPrimary(label, val.val);

    DumpI2sSecondary("tx_pcm_conf : ", val.tx_pcm_conf);
    DumpI2sSecondary("tx_pcm_bypass : ", val.tx_pcm_bypass);
    DumpI2sSecondary("rx_pcm_conf : ", val.rx_pcm_conf);
    DumpI2sSecondary("rx_pcm_bypass : ", val.rx_pcm_bypass);
    DumpI2sSecondary("tx_stop_en : ", val.tx_stop_en);
    DumpI2sSecondary("tx_zeros_rm_en : ", val.tx_zeros_rm_en);
}

void DumpI2s_clkm_conf(const char* label, i2s_dev_t* bus)
{
    typeof(bus->clkm_conf) val;
    val.val = bus->clkm_conf.val;

    DumpI2sPrimary(label, val.val);

    DumpI2sSecondary("clkm_div_num : ", val.clkm_div_num);
    DumpI2sSecondary("clkm_div_b : ", val.clkm_div_b);
    DumpI2sSecondary("clkm_div_a : ", val.clkm_div_a);
    DumpI2sSecondary("clk_en : ", val.clk_en);
    DumpI2sSecondary("clka_en : ", val.clka_en);
}

void DumpI2s_lc_conf(const char* label, i2s_dev_t* bus)
{
    typeof(bus->lc_conf) val;
    val.val = bus->lc_conf.val;

    DumpI2sPrimary(label, val.val);

    DumpI2sSecondary("out_auto_wrback : ", val.out_auto_wrback);
    DumpI2sSecondary("out_no_restart_clr : ", val.out_no_restart_clr);
    DumpI2sSecondary("out_eof_mode : ", val.out_eof_mode);
    DumpI2sSecondary("outdscr_burst_en : ", val.outdscr_burst_en);
    DumpI2sSecondary("indscr_burst_en : ", val.indscr_burst_en);
    DumpI2sSecondary("out_data_burst_en : ", val.out_data_burst_en);
    DumpI2sSecondary("check_owner : ", val.check_owner);
    DumpI2sSecondary("mem_trans_en : ", val.mem_trans_en);
}

void DumpI2s_conf(const char* label, i2s_dev_t* bus)
{
    typeof(bus->conf) val;
    val.val = bus->conf.val;

    DumpI2sPrimary(label, val.val);

    DumpI2sSecondary("tx_slave_mod : ", val.tx_slave_mod);
    DumpI2sSecondary("rx_slave_mod : ", val.rx_slave_mod);
    DumpI2sSecondary("tx_right_first : ", val.tx_right_first);
    DumpI2sSecondary("rx_right_first : ", val.rx_right_first);
    DumpI2sSecondary("tx_msb_shift : ", val.tx_msb_shift);
    DumpI2sSecondary("rx_msb_shift : ", val.rx_msb_shift);
    DumpI2sSecondary("tx_short_sync : ", val.tx_short_sync);
    DumpI2sSecondary("rx_short_sync : ", val.rx_short_sync);
    DumpI2sSecondary("tx_mono : ", val.tx_mono);
    DumpI2sSecondary("rx_mono : ", val.rx_mono);
    DumpI2sSecondary("tx_msb_right : ", val.tx_msb_right);
    DumpI2sSecondary("rx_msb_right : ", val.rx_msb_right);
    DumpI2sSecondary("sig_loopback : ", val.sig_loopback);
}

void DumpI2s_fifo_conf(const char* label, i2s_dev_t* bus)
{
    typeof(bus->fifo_conf) val;
    val.val = bus->fifo_conf.val;

    DumpI2sPrimary(label, val.val);

    DumpI2sSecondary("rx_data_num : ", val.rx_data_num);
    DumpI2sSecondary("tx_data_num : ", val.tx_data_num);
    DumpI2sSecondary("dscr_en : ", val.dscr_en);
    DumpI2sSecondary("tx_fifo_mod : ", val.tx_fifo_mod);
    DumpI2sSecondary("rx_fifo_mod : ", val.rx_fifo_mod);
    DumpI2sSecondary("tx_fifo_mod_force_en : ", val.tx_fifo_mod_force_en);
    DumpI2sSecondary("rx_fifo_mod_force_en : ", val.rx_fifo_mod_force_en);
}

bool i2sDump(uint8_t bus_num)
{
    if (bus_num >= NEO_I2S_COUNT)
    {
        return false;
    }
    i2s_dev_t* i2s = I2S[bus_num].bus;

    DumpI2sPrimary("fifo_wr: ", i2s->fifo_wr);
    DumpI2sPrimary("fifo_rd: ", i2s->fifo_rd);
    DumpI2s_conf("conf: ", i2s);
    DumpI2sPrimary("int_raw: ", i2s->int_raw.val);
    DumpI2sPrimary("int_st: ", i2s->int_st.val);
    DumpI2sPrimary("int_ena: ", i2s->int_ena.val);
    DumpI2sPrimary("int_clr: ", i2s->int_clr.val);
    DumpI2sPrimary("timing: ", i2s->timing.val);
    DumpI2s_fifo_conf("fifo_conf: ", i2s);
    DumpI2sPrimary("rx_eof_num: ", i2s->rx_eof_num);
    DumpI2sPrimary("conf_single_data: ", i2s->conf_single_data);
    DumpI2sPrimary("conf_chan: ", i2s->conf_chan.val);
    DumpI2sPrimary("out_link: ", i2s->out_link.val);
    DumpI2sPrimary("in_link: ", i2s->in_link.val);
    DumpI2sPrimary("out_eof_des_addr: ", i2s->out_eof_des_addr);
    DumpI2sPrimary("in_eof_des_addr: ", i2s->in_eof_des_addr);
    DumpI2sPrimary("out_eof_bfr_des_addr: ", i2s->out_eof_bfr_des_addr);
    DumpI2sPrimary("ahb_test: ", i2s->ahb_test.val);

    DumpI2sPrimary("in_link_dscr: ", i2s->in_link_dscr);
    DumpI2sPrimary("in_link_dscr_bf0: ", i2s->in_link_dscr_bf0);
    DumpI2sPrimary("in_link_dscr_bf1: ", i2s->in_link_dscr_bf1);
    DumpI2sPrimary("out_link_dscr: ", i2s->out_link_dscr);
    DumpI2sPrimary("out_link_dscr_bf0: ", i2s->out_link_dscr_bf0);
    DumpI2sPrimary("out_link_dscr_bf1: ", i2s->out_link_dscr_bf1);

    DumpI2s_lc_conf("lc_conf: ", i2s);

    DumpI2sPrimary("out_fifo_push: ", i2s->out_fifo_push.val);
    DumpI2sPrimary("in_fifo_pop: ", i2s->in_fifo_pop.val);

    DumpI2sPrimary("lc_state0: ", i2s->lc_state0);
    DumpI2sPrimary("lc_state1: ", i2s->lc_state1);
    DumpI2sPrimary("lc_hung_conf: ", i2s->lc_hung_conf.val);

    DumpI2sPrimary("reserved_78: ", i2s->reserved_78);
    DumpI2sPrimary("reserved_7c: ", i2s->reserved_7c);

    DumpI2sPrimary("cvsd_conf0: ", i2s->cvsd_conf0.val);
    DumpI2sPrimary("cvsd_conf1: ", i2s->cvsd_conf1.val);
    DumpI2sPrimary("cvsd_conf2: ", i2s->cvsd_conf2.val);

    DumpI2sPrimary("plc_conf0: ", i2s->plc_conf0.val);
    DumpI2sPrimary("plc_conf1: ", i2s->plc_conf1.val);
    DumpI2sPrimary("plc_conf2: ", i2s->plc_conf2.val);

    DumpI2sPrimary("esco_conf0: ", i2s->esco_conf0.val);
    DumpI2sPrimary("sco_conf0: ", i2s->sco_conf0.val);

    DumpI2s_conf1("conf1: ", i2s);

    DumpI2sPrimary("pd_conf: ", i2s->pd_conf.val);
    DumpI2sPrimary("conf2: ", i2s->conf2.val);


    DumpI2s_clkm_conf("clkm_conf: ", i2s);
    DumpI2s_sample_rate_conf("sample_rate_conf: ", i2s);

    DumpI2sPrimary("pdm_conf: ", i2s->pdm_conf.val);
    DumpI2sPrimary("pdm_freq_conf: ", i2s->pdm_freq_conf.val);

    DumpI2sPrimary("state: ", i2s->state.val);

    /*
    uint32_t reserved_c0;
    uint32_t reserved_c4;
    uint32_t reserved_c8;
    uint32_t reserved_cc;
    uint32_t reserved_d0;
    uint32_t reserved_d4;
    uint32_t reserved_d8;
    uint32_t reserved_dc;
    uint32_t reserved_e0;
    uint32_t reserved_e4;
    uint32_t reserved_e8;
    uint32_t reserved_ec;
    uint32_t reserved_f0;
    uint32_t reserved_f4;
    uint32_t reserved_f8;
    */
    DumpI2sPrimary("date: ", i2s->date);

    return true;
}

bool i2sGetClks(uint8_t bus_num, 
        uint8_t* clkm_div_num, 
        uint8_t* clkm_div_b, 
        uint8_t* clkm_div_a)
{
    if (bus_num >= NEO_I2S_COUNT)
    {
        return false;
    }
    if (!clkm_div_num || !clkm_div_b || !clkm_div_a)
    {
        return false;
    }

    i2s_dev_t* i2s = I2S[bus_num].bus;

    typeof(i2s->clkm_conf) val;
    val.val = i2s->clkm_conf.val;

    *clkm_div_num = val.clkm_div_num;
    *clkm_div_b = val.clkm_div_b;
    *clkm_div_a = val.clkm_div_a;

    return true;
}
#endif

#endif //  !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32H2)

#endif // defined(ARDUINO_ARCH_ESP32)