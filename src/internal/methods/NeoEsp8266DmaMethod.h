/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp8266.


Written by Michael C. Miller.
Thanks to g3gg0.de for porting the initial DMA support which lead to this.
Thanks to github/cnlohr for the original work on DMA support, which opend
all our minds to a better way (located at https://github.com/cnlohr/esp8266ws2812i2s).

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

#ifdef ARDUINO_ARCH_ESP8266
#include "NeoEsp8266I2sMethodCore.h"

class NeoEsp8266DmaSpeedBase
{
public:
    static const uint8_t IdleLevel = 0;
    static uint16_t Convert(uint8_t value)
    {
        const uint16_t bitpatterns[16] =
        {
            0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
            0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
            0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
            0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
        };

        return bitpatterns[value];
    }
};

class NeoEsp8266DmaInvertedSpeedBase
{
public:
    static const uint8_t IdleLevel = 1;
    static uint16_t Convert(uint8_t value)
    {
        const uint16_t bitpatterns[16] =
        {
            0b0111011101110111, 0b0111011101110001, 0b0111011100010111, 0b0111011100010001,
            0b0111000101110111, 0b0111000101110001, 0b0111000100010111, 0b0111000100010001,
            0b0001011101110111, 0b0001011101110001, 0b0001011100010111, 0b0001011100010001,
            0b0001000101110111, 0b0001000101110001, 0b0001000100010111, 0b0001000100010001,
        };

        return bitpatterns[value];
    }
};

class NeoEsp8266DmaSpeed800KbpsBase : public NeoEsp8266DmaSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 5; // 0-63
    const static uint32_t I2sBaseClockDivisor = 10; // 0-63
    const static uint32_t ByteSendTimeUs = 10; // us it takes to send a single pixel element at 800khz speed
};

class NeoEsp8266DmaSpeedWs2812x : public NeoEsp8266DmaSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 300;
};

class NeoEsp8266DmaSpeedWs2805 : public NeoEsp8266DmaSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 4; // 0-63
    const static uint32_t I2sBaseClockDivisor = 11; // 0-63
    const static uint32_t ByteSendTimeUs = 9; // us it takes to send a single pixel element
    const static uint32_t ResetTimeUs = 300; // spec is 280, intentionally longer for compatiblity use
};

class NeoEsp8266DmaSpeedSk6812 : public NeoEsp8266DmaSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 80;
};

class NeoEsp8266DmaInvertedSpeedTm1814 : public NeoEsp8266DmaSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 200;
};

class NeoEsp8266DmaInvertedSpeedTm1829 : public NeoEsp8266DmaSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 200;
};

class NeoEsp8266DmaSpeed800Kbps : public NeoEsp8266DmaSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 50;
};

class NeoEsp8266DmaSpeed400Kbps : public NeoEsp8266DmaSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 10; // 0-63
    const static uint32_t I2sBaseClockDivisor = 10; // 0-63
    const static uint32_t ByteSendTimeUs = 20; // us it takes to send a single pixel element at 400khz speed
    const static uint32_t ResetTimeUs = 50;
};

class NeoEsp8266DmaSpeedApa106 : public NeoEsp8266DmaSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 4; // 0-63
    const static uint32_t I2sBaseClockDivisor = 17; // 0-63
    const static uint32_t ByteSendTimeUs = 14; // us it takes to send a single pixel element
    const static uint32_t ResetTimeUs = 50;
};

class NeoEsp8266DmaSpeedIntertek : public NeoEsp8266DmaSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 5; // 0-63
    const static uint32_t I2sBaseClockDivisor = 10; // 0-63
    const static uint32_t ByteSendTimeUs = 10; // us it takes to send a single pixel element
    const static uint32_t ResetTimeUs = 12470;
};

class NeoEsp8266DmaInvertedSpeed800KbpsBase : public NeoEsp8266DmaInvertedSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 5; // 0-63
    const static uint32_t I2sBaseClockDivisor = 10; // 0-63
    const static uint32_t ByteSendTimeUs = 10; // us it takes to send a single pixel element at 800khz speed
};

class NeoEsp8266DmaInvertedSpeedWs2812x : public NeoEsp8266DmaInvertedSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 300;
};

class NeoEsp8266DmaInvertedSpeedWs2805 : public NeoEsp8266DmaInvertedSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 4; // 0-63
    const static uint32_t I2sBaseClockDivisor = 11; // 0-63
    const static uint32_t ByteSendTimeUs = 9; // us it takes to send a single pixel element
    const static uint32_t ResetTimeUs = 300; // spec is 280, intentionally longer for compatiblity use
};

class NeoEsp8266DmaInvertedSpeedSk6812 : public NeoEsp8266DmaInvertedSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 80;
};

class NeoEsp8266DmaSpeedTm1814 : public NeoEsp8266DmaInvertedSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 200;
};

class NeoEsp8266DmaSpeedTm1829 : public NeoEsp8266DmaInvertedSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 200;
};

class NeoEsp8266DmaInvertedSpeed800Kbps : public NeoEsp8266DmaInvertedSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 50;
};

class NeoEsp8266DmaInvertedSpeed400Kbps : public NeoEsp8266DmaInvertedSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 10; // 0-63
    const static uint32_t I2sBaseClockDivisor = 10; // 0-63
    const static uint32_t ByteSendTimeUs = 20; // us it takes to send a single pixel element at 400khz speed
    const static uint32_t ResetTimeUs = 50;
};

class NeoEsp8266DmaInvertedSpeedApa106 : public NeoEsp8266DmaInvertedSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 4; // 0-63
    const static uint32_t I2sBaseClockDivisor = 17; // 0-63
    const static uint32_t ByteSendTimeUs = 14; // us it takes to send a single pixel element
    const static uint32_t ResetTimeUs = 50;
};

class NeoEsp8266DmaInvertedSpeedIntertek : public NeoEsp8266DmaInvertedSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 5; // 0-63
    const static uint32_t I2sBaseClockDivisor = 10; // 0-63
    const static uint32_t ByteSendTimeUs = 10; // us it takes to send a single pixel element at 800khz speed
    const static uint32_t ResetTimeUs = 12470;
};

template<typename T_SPEED> class NeoEsp8266DmaMethodBase : NeoEsp8266I2sMethodCore
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp8266DmaMethodBase(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _pixelCount(pixelCount)
    {
        size_t dmaPixelSize = DmaBytesPerPixelBytes * elementSize;
        size_t dmaSettingsSize = DmaBytesPerPixelBytes * settingsSize;

        size_t i2sBufferSize = pixelCount * dmaPixelSize + dmaSettingsSize;
        // size is rounded up to nearest c_I2sByteBoundarySize
        i2sBufferSize = NeoUtil::RoundUp(i2sBufferSize, c_I2sByteBoundarySize);

        // calculate a buffer size that takes reset amount of time
        size_t i2sResetSize = T_SPEED::ResetTimeUs * DmaBytesPerPixelBytes / T_SPEED::ByteSendTimeUs;
        // size is rounded up to nearest c_I2sByteBoundarySize
        i2sResetSize = NeoUtil::RoundUp(i2sResetSize, c_I2sByteBoundarySize);
        size_t is2BufMaxBlockSize = (c_maxDmaBlockSize / dmaPixelSize) * dmaPixelSize;

        AllocateI2s(i2sBufferSize, i2sResetSize, is2BufMaxBlockSize, T_SPEED::IdleLevel);
    }

    NeoEsp8266DmaMethodBase([[maybe_unused]] uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) : 
        NeoEsp8266DmaMethodBase(pixelCount, elementSize, settingsSize)
    {
    }

    ~NeoEsp8266DmaMethodBase()
    {
        uint8_t waits = 1;
        while (!IsReadyToUpdate())
        {
            waits = 2;
            yield();
        }

        // wait for any pending sends to complete
        // due to internal i2s caching/send delays, this can more than once the data size
        uint32_t time = micros();
        while ((micros() - time) < ((getPixelTime() + T_SPEED::ResetTimeUs) * waits))
        {
            yield();
        }

        FreeI2s();
    }

    bool IsReadyToUpdate() const
    {
        return IsIdle();
    }

    void Initialize()
    {
        InitializeI2s(T_SPEED::I2sClockDivisor, T_SPEED::I2sBaseClockDivisor);
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
        
        WriteI2s();
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    // due to encoding required for i2s, we need 4 bytes to encode the pulses
    static const uint16_t DmaBytesPerPixelBytes = 4;
    const uint16_t _pixelCount; // count of pixels in the strip

    uint32_t getPixelTime() const
    {
        return (T_SPEED::ByteSendTimeUs * GetSendSize() / DmaBytesPerPixelBytes);
    };

    void FillBuffer(
        const uint8_t* sendData,
        size_t sendDataSize,
        uint8_t** i2sBuffer)
    {
        uint16_t* pDma = reinterpret_cast<uint16_t*>(*i2sBuffer);
        const uint8_t* pEnd = sendData + sendDataSize;

        while (sendData < pEnd)
        {
            *(pDma++) = T_SPEED::Convert(((*sendData) & 0x0f));
            *(pDma++) = T_SPEED::Convert(((*sendData) >> 4) & 0x0f);
            sendData++;
        }

        *i2sBuffer = reinterpret_cast<uint8_t*>(pDma);
    }
};



// normal
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedWs2812x> NeoEsp8266DmaWs2812xMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedWs2805> NeoEsp8266DmaWs2805Method;
typedef NeoEsp8266DmaWs2805Method NeoEsp8266DmaWs2814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedSk6812> NeoEsp8266DmaSk6812Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedTm1814> NeoEsp8266DmaTm1814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedTm1829> NeoEsp8266DmaTm1829Method;
typedef NeoEsp8266DmaTm1814Method NeoEsp8266DmaTm1914Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeed800Kbps> NeoEsp8266Dma800KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeed400Kbps> NeoEsp8266Dma400KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedApa106> NeoEsp8266DmaApa106Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedIntertek> NeoEsp8266DmaIntertekMethod;

// inverted
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedWs2812x> NeoEsp8266DmaInvertedWs2812xMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedWs2805> NeoEsp8266DmaInvertedWs2805Method;
typedef NeoEsp8266DmaInvertedWs2805Method NeoEsp8266DmaInvertedWs2814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedSk6812> NeoEsp8266DmaInvertedSk6812Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedTm1814> NeoEsp8266DmaInvertedTm1814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedTm1829> NeoEsp8266DmaInvertedTm1829Method;
typedef NeoEsp8266DmaInvertedTm1814Method NeoEsp8266DmaInvertedTm1914Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeed800Kbps> NeoEsp8266DmaInverted800KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeed400Kbps> NeoEsp8266DmaInverted400KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedApa106> NeoEsp8266DmaInvertedApa106Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedIntertek> NeoEsp8266DmaInvertedIntertekMethod;

// Dma  method is the default method for Esp8266
typedef NeoEsp8266DmaWs2812xMethod NeoWs2813Method;
typedef NeoEsp8266DmaWs2812xMethod NeoWs2812xMethod;
typedef NeoEsp8266Dma800KbpsMethod NeoWs2812Method;
typedef NeoEsp8266DmaWs2812xMethod NeoWs2811Method;
typedef NeoEsp8266DmaWs2812xMethod NeoWs2816Method;
typedef NeoEsp8266DmaWs2805Method NeoWs2805Method;
typedef NeoEsp8266DmaWs2814Method NeoWs2814Method;
typedef NeoEsp8266DmaSk6812Method NeoSk6812Method;
typedef NeoEsp8266DmaTm1814Method NeoTm1814Method;
typedef NeoEsp8266DmaTm1829Method NeoTm1829Method;
typedef NeoEsp8266DmaTm1914Method NeoTm1914Method;
typedef NeoEsp8266DmaSk6812Method NeoLc8812Method;
typedef NeoEsp8266DmaApa106Method NeoApa106Method;
typedef NeoEsp8266DmaIntertekMethod NeoIntertekMethod;

typedef NeoEsp8266DmaWs2812xMethod Neo800KbpsMethod;
typedef NeoEsp8266Dma400KbpsMethod Neo400KbpsMethod;

// inverted
typedef NeoEsp8266DmaInvertedWs2812xMethod NeoWs2813InvertedMethod;
typedef NeoEsp8266DmaInvertedWs2812xMethod NeoWs2812xInvertedMethod;
typedef NeoEsp8266DmaInverted800KbpsMethod NeoWs2812InvertedMethod;
typedef NeoEsp8266DmaInvertedWs2812xMethod NeoWs2811InvertedMethod;
typedef NeoEsp8266DmaInvertedWs2812xMethod NeoWs2816InvertedMethod;
typedef NeoEsp8266DmaInvertedWs2805Method NeoWs2805InvertedMethod;
typedef NeoEsp8266DmaInvertedWs2814Method NeoWs2814InvertedMethod;
typedef NeoEsp8266DmaInvertedSk6812Method NeoSk6812InvertedMethod;
typedef NeoEsp8266DmaInvertedTm1814Method NeoTm1814InvertedMethod;
typedef NeoEsp8266DmaInvertedTm1829Method NeoTm1829InvertedMethod;
typedef NeoEsp8266DmaInvertedTm1914Method NeoTm1914InvertedMethod;
typedef NeoEsp8266DmaInvertedSk6812Method NeoLc8812InvertedMethod;
typedef NeoEsp8266DmaInvertedApa106Method NeoApa106InvertedMethod;
typedef NeoEsp8266DmaInvertedIntertekMethod NeoInvertedIntertekMethod;

typedef NeoEsp8266DmaInvertedWs2812xMethod Neo800KbpsInvertedMethod;
typedef NeoEsp8266DmaInverted400KbpsMethod Neo400KbpsInvertedMethod;
#endif
