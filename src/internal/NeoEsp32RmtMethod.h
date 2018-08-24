/*-------------------------------------------------------------------------
NeoPixel library helper functions for ESP32.

Written by Michael C. Miller.
 * RMT implementation provided by r1dd1ck@GitHub

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/NeoPixelBus)

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

#pragma once

#ifdef ARDUINO_ARCH_ESP32

#ifndef ICACHE_RAM_ATTR
#define ICACHE_RAM_ATTR IRAM_ATTR
#endif

extern "C"
{
#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/periph_ctrl.h>
#include <driver/rmt.h>
}

// -- RMT clock divider
#define CLK_DIV_RMT     8   // pulse granularity ->  4 =(50ns) 8 =(100ns)

// -- RMT cycles conversion
#define F_CLK_RMT       (   80000000L )
#define NS_PER_SEC      ( 1000000000L )
#define CYCLES_PER_SEC  ( F_CLK_RMT / CLK_DIV_RMT )
#define NS_PER_CYCLE    ( NS_PER_SEC / CYCLES_PER_SEC )
#define NS_TO_CYCLES(n) ( (n) / NS_PER_CYCLE )
#define CYCLES_TO_NS(n) ( (n) * NS_PER_CYCLE )
#define CYCLES_TO_US(n) ( CYCLES_TO_NS(n) / 1000L )

class NeoEsp32RmtSpeedWs2813
{
public:
    const static uint32_t T0H = 300;
    const static uint32_t T0L = 900;
    const static uint32_t T1H = 900;
    const static uint32_t T1L = 300;
    const static uint32_t ResetTimeUs = 300;
};

class NeoEsp32RmtSpeedSK6812
{
public:
    const static uint32_t T0H = 300;
    const static uint32_t T0L = 900;
    const static uint32_t T1H = 600;
    const static uint32_t T1L = 600;
    const static uint32_t ResetTimeUs = 80;
};

class NeoEsp32RmtSpeed800Kbps
{
public:
    const static uint32_t T0H = 300;
    const static uint32_t T0L = 900;
    const static uint32_t T1H = 900;
    const static uint32_t T1L = 300;
    const static uint32_t ResetTimeUs = 300;
};

class NeoEsp32RmtSpeed400Kbps
{
public:
    const static uint32_t T0H = 700;
    const static uint32_t T0L = 1800;
    const static uint32_t T1H = 1800;
    const static uint32_t T1L = 700;
    const static uint32_t ResetTimeUs = 300;
};


enum NeoRmtState
{
    NeoRmtState_Idle,
    NeoRmtState_Pending,
    NeoRmtState_Sending,
};


template<typename T_SPEED> class NeoEsp32RmtMethodBase
{
public:
    NeoEsp32RmtMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize) :
        _pin(pin)
    {
        _pixelsSize = pixelCount * elementSize;
        _bufferSize = _pixelsSize + 1; // +1 for RESET pulse

        _pixels = (uint8_t*)malloc(_pixelsSize);
        memset(_pixels, 0x00, _pixelsSize);

        _rmtBuffer = (uint8_t*)malloc(_bufferSize);
        memset(_rmtBuffer, 0x00, _bufferSize);
    }

    ~NeoEsp32RmtMethodBase()
    {
        StopRmt();
        free(_pixels);
        free(_rmtBuffer);
    }

    bool IsReadyToUpdate()
    {
        // TODO: use "rmt_tx_end_callback" to set channel BUSY->IDLE state (?)
        updateState();
        return (_rmtState == NeoRmtState_Idle);
    }

    void Initialize()
    {
        ESP_ERROR_CHECK(get_channel_free(&_channel));

        //Serial.printf("[NeoPixelBus] Initializing RMT on channel:%u, pin:%u\n", _channel, _pin);

        rmt_config_t config;
        config.rmt_mode = RMT_MODE_TX;
        config.channel = (rmt_channel_t)_channel;
        config.gpio_num = (gpio_num_t)_pin;
        config.mem_block_num = 1;
        config.clk_div = CLK_DIV_RMT;
        config.tx_config.loop_en = false;
        config.tx_config.idle_output_en = true;
        config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
        config.tx_config.carrier_en = false;
        config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;

        ESP_ERROR_CHECK(rmt_config(&config));
        ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
        ESP_ERROR_CHECK(rmt_translator_init(config.channel, u8_to_rmt));

        _rmtState = NeoRmtState_Idle;
    }

    void ICACHE_RAM_ATTR Update()
    {
      // wait for not actively sending data
        while (!IsReadyToUpdate())
        {
            yield();
        }

        FillBuffers();
        ESP_ERROR_CHECK(rmt_write_sample(_channel, _rmtBuffer, _bufferSize, false));

        _rmtState = NeoRmtState_Sending;
    }

    uint8_t* getPixels() const
    {
        return _pixels;
    }

    size_t getPixelsSize() const
    {
        return _pixelsSize;
    }

private:
    uint8_t* _pixels;       // holds LED color values
    uint8_t* _rmtBuffer;    // holds a copy of '_pixels' (+1 for RESET), cloned on Update()

    size_t  _pixelsSize;    // size of '_pixels' array
    size_t  _bufferSize;    // size of '_rmtBuffer' array
    uint8_t _pin;           // output pin number

    rmt_channel_t _channel; // RMT channel

    static const uint32_t _bit0 = (NS_TO_CYCLES(T_SPEED::T0H) | (1 << 15) | (NS_TO_CYCLES(T_SPEED::T0L) << 16));  // logical 0
    static const uint32_t _bit1 = (NS_TO_CYCLES(T_SPEED::T1H) | (1 << 15) | (NS_TO_CYCLES(T_SPEED::T1L) << 16));  // logical 1
    static const uint32_t  _rst = (NS_TO_CYCLES(T_SPEED::ResetTimeUs*1000L));                                     // reset

    volatile NeoRmtState _rmtState;


    // translate routine (called from RMT ISR) -> sample_to_rmt_fn
    static void ICACHE_RAM_ATTR u8_to_rmt(const void* src, rmt_item32_t* dest, size_t src_size, size_t wanted_num, size_t* translated_size, size_t* item_num)
    {
        if (src == NULL || dest == NULL)
        {
            *translated_size = 0;
            *item_num = 0;
            return;
        }

        size_t size = 0;
        size_t num = 0;
        uint8_t mask;

        uint8_t *psrc = (uint8_t *)src;
        rmt_item32_t* pdest = dest;

        bool last = !(src_size > (wanted_num >> 3));
        if (last)
        {
        	src_size--;
        }

        while (size < src_size && num < wanted_num)
        {
            for (mask = 0x80; mask != 0; mask >>= 1)
            {
                if (*psrc & mask)
                {
                    pdest->val = _bit1;
                }
                else
                {
                    pdest->val = _bit0;
                }
                num++;
                pdest++;
            }
            size++;
            psrc++;
        }

        if (last)
        {
          pdest->val = _rst;
          num++;
          size++;
        }

        *translated_size = size;
        *item_num = num;
    }

    void FillBuffers()
    {
        memcpy(_rmtBuffer, _pixels, _pixelsSize);
    }

    void updateState()
    {
        uint32_t status;
        ESP_ERROR_CHECK(rmt_get_status(_channel, &status));
        _rmtState = (status & 0x1000000) ? NeoRmtState_Sending : NeoRmtState_Idle;
    }

    void StopRmt()
    {
        ESP_ERROR_CHECK(rmt_driver_uninstall(_channel));
    }

    esp_err_t get_channel_free(rmt_channel_t* channel)
    {
        esp_err_t err;
        rmt_item32_t itm;

        esp_log_level_set("rmt", ESP_LOG_NONE);
        for (uint8_t ch = 0; ch < RMT_CHANNEL_MAX; ch++)
        {
            err = rmt_write_items((rmt_channel_t)ch, &itm, 0, false);
            if (err == ESP_FAIL)
            {
                *channel = (rmt_channel_t)ch;
                esp_log_level_set("rmt", ESP_LOG_WARN);
                return ESP_OK;
            }
        }
        esp_log_level_set("rmt", ESP_LOG_WARN);
        return ESP_FAIL;
    }

};

typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedWs2813> NeoEsp32RmtWs2813Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedSK6812> NeoEsp32RmtSK6812Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeed800Kbps> NeoEsp32Rmt800KbpsMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeed400Kbps> NeoEsp32Rmt400KbpsMethod;

/* define RMT method as the default method for ESP32 */
typedef NeoEsp32RmtWs2813Method NeoWs2813Method;
typedef NeoEsp32RmtSK6812Method NeoSK6812Method;
typedef NeoEsp32Rmt800KbpsMethod Neo800KbpsMethod;
typedef NeoEsp32Rmt400KbpsMethod Neo400KbpsMethod;

#endif