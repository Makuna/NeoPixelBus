#pragma once

/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp32.

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

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)

extern "C"
{
#include <driver/periph_ctrl.h>
#include <esp_private/gdma.h>
#include <esp_rom_gpio.h>
//#include <esp_rom_lldesc.h>
#include <hal/dma_types.h>
#include <hal/gpio_hal.h>
#include <hal/lcd_ll.h>
#include <soc/lcd_cam_struct.h>
#include "FractionClk.h"
}

//
// true size of mux channel, 8 bit
// 3 step cadence, so pulses are 1/3 and 2/3 of pulse width
//
class NeoEspLcdMuxBusSize8Bit
{
public:
    NeoEspLcdMuxBusSize8Bit() {};

    const static size_t MuxBusDataSize = 1;
    const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches endcoding


    // by using a 3 step cadence, the dma data can't be updated with a single OR operation as
    //    its value resides across a non-uint16_t aligned 3 element type, so it requires two seperate OR
    //    operations to update a single pixel bit, the last element can be skipped as its always 0
    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
        uint8_t* pDma = dmaBuffer;
        const uint8_t* pValue = data;
        const uint8_t* pEnd = pValue + sizeData;
        const uint8_t muxBit = 0x1 << muxId;

        while (pValue < pEnd)
        {
            uint8_t value = *(pValue++);

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                // first cadence step init to 1
                *(pDma++) |= muxBit;

                // second cadence step set based on bit
                if (value & 0x80)
                {
                    *(pDma) |= muxBit;
                }
                pDma++;

                // last cadence step already init to 0, skip it
                pDma++;

                // Next
                value <<= 1;
            }
        }
    }
};

//
// true size of mux channel, 16 bit
// 3 step cadence, so pulses are 1/3 and 2/3 of pulse width
//
class NeoEspLcdMuxBusSize16Bit
{
public:
    NeoEspLcdMuxBusSize16Bit() {};

    const static size_t MuxBusDataSize = 2;
    const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches endcoding

    // by using a 3 step cadence, the dma data can't be updated with a single OR operation as
    //    its value resides across a non-uint32_t aligned 3 element type, so it requires two seperate OR
    //    operations to update a single pixel bit, the last element can be skipped as its always 0
    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
        uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
        const uint8_t* pValue = data;
        const uint8_t* pEnd = pValue + sizeData;
        const uint16_t muxBit = 0x1 << muxId;

        while (pValue < pEnd)
        {
            uint8_t value = *(pValue++);

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                // first cadence step init to 1
                *(pDma++) |= muxBit;

                // second cadence step set based on bit
                if (value & 0x80) 
                {
                    *(pDma) |= muxBit;
                }
                pDma++;

                // last cadence step already init to 0, skip it
                pDma++;

                // Next
                value <<= 1;
            }
        }
    }
};

//
// tracks mux channels used and if updated
// 
// T_FLAG - type used to store bit flags, UINT8_t for 8 channels, UINT16_t for 16
// T_MUXSIZE - true size of mux channel = NeoEspLcdMuxBusSize8Bit or NeoEspLcdMuxBusSize16Bit
//
template<typename T_FLAG, typename T_MUXSIZE> 
class NeoEspLcdMuxMap : public T_MUXSIZE
{
public:
    const static uint8_t InvalidMuxId = -1;
    const static size_t BusMaxCount = sizeof(T_FLAG) * 8;

    size_t MaxBusDataSize; // max size of stream data from any single mux bus
    T_FLAG UpdateMap;     // bitmap flags of mux buses to track update state
    T_FLAG UpdateMapMask; // mask to used bits in s_UpdateMap
    T_FLAG BusCount;      // count of mux buses

    // as a static instance, all members get initialized to zero
    // and the constructor is called at inconsistent time to other globals
    // so its not useful to have or rely on, 
    // but without it presence they get zeroed far too late
    NeoEspLcdMuxMap() 
        //:
    {
    }

    uint8_t RegisterNewMuxBus(const size_t dataSize)
    {
        // find first available bus id
        uint8_t muxId = 0;
        while (muxId < BusMaxCount)
        {
            T_FLAG muxIdField = (1 << muxId);
            if ((UpdateMapMask & muxIdField) == 0)
            {
                // complete registration
                BusCount++;
                UpdateMapMask |= muxIdField;
                if (dataSize > MaxBusDataSize)
                {
                    MaxBusDataSize = dataSize;
                }
                break;
            }
            muxId++;
        }
        if (muxId == BusMaxCount)
        {
            log_e("exceded channel limit of %u on bus", BusMaxCount);
        }
        return muxId;
    }


    bool DeregisterMuxBus(uint8_t muxId)
    {
        T_FLAG muxIdField = (1 << muxId);
        if (UpdateMapMask & muxIdField)
        {
            // complete deregistration
            BusCount--;
            UpdateMapMask &= ~muxIdField;
            if (UpdateMapMask == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool IsAllMuxBusesUpdated()
    {
        return (UpdateMap == UpdateMapMask);
    }

    bool IsNoMuxBusesUpdate()
    {
        return (UpdateMap == 0);
    }

    void MarkMuxBusUpdated(uint8_t muxId)
    {
        UpdateMap |= (1 << muxId);
    }

    void ResetMuxBusesUpdated()
    {
        UpdateMap = 0;
    }

    void Reset()
    {
        MaxBusDataSize = 0;
        UpdateMap = 0;
        UpdateMapMask = 0;
        BusCount = 0;
    }
};

// REVIEW:  Is this actually in IRAM, old compiler bug ignored function attributes in header files
static IRAM_ATTR bool dma_callback(gdma_channel_handle_t dma_chan,
                                   gdma_event_data_t *event_data,
                                   void *user_data) 
{
//    esp_rom_delay_us(5); This was to handle the that the IRQ is fired on the last DMA block starting to transfer rather than when its done
    LCD_CAM.lcd_user.lcd_start = 0;
    return true;
}

//
// Implementation of a Single Buffered version of a LcdContext
// Manages the underlying I2S details including the buffer
// This creates only a actively sending back buffer, 
// Note that the back buffer must be DMA memory, a limited resource
// 
// T_MUXMAP - NeoEspLcdMuxMap - tracking class for mux state
//
template<typename T_MUXMAP> 
class NeoEspLcdMonoBuffContext 
{
private:
    gdma_channel_handle_t _dmaChannel;
    dma_descriptor_t* _dmaItems; // holds the DMA description table

public:
    size_t LcdBufferSize; // total size of LcdBuffer
    uint8_t* LcdBuffer;   // holds the DMA buffer that is referenced by _dmaItems
    T_MUXMAP MuxMap;

    // as a static instance, all members get initialized to zero
    // and the constructor is called at inconsistent time to other globals
    // so its not useful to have or rely on, 
    // but without it presence they get zeroed far too late
    NeoEspLcdMonoBuffContext()
        //:
    {
    }

    void Construct(uint16_t nsBitSendTime)
    {
        // construct only once on first time called
        if (_dmaItems == nullptr)
        {
            // MuxMap.MaxBusDataSize = max size in bytes of a single channel
            // DmaBitsPerPixelBit = how many dma bits/byte are needed for each source (pixel) bit/byte
            // T_MUXMAP::MuxBusDataSize = the true size of data for selected mux mode (not exposed size as i2s0 only supports 16bit mode)
            const size_t DmaBytesPerPixelByte = (8 * T_MUXMAP::DmaBitsPerPixelBit * T_MUXMAP::MuxBusDataSize);
            LcdBufferSize = DmaBytesPerPixelByte * MuxMap.MaxBusDataSize;

            // must have a 4 byte aligned buffer for DMA
            uint32_t alignment = LcdBufferSize % 4;
            if (alignment)
            {
                LcdBufferSize += 4 - alignment;
            }

            size_t dmaBlockCount = (LcdBufferSize + DMA_DESCRIPTOR_BUFFER_MAX_SIZE - 1) / DMA_DESCRIPTOR_BUFFER_MAX_SIZE;
            dmaBlockCount++; // extra one at the end for trigger IRQ only
            size_t dmaBlockSize = dmaBlockCount * sizeof(dma_descriptor_t);

            _dmaItems = static_cast<dma_descriptor_t*>(heap_caps_malloc(dmaBlockSize, MALLOC_CAP_DMA));
            if (_dmaItems == nullptr)
            {
                log_e("LCD Dma Table memory allocation failure (size %u)",
                    dmaBlockSize);
            }
            // required to init to zero as settings these below only resets some fields
            memset(_dmaItems, 0x00, dmaBlockSize); 

            LcdBuffer = static_cast<uint8_t*>(heap_caps_malloc(LcdBufferSize, MALLOC_CAP_DMA));
            if (LcdBuffer == nullptr)
            {
                log_e("LCD Dma Buffer memory allocation failure (size %u)",
                    LcdBufferSize);
            }
            memset(LcdBuffer, 0x00, LcdBufferSize);

            // init dma descriptor blocks
            // 
            dma_descriptor_t* itemFirst = _dmaItems;
            dma_descriptor_t* item = itemFirst;
            dma_descriptor_t* itemNext = item + 1;

            size_t dataLeft = LcdBufferSize - DmaBytesPerPixelByte; // last one for IRQ use
            uint8_t* pos = LcdBuffer;

            // init blocks with available data
            //
            while (dataLeft)
            {
                // track how much of data goes into this descriptor block
                size_t blockSize = dataLeft;
                if (blockSize >= DMA_DESCRIPTOR_BUFFER_MAX_SIZE)
                {
                    blockSize = DMA_DESCRIPTOR_BUFFER_MAX_SIZE - 1;
                }
                dataLeft -= blockSize;

                // init a DMA descriptor item
                item->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
                item->dw0.suc_eof = 0;
                item->next = itemNext;
                item->dw0.size = blockSize;
                item->dw0.length = blockSize;
                item->buffer = pos;

                pos += blockSize;

                item = itemNext;
                itemNext++;
            }

            // last dma descriptor item is EOF to manage send state using EOF ISR
            item->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
            item->dw0.suc_eof = 1;
            item->next = NULL;
            item->dw0.size = 0;
            item->dw0.length = 0;
            item->buffer = NULL; 

            // Configure LCD Peripheral
            // 
            
            // LCD_CAM isn't enabled by default -- MUST begin with this:
            periph_module_enable(PERIPH_LCD_CAM_MODULE);
            periph_module_reset(PERIPH_LCD_CAM_MODULE);

            // Reset LCD bus
            LCD_CAM.lcd_user.lcd_reset = 1;
            esp_rom_delay_us(100);

            // calc needed clock scaler values from bit send time
            //
            double clkm_div = (double)nsBitSendTime / T_MUXMAP::DmaBitsPerPixelBit / 1000.0 * 240.0; // PLL 240Mhz
            if (clkm_div > LCD_LL_CLK_FRAC_DIV_N_MAX)
            {
                log_e("rate is too low");
                return;
            }
            else if (clkm_div < 2.0)
            {
                log_e("rate is too fast, clkmdiv = %f (%u)",
                    clkm_div,
                    nsBitSendTime);
                return;
            }

            // calc integer and franctional for more precise timing
            uint8_t clkm_div_Integer = clkm_div;
            double clkm_Fraction = (clkm_div - clkm_div_Integer);
            uint8_t divB = 0;
            uint8_t divA = 0;

            UnitDecimalToFractionClks(&divB, &divA, clkm_Fraction, 0.000001);

            //Serial.print("Clk Div ");
            //Serial.print(clkm_div);
            //Serial.print(" = ");

            //Serial.print(clkm_div_Integer);
            //Serial.print(" ");
            //Serial.print(divB);
            //Serial.print("/");
            //Serial.println(divA);

            // Configure LCD clock
            LCD_CAM.lcd_clock.clk_en = 1;             // Enable clock
            LCD_CAM.lcd_clock.lcd_clk_sel = 2;        // PLL240M source
            LCD_CAM.lcd_clock.lcd_clkm_div_a = divA;  // scale fractional denomenator,
            LCD_CAM.lcd_clock.lcd_clkm_div_b = divB;  // scale fractional numerator
            LCD_CAM.lcd_clock.lcd_clkm_div_num = clkm_div_Integer;  // scale integer (240Mhz clock)
            LCD_CAM.lcd_clock.lcd_ck_out_edge = 0;    // PCLK low in 1st half cycle
            LCD_CAM.lcd_clock.lcd_ck_idle_edge = 0;   // PCLK low idle
            LCD_CAM.lcd_clock.lcd_clk_equ_sysclk = 1; // PCLK = CLK (ignore CLKCNT_N)

            // Configure frame format
            LCD_CAM.lcd_ctrl.lcd_rgb_mode_en = 0;    // i8080 mode (not RGB)
            LCD_CAM.lcd_rgb_yuv.lcd_conv_bypass = 0; // Disable RGB/YUV converter
            LCD_CAM.lcd_misc.lcd_next_frame_en = 0;  // Do NOT auto-frame
            LCD_CAM.lcd_data_dout_mode.val = 0;      // No data delays
            LCD_CAM.lcd_user.lcd_always_out_en = 1;  // Enable 'always out' mode
            LCD_CAM.lcd_user.lcd_8bits_order = 0;    // Do not swap bytes
            LCD_CAM.lcd_user.lcd_bit_order = 0;      // Do not reverse bit order
            LCD_CAM.lcd_user.lcd_2byte_en = (T_MUXMAP::MuxBusDataSize == 2);
            LCD_CAM.lcd_user.lcd_dummy = 1;          // Dummy phase(s) @ LCD start
            LCD_CAM.lcd_user.lcd_dummy_cyclelen = 0; // 1 dummy phase
            LCD_CAM.lcd_user.lcd_cmd = 0;            // No command at LCD start
            // Dummy phase(s) MUST be enabled for DMA to trigger reliably.

            // Alloc DMA channel & connect it to LCD periph
            gdma_channel_alloc_config_t dma_chan_config = {
                .sibling_chan = NULL,
                .direction = GDMA_CHANNEL_DIRECTION_TX,
                .flags = {.reserve_sibling = 0}};
            gdma_new_channel(&dma_chan_config, &_dmaChannel);
            gdma_connect(_dmaChannel, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
            gdma_strategy_config_t strategy_config = {.owner_check = false,
                                                        .auto_update_desc = false};
            gdma_apply_strategy(_dmaChannel, &strategy_config);

            // Enable DMA transfer callback
            gdma_tx_event_callbacks_t tx_cbs = {.on_trans_eof = dma_callback};
            gdma_register_tx_event_callbacks(_dmaChannel, &tx_cbs, NULL);
        }
    }

    void Destruct()
    {
        if (_dmaItems == nullptr)
        {
            return;
        }

        periph_module_disable(PERIPH_LCD_CAM_MODULE);
        periph_module_reset(PERIPH_LCD_CAM_MODULE);

        gdma_reset(_dmaChannel);

        heap_caps_free(LcdBuffer);
        heap_caps_free(_dmaItems);

        LcdBufferSize = 0;
        _dmaItems = nullptr;
        LcdBuffer = nullptr;

        MuxMap.Reset();
    }

    void StartWrite()
    {
        if (MuxMap.IsAllMuxBusesUpdated())
        {
            MuxMap.ResetMuxBusesUpdated();
            
            gdma_reset(_dmaChannel);
            LCD_CAM.lcd_user.lcd_dout = 1;
            LCD_CAM.lcd_user.lcd_update = 1;
            LCD_CAM.lcd_misc.lcd_afifo_reset = 1;

            gdma_start(_dmaChannel, (intptr_t)&_dmaItems[0]);
            esp_rom_delay_us(1);
            LCD_CAM.lcd_user.lcd_start = 1; 
        }
    }

    void FillBuffers(const uint8_t* data, 
            size_t sizeData, 
            uint8_t muxId)
    {
        // wait for not actively sending data
        while (LCD_CAM.lcd_user.lcd_start)
        {
            yield();
        }


        // to keep the inner loops for EncodeIntoDma smaller
        // they will just OR in their values
        // so the buffer must be cleared first
        if (MuxMap.IsNoMuxBusesUpdate())
        {
            // clear all the data in preperation for each mux channel to update their bit
            memset(LcdBuffer, 0x00, LcdBufferSize);
        }

        MuxMap.EncodeIntoDma(LcdBuffer,
            data,
            sizeData,
            muxId);

        MuxMap.MarkMuxBusUpdated(muxId);
    }
};

//
// Implementation of the low level interface into lcd mux bus
// 
// T_BUSCONTEXT - the context to use, currently only NeoEspLcdDblBuffContext but there is
//      a plan to provide one that doesn't implement the front buffer but would be less
//      async as it would have to wait until the last frame was completely sent before
//      updating and new data
// T_BUS - the bus id, NeoEsp32LcdBusZero, NeoEsp32LcdBusOne
//
template<typename T_BUSCONTEXT> 
class NeoEsp32LcdMuxBus
{
public:    
    NeoEsp32LcdMuxBus() :
        _muxId(s_context.MuxMap.InvalidMuxId)
    {
    }

    void RegisterNewMuxBus(size_t dataSize)
    {
        _muxId = s_context.MuxMap.RegisterNewMuxBus(dataSize);
    }

    void Initialize(uint8_t pin, uint16_t nsBitSendTime, bool invert)
    {
        s_context.Construct(nsBitSendTime);
        
        uint8_t muxIdx = LCD_DATA_OUT0_IDX + _muxId;
        esp_rom_gpio_connect_out_signal(pin, muxIdx, invert, false);
        gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
        gpio_set_drive_capability((gpio_num_t)pin, (gpio_drive_cap_t)3);
    }

    void DeregisterMuxBus(uint8_t pin)
    {
        if (s_context.MuxMap.DeregisterMuxBus(_muxId))
        {
            s_context.Destruct();
        }

        // disconnect muxed pin
        gpio_matrix_out(pin, SIG_GPIO_OUT_IDX, false, false);
        pinMode(pin, INPUT);

        _muxId = s_context.MuxMap.InvalidMuxId;
    }

    void StartWrite()
    {
        s_context.StartWrite();
    }

    bool IsWriteDone() const
    {
        bool busy = LCD_CAM.lcd_user.lcd_start;
        return !busy;
    }

    void FillBuffers(const uint8_t* data, size_t sizeData)
    {
        s_context.FillBuffers(data, sizeData, _muxId);
    }

    void MarkUpdated()
    {
        s_context.MuxMap.MarkMuxBusUpdated(_muxId);
    }

private:
    static T_BUSCONTEXT s_context;
    uint8_t _muxId; 
};

template<typename T_BUSCONTEXT> T_BUSCONTEXT NeoEsp32LcdMuxBus<T_BUSCONTEXT>::s_context = T_BUSCONTEXT();

//
// wrapping layer of the lcd mux bus as a NeoMethod
// 
// T_SPEED - NeoBitsSpeed* (ex NeoBitsSpeedWs2812x) used to define output signal form
// T_BUS - NeoEsp32LcdMuxBus, the bus to use
// T_INVERT - NeoBitsNotInverted or NeoBitsInverted, will invert output signal
//
template<typename T_SPEED, typename T_BUS, typename T_INVERT>
class NeoEsp32LcdXMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32LcdXMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin),
        _bus()
    {
        size_t numResetBytes = T_SPEED::ResetTimeUs / T_SPEED::ByteSendTimeUs(T_SPEED::BitSendTimeNs);
        _bus.RegisterNewMuxBus(_sizeData + numResetBytes);        
    }

    ~NeoEsp32LcdXMethodBase()
    {
        while (!_bus.IsWriteDone())
        {
            yield();
        }

        _bus.DeregisterMuxBus(_pin);

        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        return _bus.IsWriteDone();
    }

    void Initialize()
    {
        _bus.Initialize(_pin, T_SPEED::BitSendTimeNs, T_INVERT::Inverted);

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        if (_data == nullptr)
        {
            log_e("front buffer memory allocation failure");
        }
        // data cleared later in Begin()
    }

    void Update(bool)
    {
        _bus.FillBuffers(_data, _sizeData);

        _bus.StartWrite(); // only triggers actual write after all mux busses have updated
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called even if no changes to method buffer
        // as edit buffer is always cleared and then copied to send buffer and all
        // mux bus needs to included
        return true;
    }

    bool SwapBuffers()
    {
        return false;
    }

    uint8_t* getData() const
    {
        return _data;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    const size_t  _sizeData;    // Size of '_data' buffer 
    const uint8_t _pin;         // output pin number

    T_BUS _bus;          // holds instance for mux bus support
    uint8_t* _data;      // Holds LED color values
};



typedef NeoEsp32LcdMuxBus<NeoEspLcdMonoBuffContext<NeoEspLcdMuxMap<uint8_t, NeoEspLcdMuxBusSize8Bit>>> NeoEsp32LcdMux8Bus;
typedef NeoEsp32LcdMuxBus<NeoEspLcdMonoBuffContext<NeoEspLcdMuxMap<uint16_t, NeoEspLcdMuxBusSize16Bit>>> NeoEsp32LcdMux16Bus;


//--------------------------------------------------------

typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8Ws2812xMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedWs2805,   NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8Ws2805Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedSk6812,   NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8Sk6812Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1814,   NeoEsp32LcdMux8Bus, NeoBitsInverted> NeoEsp32LcdX8Tm1814Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1914,   NeoEsp32LcdMux8Bus, NeoBitsInverted> NeoEsp32LcdX8Tm1829Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1829,   NeoEsp32LcdMux8Bus, NeoBitsInverted> NeoEsp32LcdX8Tm1914Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeed800Kbps,  NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8800KbpsMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeed400Kbps,  NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8400KbpsMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedApa106,   NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8Apa106Method;

typedef NeoEsp32LcdX8Ws2805Method NeoEsp32LcdX8Ws2814Method;
typedef NeoEsp32LcdX8Ws2812xMethod NeoEsp32LcdX8Ws2813Method;
typedef NeoEsp32LcdX8Ws2812xMethod NeoEsp32LcdX8Ws2812dMethod;
typedef NeoEsp32LcdX8Ws2812xMethod NeoEsp32LcdX8Ws2811Method;
typedef NeoEsp32LcdX8Ws2812xMethod NeoEsp32LcdX8Ws2816Method;
typedef NeoEsp32LcdX8800KbpsMethod NeoEsp32LcdX8Ws2812Method;
typedef NeoEsp32LcdX8Sk6812Method  NeoEsp32LcdX8Lc8812Method;

typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16Ws2812xMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedWs2805,   NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16Ws2805Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedSk6812,   NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16Sk6812Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1814,   NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16Tm1814Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1914,   NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16Tm1829Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1829,   NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16Tm1914Method;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeed800Kbps,  NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16800KbpsMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeed400Kbps,  NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16400KbpsMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedApa106,   NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16Apa106Method;

typedef NeoEsp32LcdX16Ws2805Method NeoEsp32LcdX16Ws2814Method;
typedef NeoEsp32LcdX16Ws2812xMethod NeoEsp32LcdX16Ws2813Method;
typedef NeoEsp32LcdX16Ws2812xMethod NeoEsp32LcdX16Ws2812dMethod;
typedef NeoEsp32LcdX16Ws2812xMethod NeoEsp32LcdX16Ws2811Method;
typedef NeoEsp32LcdX16Ws2812xMethod NeoEsp32LcdX16Ws2816Method;
typedef NeoEsp32LcdX16800KbpsMethod NeoEsp32LcdX16Ws2812Method;
typedef NeoEsp32LcdX16Sk6812Method  NeoEsp32LcdX16Lc8812Method;


//--------------------------------------------------------
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32LcdMux8Bus, NeoBitsInverted>    NeoEsp32LcdX8Ws2812xInvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedWs2805,   NeoEsp32LcdMux8Bus, NeoBitsInverted>    NeoEsp32LcdX8Ws2805InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedSk6812,   NeoEsp32LcdMux8Bus, NeoBitsInverted>    NeoEsp32LcdX8Sk6812InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1814,   NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8Tm1814InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1914,   NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8Tm1829InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1829,   NeoEsp32LcdMux8Bus, NeoBitsNotInverted> NeoEsp32LcdX8Tm1914InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeed800Kbps,  NeoEsp32LcdMux8Bus, NeoBitsInverted>    NeoEsp32LcdX8800KbpsInvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeed400Kbps,  NeoEsp32LcdMux8Bus, NeoBitsInverted>    NeoEsp32LcdX8400KbpsInvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedApa106,   NeoEsp32LcdMux8Bus, NeoBitsInverted>    NeoEsp32LcdX8Apa106InvertedMethod;

typedef NeoEsp32LcdX8Ws2805InvertedMethod  NeoEsp32LcdX8Ws2814InvertedMethod;
typedef NeoEsp32LcdX8Ws2812xInvertedMethod NeoEsp32LcdX8Ws2813InvertedMethod;
typedef NeoEsp32LcdX8Ws2812xInvertedMethod NeoEsp32LcdX8Ws2812dInvertedMethod;
typedef NeoEsp32LcdX8Ws2812xInvertedMethod NeoEsp32LcdX8Ws2811InvertedMethod;
typedef NeoEsp32LcdX8Ws2812xInvertedMethod NeoEsp32LcdX8Ws2816InvertedMethod;
typedef NeoEsp32LcdX8800KbpsInvertedMethod NeoEsp32LcdX8Ws2812InvertedMethod;
typedef NeoEsp32LcdX8Sk6812InvertedMethod  NeoEsp32LcdX8Lc8812InvertedMethod;

typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16Ws2812xInvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedWs2805,   NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16Ws2805InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedSk6812,   NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16Sk6812InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1814,   NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16Tm1814InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1914,   NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16Tm1829InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedTm1829,   NeoEsp32LcdMux16Bus, NeoBitsNotInverted> NeoEsp32LcdX16Tm1914InvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeed800Kbps,  NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16800KbpsInvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeed400Kbps,  NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16400KbpsInvertedMethod;
typedef NeoEsp32LcdXMethodBase<NeoBitsSpeedApa106,   NeoEsp32LcdMux16Bus, NeoBitsInverted> NeoEsp32LcdX16Apa106InvertedMethod;

typedef NeoEsp32LcdX16Ws2805InvertedMethod  NeoEsp32LcdX16Ws2814InvertedMethod;
typedef NeoEsp32LcdX16Ws2812xInvertedMethod NeoEsp32LcdX16Ws2813InvertedMethod;
typedef NeoEsp32LcdX16Ws2812xInvertedMethod NeoEsp32LcdX16Ws2812dInvertedMethod;
typedef NeoEsp32LcdX16Ws2812xInvertedMethod NeoEsp32LcdX16Ws2811InvertedMethod;
typedef NeoEsp32LcdX16Ws2812xInvertedMethod NeoEsp32LcdX16Ws2816InvertedMethod;
typedef NeoEsp32LcdX16800KbpsInvertedMethod NeoEsp32LcdX16Ws2812InvertedMethod;
typedef NeoEsp32LcdX16Sk6812InvertedMethod  NeoEsp32LcdX16Lc8812InvertedMethod;

#endif // defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
