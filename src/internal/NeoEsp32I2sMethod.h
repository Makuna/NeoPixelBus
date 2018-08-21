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

#ifdef ARDUINO_ARCH_ESP32

extern "C"
{
#include <Arduino.h>
#include "Esp32_i2s.h"
}

const uint16_t c_dmaBytesPerPixelBytes = 4;
const uint16_t c_dmaBytesPer50us = 20;
const uint32_t c_dmaI2sSampleRate = 100000;

class NeoEsp32I2sSpeedWs2813
{
public:
    const static uint16_t I2sSampleRateDiv = 1;
    const static uint16_t ResetTimeUs = 250;
};

class NeoEsp32I2sSpeed800Kbps
{
public:
    const static uint16_t I2sSampleRateDiv = 1;
    const static uint16_t ResetTimeUs = 50;
};

class NeoEsp32I2sSpeed400Kbps
{
public:
    const static uint16_t I2sSampleRateDiv = 2;
    const static uint16_t ResetTimeUs = 50;
};

class NeoEsp32I2sBusZero
{
public:
    const static uint8_t I2sBusNumber = 0;
};

class NeoEsp32I2sBusOne
{
public:
    const static uint8_t I2sBusNumber = 1;
};

template<typename T_SPEED, typename T_BUS> class NeoEsp32I2sMethodBase
{
public:
    NeoEsp32I2sMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize)  :
        _pin(pin)
    {
        uint16_t dmaPixelSize = c_dmaBytesPerPixelBytes * elementSize;
        uint16_t resetSize = (c_dmaBytesPer50us * T_SPEED::ResetTimeUs / 50 / T_SPEED::I2sSampleRateDiv);

        _pixelsSize = pixelCount * elementSize;
        _i2sBufferSize = pixelCount * dmaPixelSize + resetSize;

        // must have a 4 byte aligned buffer for i2s
        uint32_t alignment = _i2sBufferSize % 4;
        if (alignment)
        {
            _i2sBufferSize += 4 - alignment;
        }

        _pixels = (uint8_t*)malloc(_pixelsSize);
        memset(_pixels, 0x00, _pixelsSize);

        _i2sBuffer = (uint8_t*)malloc(_i2sBufferSize);
        memset(_i2sBuffer, 0x00, _i2sBufferSize);
    }

    ~NeoEsp32I2sMethodBase()
    {
        while (!IsReadyToUpdate())
        {
            yield();
        }

        pinMode(_pin, INPUT);

        free(_pixels);
        free(_i2sBuffer);
    }

    bool IsReadyToUpdate() const
    {
        return (i2sWriteDone(T_BUS::I2sBusNumber));
    }

    void Initialize()
    {
        i2sInit(T_BUS::I2sBusNumber, 16, c_dmaI2sSampleRate / T_SPEED::I2sSampleRateDiv, I2S_CHAN_STEREO, I2S_FIFO_16BIT_DUAL, 4, 0);
        i2sSetPins(T_BUS::I2sBusNumber, _pin, -1, -1, -1);
    }

    void Update()
    {
        // wait for not actively sending data
        while (!IsReadyToUpdate())
        {
            yield();
        }

        FillBuffers();

        i2sWrite(T_BUS::I2sBusNumber, _i2sBuffer, _i2sBufferSize, false, false);
    }

    uint8_t* getPixels() const
    {
        return _pixels;
    };

    size_t getPixelsSize() const
    {
        return _pixelsSize;
    }

private:
    const uint8_t _pin;            // output pin number

    size_t    _pixelsSize;    // Size of '_pixels' buffer 
    uint8_t*  _pixels;        // Holds LED color values

    uint32_t _i2sBufferSize; // total size of _i2sBuffer
    uint8_t* _i2sBuffer;  // holds the DMA buffer that is referenced by _i2sBufDesc

    void FillBuffers()
    {
        const uint16_t bitpatterns[16] =
        {
            0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
            0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
            0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
            0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
        };

        uint16_t* pDma = (uint16_t*)_i2sBuffer;
        uint8_t* pPixelsEnd = _pixels + _pixelsSize;
        for (uint8_t* pPixel = _pixels; pPixel < pPixelsEnd; pPixel++)
        {
            *(pDma++) = bitpatterns[((*pPixel) & 0x0f)];
            *(pDma++) = bitpatterns[((*pPixel) >> 4) & 0x0f];
        }
    }
};

typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2813, NeoEsp32I2sBusZero> NeoEsp32I2s0Ws2813Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2sBusZero> NeoEsp32I2s0800KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2sBusZero> NeoEsp32I2s0400KbpsMethod;

typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeedWs2813, NeoEsp32I2sBusOne> NeoEsp32I2s1Ws2813Method;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2sBusOne> NeoEsp32I2s1800KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2sBusOne> NeoEsp32I2s1400KbpsMethod;

// I2s Bus 1 method is the default method for Esp32
typedef NeoEsp32I2s1Ws2813Method NeoWs2813Method;
typedef NeoEsp32I2s1800KbpsMethod Neo800KbpsMethod;
typedef NeoEsp32I2s1400KbpsMethod Neo400KbpsMethod;

#endif