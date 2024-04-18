/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp32.

Written by Michael C. Miller.

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

// ESP32 C3 & S3 I2S is not supported yet due to significant changes to interface
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)


extern "C"
{
#include <rom/gpio.h>
#include "Esp32_i2s.h"
}

const uint16_t c_dmaBytesPerPixelBytes = 4;

class NeoEsp32I2sSpeedWs2812x
{
public:
    const static uint32_t I2sSampleRate = 100000;
    const static uint16_t ByteSendTimeUs = 10;
    const static uint16_t ResetTimeUs = 300;
};

class NeoEsp32I2sSpeedWs2805
{
public:
    const static uint32_t I2sSampleRate = 114678;  //  917431 hz / 8 = 114678
    const static uint16_t ByteSendTimeUs = 9;
    const static uint16_t ResetTimeUs = 300; // spec is 280, intentionally longer for compatiblity use
};

class NeoEsp32I2sSpeedSk6812
{
public:
    const static uint32_t I2sSampleRate = 100000;
    const static uint16_t ByteSendTimeUs = 10;
    const static uint16_t ResetTimeUs = 80;
};

class NeoEsp32I2sSpeedTm1814
{
public:
    const static uint32_t I2sSampleRate = 100000;
    const static uint16_t ByteSendTimeUs = 10;
    const static uint16_t ResetTimeUs = 200;
};

class NeoEsp32I2sSpeedTm1914
{
public:
    const static uint32_t I2sSampleRate = 100000;
    const static uint16_t ByteSendTimeUs = 10;
    const static uint16_t ResetTimeUs = 200;
};

class NeoEsp32I2sSpeedTm1829
{
public:
    const static uint32_t I2sSampleRate = 100000;
    const static uint16_t ByteSendTimeUs = 10;
    const static uint16_t ResetTimeUs = 200;
};

class NeoEsp32I2sSpeed800Kbps
{
public:
    const static uint32_t I2sSampleRate = 100000;
    const static uint16_t ByteSendTimeUs = 10;
    const static uint16_t ResetTimeUs = 50;
};

class NeoEsp32I2sSpeed400Kbps
{
public:
    const static uint32_t I2sSampleRate = 50000;
    const static uint16_t ByteSendTimeUs = 20;
    const static uint16_t ResetTimeUs = 50;
};

class NeoEsp32I2sSpeedApa106
{
public:
    const static uint32_t I2sSampleRate = 72960;  //  588,235 hz / 8 = 73,529 sample rate
    const static uint16_t ByteSendTimeUs = 14;
    const static uint16_t ResetTimeUs = 50;
};

class NeoEsp32I2sBusZero
{
public:
    NeoEsp32I2sBusZero() {};

    const static uint8_t I2sBusNumber = 0;
};

class NeoEsp32I2sBusOne
{
public:
    NeoEsp32I2sBusOne() {};

    const static uint8_t I2sBusNumber = 1;
};

// dynamic channel support
class NeoEsp32I2sBusN
{
public:
    NeoEsp32I2sBusN(NeoBusChannel channel) :
        I2sBusNumber(static_cast<uint8_t>(channel))
    {
    }
    NeoEsp32I2sBusN() = delete; // no default constructor

    const uint8_t I2sBusNumber;
};

class NeoEsp32I2sNotInverted
{
public:
    const static bool Inverted = false;
};

class NeoEsp32I2sInverted
{
public:
    const static bool Inverted = true;
};

template<typename T_SPEED, typename T_BUS, typename T_INVERT> class NeoEsp32I2sMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32I2sMethodBase(uint8_t pin, 
        uint16_t pixelCount, 
        size_t pixelSize, 
        size_t settingsSize)  :
        _pin(pin),
        _pixelCount(pixelCount)
    {
        construct(pixelSize, settingsSize);
    }

    NeoEsp32I2sMethodBase(uint8_t pin, 
        uint16_t pixelCount, 
        size_t pixelSize, 
        size_t settingsSize, 
        NeoBusChannel channel) :
        _pin(pin),
        _pixelCount(pixelCount),
        _bus(channel)
    {
        construct(pixelSize, settingsSize);
    }

    ~NeoEsp32I2sMethodBase()
    {
        while (!IsReadyToUpdate())
        {
            yield();
        }

        i2sDeinit(_bus.I2sBusNumber);

        gpio_matrix_out(_pin, SIG_GPIO_OUT_IDX, false, false);
        pinMode(_pin, INPUT);

        heap_caps_free(_i2sBuffer);
    }

    bool IsReadyToUpdate() const
    {
        return (i2sWriteDone(_bus.I2sBusNumber));
    }

    void Initialize()
    {
        size_t dmaBlockCount = (_i2sBufferSize + I2S_DMA_MAX_DATA_LEN - 1) / I2S_DMA_MAX_DATA_LEN;

        i2sInit(_bus.I2sBusNumber, 
            false,
            2, // bytes per sample 
            T_SPEED::I2sSampleRate, 
            I2S_CHAN_STEREO, 
            I2S_FIFO_16BIT_DUAL, 
            dmaBlockCount,
            _i2sBuffer,
            _i2sBufferSize);
        i2sSetPins(_bus.I2sBusNumber, _pin, -1, -1, T_INVERT::Inverted);
    }

    template <typename T_COLOR_OBJECT,
        typename T_COLOR_FEATURE,
        typename T_SHADER>
    void Update(
        T_COLOR_OBJECT* pixels,
        size_t countPixels,
        const typename T_COLOR_FEATURE::SettingsObject& featureSettings,
        const T_SHADER& shader)
    {
        // wait for not actively sending data
        while (!IsReadyToUpdate())
        {
            yield();
        }

        const size_t sendDataSize = T_COLOR_FEATURE::SettingsSize >= T_COLOR_FEATURE::PixelSize ? T_COLOR_FEATURE::SettingsSize : T_COLOR_FEATURE::PixelSize;
        uint8_t sendData[sendDataSize];
        uint8_t* dma = _i2sBuffer;

        // if there are settings at the front
        //
        if (T_COLOR_FEATURE::applyFrontSettings(sendData, sendDataSize, featureSettings))
        {
            FillBuffer(sendData, T_COLOR_FEATURE::SettingsSize, &dma);
        }

        // fill primary color data
        //
        T_COLOR_OBJECT* pixel = pixels;
        const T_COLOR_OBJECT* pixelEnd = pixel + countPixels;
        uint16_t stripCount = _pixelCount;

        while (stripCount--)
        {
            typename T_COLOR_FEATURE::ColorObject color = shader.Apply(*pixel);
            T_COLOR_FEATURE::applyPixelColor(sendData, sendDataSize, color);

            FillBuffer(sendData, T_COLOR_FEATURE::PixelSize, &dma);

            pixel++;
            if (pixel >= pixelEnd)
            {
                // restart at first
                pixel = pixels;
            }
        }

        // if there are settings at the back
        //
        if (T_COLOR_FEATURE::applyBackSettings(sendData, sendDataSize, featureSettings))
        {
            FillBuffer(sendData, T_COLOR_FEATURE::SettingsSize, &dma);
        }

        i2sWrite(_bus.I2sBusNumber);
    }


    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    const uint8_t _pin;            // output pin number
    const uint16_t _pixelCount; // count of pixels in the strip
    const T_BUS _bus; // holds instance for multi bus support

    size_t _i2sBufferSize; // total size of _i2sBuffer
    uint8_t* _i2sBuffer;  // holds the DMA buffer that is referenced by _i2sBufDesc

    void construct(size_t pixelSize, size_t settingsSize) 
    {
        // must have a 4 byte aligned buffer for i2s
        // since the reset/silence at the end is used for looping
        // it also needs to 4 byte aligned
        size_t dmaSettingsSize = c_dmaBytesPerPixelBytes * settingsSize;
        size_t dmaPixelSize = c_dmaBytesPerPixelBytes * pixelSize;
        size_t resetSize = NeoUtil::RoundUp(c_dmaBytesPerPixelBytes * T_SPEED::ResetTimeUs / T_SPEED::ByteSendTimeUs, 4);

        _i2sBufferSize = NeoUtil::RoundUp(_pixelCount * dmaPixelSize + dmaSettingsSize, 4) +
                resetSize;

        _i2sBuffer = static_cast<uint8_t*>(heap_caps_malloc(_i2sBufferSize, MALLOC_CAP_DMA));
        // no need to initialize all of it, but since it contains
        // "reset" bits that don't latter get overwritten we just clear it all
        memset(_i2sBuffer, 0x00, _i2sBufferSize);
    }

    void FillBuffer(const uint8_t* sendData, size_t sendDataSize, uint8_t** buffer)
    {
        const uint16_t bitpatterns[16] =
        {
            0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
            0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
            0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
            0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
        };

        uint16_t* pDma = reinterpret_cast<uint16_t*>(*buffer);
        const uint8_t* pEnd = sendData + sendDataSize;

        while (sendData < pEnd)
        { 
            *(pDma++) = bitpatterns[((*sendData) & 0x0f)];
            *(pDma++) = bitpatterns[((*sendData) >> 4) & 0x0f];
            sendData++;
        }

        *buffer = reinterpret_cast<uint8_t*>(pDma);
    }
};

typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0Ws2812xMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2805, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0Ws2805Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0Sk6812Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0Tm1814Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0Tm1829Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0Tm1914Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0800KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0400KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0Apa106Method;
typedef NeoEsp32I2s0Ws2805Method NeoEsp32I2s0Ws2814Method;

typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0Ws2812xInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2805, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0SWs2805InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0Sk6812InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0Tm1814InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0Tm1914InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2sBusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0Tm1829InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0800KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0400KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2sBusZero, NeoEsp32I2sInverted> NeoEsp32I2s0Apa106InvertedMethod;
typedef NeoEsp32I2s0SWs2805InvertedMethod NeoEsp32I2s0SWs2814InvertedMethod;

#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
// (SOC_I2S_NUM == 2)

typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1Ws2812xMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2805, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1Ws2805Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1Sk6812Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1Tm1814Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1Tm1829Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1Tm1914Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1800KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1400KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1Apa106Method;
typedef NeoEsp32I2s1Ws2805Method NeoEsp32I2s1Ws2814Method;

typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1Ws2812xInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2805, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1Ws2805InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1Sk6812InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1Tm1814InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1Tm1829InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2sBusOne, NeoEsp32I2sNotInverted> NeoEsp32I2s1Tm1914InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1800KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1400KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2sBusOne, NeoEsp32I2sInverted> NeoEsp32I2s1Apa106InvertedMethod;
typedef NeoEsp32I2s1Ws2805InvertedMethod NeoEsp32I2s1Ws2814InvertedMethod;

typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sNWs2812xMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2805, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sNWs2805Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sNSk6812Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sNTm1814Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sNTm1829Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sNTm1914Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sN800KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sN400KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sNApa106Method;
typedef NeoEsp32I2sNWs2805Method NeoEsp32I2sNWs2814Method;

typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sNWs2812xInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2805, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sNWs2805InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sNSk6812InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sNTm1814InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sNTm1829InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2sBusN, NeoEsp32I2sNotInverted> NeoEsp32I2sNTm1914InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sN800KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sN400KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2sBusN, NeoEsp32I2sInverted> NeoEsp32I2sNApa106InvertedMethod;
typedef NeoEsp32I2sNWs2805InvertedMethod NeoEsp32I2sNWs2814InvertedMethod;

#endif

#if !defined(NEOPIXEL_ESP32_RMT_DEFAULT) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) 

// I2s Bus 1 method is the default method for Esp32
// Esp32 S2 & C3 & S3 will use RMT as the default allways
typedef NeoEsp32I2s1Ws2812xMethod NeoWs2813Method;
typedef NeoEsp32I2s1Ws2812xMethod NeoWs2812xMethod;
typedef NeoEsp32I2s1800KbpsMethod NeoWs2812Method;
typedef NeoEsp32I2s1Ws2812xMethod NeoWs2811Method;
typedef NeoEsp32I2s1Ws2812xMethod NeoWs2816Method;
typedef NeoEsp32I2s1Ws2805Method NeoWs2805Method;
typedef NeoEsp32I2s1Ws2814Method NeoWs2814Method;
typedef NeoEsp32I2s1Sk6812Method NeoSk6812Method;
typedef NeoEsp32I2s1Tm1814Method NeoTm1814Method;
typedef NeoEsp32I2s1Tm1829Method NeoTm1829Method;
typedef NeoEsp32I2s1Tm1914Method NeoTm1914Method;
typedef NeoEsp32I2s1Sk6812Method NeoLc8812Method;
typedef NeoEsp32I2s1Apa106Method NeoApa106Method;

typedef NeoEsp32I2s1Ws2812xMethod Neo800KbpsMethod;
typedef NeoEsp32I2s1400KbpsMethod Neo400KbpsMethod;

typedef NeoEsp32I2s1Ws2812xInvertedMethod NeoWs2813InvertedMethod;
typedef NeoEsp32I2s1Ws2812xInvertedMethod NeoWs2812xInvertedMethod;
typedef NeoEsp32I2s1Ws2812xInvertedMethod NeoWs2811InvertedMethod;
typedef NeoEsp32I2s1Ws2812xInvertedMethod NeoWs2816InvertedMethod;
typedef NeoEsp32I2s1Ws2805InvertedMethod NeoWs2805InvertedMethod;
typedef NeoEsp32I2s1Ws2814InvertedMethod NeoWs2814InvertedMethod;
typedef NeoEsp32I2s1800KbpsInvertedMethod NeoWs2812InvertedMethod;
typedef NeoEsp32I2s1Sk6812InvertedMethod NeoSk6812InvertedMethod;
typedef NeoEsp32I2s1Tm1814InvertedMethod NeoTm1814InvertedMethod;
typedef NeoEsp32I2s1Tm1829InvertedMethod NeoTm1829InvertedMethod;
typedef NeoEsp32I2s1Tm1914InvertedMethod NeoTm1914InvertedMethod;
typedef NeoEsp32I2s1Sk6812InvertedMethod NeoLc8812InvertedMethod;
typedef NeoEsp32I2s1Apa106InvertedMethod NeoApa106InvertedMethod;

typedef NeoEsp32I2s1Ws2812xInvertedMethod Neo800KbpsInvertedMethod;
typedef NeoEsp32I2s1400KbpsInvertedMethod Neo400KbpsInvertedMethod;

#endif // !defined(NEOPIXEL_ESP32_RMT_DEFAULT) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)

#endif
