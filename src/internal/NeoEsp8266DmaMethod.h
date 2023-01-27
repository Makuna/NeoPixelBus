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
    const static uint32_t I2sClockDivisor = 3; // 0-63
    const static uint32_t I2sBaseClockDivisor = 16; // 0-63
    const static uint32_t ByteSendTimeUs = 10; // us it takes to send a single pixel element at 800khz speed
};

class NeoEsp8266DmaSpeedWs2812x : public NeoEsp8266DmaSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 300;
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
    const static uint32_t I2sClockDivisor = 6; // 0-63
    const static uint32_t I2sBaseClockDivisor = 16; // 0-63
    const static uint32_t ByteSendTimeUs = 20; // us it takes to send a single pixel element at 400khz speed
    const static uint32_t ResetTimeUs = 50;
};

class NeoEsp8266DmaSpeedApa106 : public NeoEsp8266DmaSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 4; // 0-63
    const static uint32_t I2sBaseClockDivisor = 16; // 0-63
    const static uint32_t ByteSendTimeUs = 17; // us it takes to send a single pixel element
    const static uint32_t ResetTimeUs = 50;
};



class NeoEsp8266DmaInvertedSpeed800KbpsBase : public NeoEsp8266DmaInvertedSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 3; // 0-63
    const static uint32_t I2sBaseClockDivisor = 16; // 0-63
    const static uint32_t ByteSendTimeUs = 10; // us it takes to send a single pixel element at 800khz speed
};

class NeoEsp8266DmaInvertedSpeedWs2812x : public NeoEsp8266DmaInvertedSpeed800KbpsBase
{
public:
    const static uint32_t ResetTimeUs = 300;
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
    const static uint32_t I2sClockDivisor = 6; // 0-63
    const static uint32_t I2sBaseClockDivisor = 16; // 0-63
    const static uint32_t ByteSendTimeUs = 20; // us it takes to send a single pixel element at 400khz speed
    const static uint32_t ResetTimeUs = 50;
};

class NeoEsp8266DmaInvertedSpeedApa106 : public NeoEsp8266DmaInvertedSpeedBase
{
public:
    const static uint32_t I2sClockDivisor = 4; // 0-63
    const static uint32_t I2sBaseClockDivisor = 16; // 0-63
    const static uint32_t ByteSendTimeUs = 17; // us it takes to send a single pixel element
    const static uint32_t ResetTimeUs = 50;
};



template<typename T_SPEED> class NeoEsp8266DmaMethodBase : NeoEsp8266I2sMethodCore
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp8266DmaMethodBase(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize)
    {
        size_t dmaPixelSize = DmaBytesPerPixelBytes * elementSize;
        size_t dmaSettingsSize = DmaBytesPerPixelBytes * settingsSize;

        size_t i2sBufferSize = pixelCount * dmaPixelSize + dmaSettingsSize;

        // normally 24 bytes creates the minimum 50us latch per spec, but
        // with the new logic, this latch is used to space between mulitple states
        // buffer size = (24 * (reset time / 50)) / 6
        size_t i2sZeroesSize = (24L * (T_SPEED::ResetTimeUs / 50L)) / 6L;

        size_t is2BufMaxBlockSize = (c_maxDmaBlockSize / dmaPixelSize) * dmaPixelSize;

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()

        AllocateI2s(i2sBufferSize, i2sZeroesSize, is2BufMaxBlockSize, T_SPEED::IdleLevel);
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
        // due to internal i2s caching/send delays, this can more that once the data size
        uint32_t time = micros();
        while ((micros() - time) < ((getPixelTime() + T_SPEED::ResetTimeUs) * waits))
        {
            yield();
        }

        FreeI2s();

        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        return IsIdle();
    }

    void Initialize()
    {
        InitializeI2s(T_SPEED::I2sClockDivisor, T_SPEED::I2sBaseClockDivisor);
    }

    void IRAM_ATTR Update(bool)
    {
        // wait for not actively sending data
        while (!IsReadyToUpdate())
        {
            yield();
        }
        FillBuffers();
        
        // toggle state so the ISR reacts
        _dmaState = NeoDmaState_Pending;
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called only if changes to buffer
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
    // due to encoding required for i2s, we need 4 bytes to encode the pulses
    static const uint16_t DmaBytesPerPixelBytes = 4;

    const size_t  _sizeData;    // Size of '_data' buffer 
    uint8_t*  _data;        // Holds LED color values

    void FillBuffers()
    {
        uint16_t* pDma = (uint16_t*)_i2sBuffer;
        uint8_t* pEnd = _data + _sizeData;
        for (uint8_t* pData = _data; pData < pEnd; pData++)
        {
            *(pDma++) = T_SPEED::Convert(((*pData) & 0x0f));
            *(pDma++) = T_SPEED::Convert(((*pData) >> 4) & 0x0f);
        }
    }

    uint32_t getPixelTime() const
    {
        return (T_SPEED::ByteSendTimeUs * this->_sizeData);
    };

};



// normal
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedWs2812x> NeoEsp8266DmaWs2812xMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedSk6812> NeoEsp8266DmaSk6812Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedTm1814> NeoEsp8266DmaTm1814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedTm1829> NeoEsp8266DmaTm1829Method;
typedef NeoEsp8266DmaTm1814Method NeoEsp8266DmaTm1914Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeed800Kbps> NeoEsp8266Dma800KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeed400Kbps> NeoEsp8266Dma400KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeedApa106> NeoEsp8266DmaApa106Method;


// inverted
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedWs2812x> NeoEsp8266DmaInvertedWs2812xMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedSk6812> NeoEsp8266DmaInvertedSk6812Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedTm1814> NeoEsp8266DmaInvertedTm1814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedTm1829> NeoEsp8266DmaInvertedTm1829Method;
typedef NeoEsp8266DmaInvertedTm1814Method NeoEsp8266DmaInvertedTm1914Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeed800Kbps> NeoEsp8266DmaInverted800KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeed400Kbps> NeoEsp8266DmaInverted400KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaInvertedSpeedApa106> NeoEsp8266DmaInvertedApa106Method;

// Dma  method is the default method for Esp8266
typedef NeoEsp8266DmaWs2812xMethod NeoWs2813Method;
typedef NeoEsp8266DmaWs2812xMethod NeoWs2812xMethod;
typedef NeoEsp8266Dma800KbpsMethod NeoWs2812Method;
typedef NeoEsp8266DmaWs2812xMethod NeoWs2811Method;
typedef NeoEsp8266DmaWs2812xMethod NeoWs2816Method;
typedef NeoEsp8266DmaSk6812Method NeoSk6812Method;
typedef NeoEsp8266DmaTm1814Method NeoTm1814Method;
typedef NeoEsp8266DmaTm1829Method NeoTm1829Method;
typedef NeoEsp8266DmaTm1914Method NeoTm1914Method;
typedef NeoEsp8266DmaSk6812Method NeoLc8812Method;
typedef NeoEsp8266DmaApa106Method NeoApa106Method;

typedef NeoEsp8266DmaWs2812xMethod Neo800KbpsMethod;
typedef NeoEsp8266Dma400KbpsMethod Neo400KbpsMethod;

// inverted
typedef NeoEsp8266DmaInvertedWs2812xMethod NeoWs2813InvertedMethod;
typedef NeoEsp8266DmaInvertedWs2812xMethod NeoWs2812xInvertedMethod;
typedef NeoEsp8266DmaInverted800KbpsMethod NeoWs2812InvertedMethod;
typedef NeoEsp8266DmaInvertedWs2812xMethod NeoWs2811InvertedMethod;
typedef NeoEsp8266DmaInvertedWs2812xMethod NeoWs2816InvertedMethod;
typedef NeoEsp8266DmaInvertedSk6812Method NeoSk6812InvertedMethod;
typedef NeoEsp8266DmaInvertedTm1814Method NeoTm1814InvertedMethod;
typedef NeoEsp8266DmaInvertedTm1829Method NeoTm1829InvertedMethod;
typedef NeoEsp8266DmaInvertedTm1914Method NeoTm1914InvertedMethod;
typedef NeoEsp8266DmaInvertedSk6812Method NeoLc8812InvertedMethod;
typedef NeoEsp8266DmaInvertedApa106Method NeoApa106InvertedMethod;

typedef NeoEsp8266DmaInvertedWs2812xMethod Neo800KbpsInvertedMethod;
typedef NeoEsp8266DmaInverted400KbpsMethod Neo400KbpsInvertedMethod;
#endif
