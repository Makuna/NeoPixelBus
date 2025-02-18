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

#pragma once

#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C2)

#include <Arduino.h>
#include "NeoEsp32RmtSpeed.h"

extern void AddLog(uint32_t loglevel, PGM_P formatP, ...);

extern "C"
{
#include <rom/gpio.h>
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_check.h"
}

struct led_strip_encoder_config_t
{
    uint32_t resolution; /*!< Encoder resolution, in Hz */
};

struct rmt_led_strip_encoder_t
{
    rmt_encoder_t base;
    rmt_encoder_t* bytes_encoder;
    rmt_encoder_t* copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
};

#define NEOPIXELBUS_RMT_INT_FLAGS (ESP_INTR_FLAG_LOWMED)


template<typename T_SPEED, typename T_INVERTED = NeoEsp32RmtNotInverted> class NeoEsp32RmtMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32RmtMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize, NeoBusChannel channel = NeoBusChannel_0) :
        _sizeData(pixelCount* elementSize + settingsSize),
        _pin(pin),
        _channel(NULL)
    {
        construct();
    }

    ~NeoEsp32RmtMethodBase()
    {
        // wait until the last send finishes before destructing everything
        // arbitrary time out of 10 seconds

        ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_tx_wait_all_done(_channel, 10000 / portTICK_PERIOD_MS));
        ESP_ERROR_CHECK(rmt_disable(_channel)); 
        ESP_ERROR_CHECK(rmt_del_channel(_channel));

        gpio_matrix_out(_pin, 0x100, false, false);
        pinMode(_pin, INPUT);

        free(_dataEditing);
        free(_dataSending);
    }


    bool IsReadyToUpdate() const
    {
        return (ESP_OK == rmt_tx_wait_all_done(_channel, 0));
    }

    void Initialize()
    {
        esp_err_t ret = ESP_OK;
        rmt_tx_channel_config_t config = {};
        config.clk_src = RMT_CLK_SRC_DEFAULT;
        config.gpio_num = static_cast<gpio_num_t>(_pin);
        config.mem_block_symbols = 192;         // memory block size, 64 * 4 = 256 Bytes
        config.resolution_hz = T_SPEED::RmtTicksPerSecond; // 1 MHz tick resolution, i.e., 1 tick = 1 µs
        config.trans_queue_depth = 4;           // set the number of transactions that can pend in the background
        config.flags.invert_out = T_INVERTED::Inverted;  // do not invert output signal
        config.flags.with_dma = false;          // do not need DMA backend

        ret += rmt_new_tx_channel(&config, &_channel);
        led_strip_encoder_config_t encoder_config = {};
        encoder_config.resolution = T_SPEED::RmtTicksPerSecond;

        _tx_config.loop_count = 0; //no loop

        ret += rmt_new_led_strip_encoder(&encoder_config, &_led_encoder, T_SPEED::RmtBit0, T_SPEED::RmtBit1);

        // ESP_LOGI(TAG, "Enable RMT TX channel");
        ret += rmt_enable(_channel);
        // if (ret) {
        //     AddLog(2,"RMT: initialized with error code: %u on pin: %u",ret, _pin);
        // }
    }

    void Update(bool maintainBufferConsistency)
    {
        // AddLog(2,"..");
        // wait for not actively sending data
        // this will time out at 10 seconds, an arbitrarily long period of time
        // and do nothing if this happens

        if (ESP_OK == ESP_ERROR_CHECK_WITHOUT_ABORT(rmt_tx_wait_all_done(_channel, 10000 / portTICK_PERIOD_MS)))
        {
            // AddLog(2,"__ %u", _sizeData);
            // now start the RMT transmit with the editing buffer before we swap
            // esp_err_t ret = 
            rmt_transmit(_channel, _led_encoder, _dataEditing, _sizeData, &_tx_config); // 3 for _sizeData
            // AddLog(2,"rmt_transmit: %u", ret);
            if (maintainBufferConsistency)
            {
                // copy editing to sending,
                // this maintains the contract that "colors present before will
                // be the same after", otherwise GetPixelColor will be inconsistent
                memcpy(_dataSending, _dataEditing, _sizeData);
            }

            // swap so the user can modify without affecting the async operation
            std::swap(_dataSending, _dataEditing);
        }
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called only if changes to buffer
        return false;
    }

    bool SwapBuffers()
    {
        std::swap(_dataSending, _dataEditing);
        return true;
    }

    uint8_t* getData() const
    {
        return _dataEditing;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    }

    void applySettings(const SettingsObject& settings)
    {}

private:
    const size_t  _sizeData;      // Size of '_data*' buffers 
    const uint8_t _pin;           // output pin rmt_channel_handle_t

    rmt_transmit_config_t _tx_config = {};
    rmt_encoder_handle_t _led_encoder = nullptr;
    rmt_channel_handle_t _channel = nullptr; // holds dynamic instance for multi channel support

    // Holds data stream which include LED color values and other settings as needed
    uint8_t* _dataEditing;   // exposed for get and set
    uint8_t* _dataSending;   // used for async send using RMT


    void construct()
    {
        // AddLog(2,"RMT:construct");
        _dataEditing = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()

        _dataSending = static_cast<uint8_t*>(malloc(_sizeData));
        // no need to initialize it, it gets overwritten on every send
    }


    static size_t rmt_encode_led_strip(rmt_encoder_t* encoder, rmt_channel_handle_t channel, const void* primary_data, size_t data_size, rmt_encode_state_t* ret_state)
    {
        rmt_led_strip_encoder_t* led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
        rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
        rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
        rmt_encode_state_t session_state = RMT_ENCODING_RESET;
        rmt_encode_state_t state = RMT_ENCODING_RESET;
        size_t encoded_symbols = 0;

        switch (led_encoder->state)
        {
        case 0: // send RGB data
            encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
            if (session_state & RMT_ENCODING_COMPLETE)
            {
                led_encoder->state = 1; // switch to next state when current encoding session finished
            }
            if (session_state & RMT_ENCODING_MEM_FULL)
            {
                // static_cast<AnimalFlags>(static_cast<int>(a) | static_cast<int>(b));
                state = static_cast<rmt_encode_state_t>(static_cast<uint8_t>(state) | static_cast<uint8_t>(RMT_ENCODING_MEM_FULL));
                goto out; // yield if there's no free space for encoding artifacts
            }
            // fall-through
        case 1: // send reset code
            encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                sizeof(led_encoder->reset_code), &session_state);
            if (session_state & RMT_ENCODING_COMPLETE)
            {
                led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
                state = static_cast<rmt_encode_state_t>(static_cast<uint8_t>(state) | static_cast<uint8_t>(RMT_ENCODING_COMPLETE));
            }
            if (session_state & RMT_ENCODING_MEM_FULL)
            {
                state = static_cast<rmt_encode_state_t>(static_cast<uint8_t>(state) | static_cast<uint8_t>(RMT_ENCODING_MEM_FULL));
                goto out; // yield if there's no free space for encoding artifacts
            }
        }

    out:
        *ret_state = state;
        return encoded_symbols;
    }

    static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t* encoder)
    {
        rmt_led_strip_encoder_t* led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
        rmt_del_encoder(led_encoder->bytes_encoder);
        rmt_del_encoder(led_encoder->copy_encoder);
        delete led_encoder;
        return ESP_OK;
    }

    static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t* encoder)
    {
        rmt_led_strip_encoder_t* led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
        rmt_encoder_reset(led_encoder->bytes_encoder);
        rmt_encoder_reset(led_encoder->copy_encoder);
        led_encoder->state = RMT_ENCODING_RESET;
        return ESP_OK;
    }

    static esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t* config, rmt_encoder_handle_t* ret_encoder, uint32_t bit0, uint32_t bit1)
    {
        esp_err_t ret = ESP_OK;
        rmt_led_strip_encoder_t* led_encoder = NULL;
        uint32_t reset_ticks = config->resolution / 1000000 * 50 / 2; // reset code duration defaults to 50us
        rmt_bytes_encoder_config_t bytes_encoder_config;
        rmt_copy_encoder_config_t copy_encoder_config = {};
        rmt_symbol_word_t reset_code_config;


        ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
        led_encoder = new rmt_led_strip_encoder_t();
        ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for led strip encoder");
        led_encoder->base.encode = rmt_encode_led_strip;
        led_encoder->base.del = rmt_del_led_strip_encoder;
        led_encoder->base.reset = rmt_led_strip_encoder_reset;

        bytes_encoder_config.bit0.val = bit0;
        bytes_encoder_config.bit1.val = bit1;

        bytes_encoder_config.flags.msb_first = 1; // WS2812 transfer bit order: G7...G0R7...R0B7...B0 - TODO: more checks

        ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder), err, "TEST_RMT", "create bytes encoder failed");
        ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder), err, "TEST_RMT", "create copy encoder failed");

        reset_code_config.level0 = 0;
        reset_code_config.duration0 = reset_ticks;
        reset_code_config.level1 = 0;
        reset_code_config.duration1 = reset_ticks;
        led_encoder->reset_code = reset_code_config;
        *ret_encoder = &led_encoder->base;
        return ret;

    err:
        // AddLog(2,"RMT:could not init led decoder");
        if (led_encoder)
        {
            if (led_encoder->bytes_encoder)
            {
                rmt_del_encoder(led_encoder->bytes_encoder);
            }
            if (led_encoder->copy_encoder)
            {
                rmt_del_encoder(led_encoder->copy_encoder);
            }
            delete led_encoder;
        }

        return ret;
    }

};

// NOTE:  While these are multi-instance auto channel selecting, there are limits
// to the number of times based on the specific ESP32 model it is compiled for
// ESP32 - 8x (beyond 4x may experience issues)
// ESP32S2 - 4x
// ESP32C3 - 2x
// ESP32S3 - 4x

// normal
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedWs2811> NeoEsp32RmtXWs2811Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedWs2812x> NeoEsp32RmtXWs2812xMethod;
typedef NeoEsp32RmtXWs2812xMethod NeoEsp32RmtXWs2816Method;
typedef NeoEsp32RmtXWs2812xMethod NeoEsp32RmtXWs2813Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedWs2805> NeoEsp32RmtXWs2805Method;
typedef NeoEsp32RmtXWs2805Method NeoEsp32RmtXWs2814Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedSk6812> NeoEsp32RmtXSk6812Method;
typedef NeoEsp32RmtXSk6812Method NeoEsp32RmtXLc8812Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedTm1814, NeoEsp32RmtInverted> NeoEsp32RmtXTm1814Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedTm1829, NeoEsp32RmtInverted> NeoEsp32RmtXTm1829Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedTm1914, NeoEsp32RmtInverted> NeoEsp32RmtXTm1914Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedApa106> NeoEsp32RmtXApa106Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedTx1812> NeoEsp32RmtXTx1812Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedGs1903> NeoEsp32RmtXGs1903Method;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeed800Kbps> NeoEsp32RmtX800KbpsMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeed400Kbps> NeoEsp32RmtX400KbpsMethod;


// inverted
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedWs2811, NeoEsp32RmtInverted> NeoEsp32RmtXWs2811InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedWs2812x, NeoEsp32RmtInverted> NeoEsp32RmtXWs2812xInvertedMethod;
typedef NeoEsp32RmtXWs2812xInvertedMethod NeoEsp32RmtXWs2816InvertedMethod;
typedef NeoEsp32RmtXWs2812xInvertedMethod NeoEsp32RmtXWs2813InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedWs2805, NeoEsp32RmtInverted> NeoEsp32RmtXWs2805InvertedMethod;
typedef NeoEsp32RmtXWs2805InvertedMethod NeoEsp32RmtXWs2814InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedSk6812, NeoEsp32RmtInverted> NeoEsp32RmtXSk6812InvertedMethod;
typedef NeoEsp32RmtXSk6812InvertedMethod NeoEsp32RmtXLc8812InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedTm1814> NeoEsp32RmtXTm1814InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedTm1829> NeoEsp32RmtXTm1829InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedTm1914> NeoEsp32RmtXTm1914InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedApa106, NeoEsp32RmtInverted> NeoEsp32RmtXApa106InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedTx1812, NeoEsp32RmtInverted> NeoEsp32RmtXTx1812InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeedGs1903, NeoEsp32RmtInverted> NeoEsp32RmtXGs1903InvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeed800Kbps, NeoEsp32RmtInverted> NeoEsp32RmtX800KbpsInvertedMethod;
typedef NeoEsp32RmtMethodBase<NeoEsp32RmtSpeed400Kbps, NeoEsp32RmtInverted> NeoEsp32RmtX400KbpsInvertedMethod;

#if defined(NEOPIXEL_ESP32_RMT_DEFAULT) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32S3)

// Normally I2s method is the default, defining NEOPIXEL_ESP32_RMT_DEFAULT 
// will switch to use RMT as the default method
// The ESP32S2, ESP32S3, ESP32C3, ESP32C6 will always default to RMT

typedef NeoEsp32RmtXWs2805Method NeoWs2805Method;
typedef NeoEsp32RmtXWs2811Method NeoWs2811Method;
typedef NeoEsp32RmtXWs2812xMethod NeoWs2812xMethod;
typedef NeoEsp32RmtXWs2816Method NeoWs2816Method;
typedef NeoEsp32RmtXSk6812Method NeoSk6812Method;
typedef NeoEsp32RmtXLc8812Method NeoLc8812Method;
typedef NeoEsp32RmtXTm1814Method NeoTm1814Method;
typedef NeoEsp32RmtXTm1829Method NeoTm1829Method;
typedef NeoEsp32RmtXTm1914Method NeoTm1914Method;
typedef NeoEsp32RmtXApa106Method NeoApa106Method;
typedef NeoEsp32RmtXTx1812Method NeoTx1812Method;
typedef NeoEsp32RmtXWs2812xMethod Neo800KbpsMethod;
typedef NeoEsp32RmtX400KbpsMethod Neo400KbpsMethod;

typedef NeoEsp32RmtXWs2805InvertedMethod NeoWs2805InvertedMethod;
typedef NeoEsp32RmtXWs2811InvertedMethod NeoWs2811InvertedMethod;
typedef NeoEsp32RmtXWs2812xInvertedMethod NeoWs2812xInvertedMethod;
typedef NeoEsp32RmtXWs2816InvertedMethod NeoWs2816InvertedMethod;
typedef NeoEsp32RmtXSk6812InvertedMethod NeoSk6812InvertedMethod;
typedef NeoEsp32RmtXLc8812InvertedMethod NeoLc8812InvertedMethod;
typedef NeoEsp32RmtXTm1814InvertedMethod NeoTm1814InvertedMethod;
typedef NeoEsp32RmtXTm1829InvertedMethod NeoTm1829InvertedMethod;
typedef NeoEsp32RmtXTm1914InvertedMethod NeoTm1914InvertedMethod;
typedef NeoEsp32RmtXApa106InvertedMethod NeoApa106InvertedMethod;
typedef NeoEsp32RmtXTx1812InvertedMethod NeoTx1812InvertedMethod;
typedef NeoEsp32RmtXWs2812xInvertedMethod Neo800KbpsInvertedMethod;
typedef NeoEsp32RmtX400KbpsInvertedMethod Neo400KbpsInvertedMethod;


#endif // defined(NEOPIXEL_ESP32_RMT_DEFAULT) || defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)

#endif
