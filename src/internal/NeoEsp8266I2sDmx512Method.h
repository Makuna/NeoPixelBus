/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp8266.

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

#ifdef ARDUINO_ARCH_ESP8266
#include "NeoEsp8266I2sMethodCore.h"

class NeoEsp8266I2sDmx512Speed
{
public:
    // 4 us bit send
    static const uint32_t I2sClockDivisor = 106;
    static const uint32_t I2sBaseClockDivisor = 16;
    static const uint32_t ByteSendTimeUs = 44; // us it takes to send a single pixel element at 800khz speed
    static const uint32_t MtbpUs = 100; // min 88
    static const uint8_t MtbpLevel = 0x1; // high
    static const uint32_t BreakMab = 0x00000003; // Break + Mab
};

class NeoEsp8266I2sDmx512InvertedSpeed
{
public:
    // 4 us bit send
    static const uint32_t I2sClockDivisor = 106;
    static const uint32_t I2sBaseClockDivisor = 16;
    static const uint32_t ByteSendTimeUs = 44; // us it takes to send a single pixel element at 800khz speed
    static const uint32_t MtbpUs = 100; // min 88
    static const uint8_t MtbpLevel = 0x00; // low
    static const uint32_t BreakMab = 0xfffffffc; // Break + Mab
};


// given 11 sending bits per pixel byte, 
// 8 pixel bytes would be 88 bits or 11 bytes of sending buffer
// i2s sends 2 byte elements, 
// so 22 byte of sending buffer gives byte boundary sending buffers 
const uint16_t c_i2sBitsPerPixelBytes = 11;
const uint16_t c_i2sByteBoundarySize = 22;

template<typename T_SPEED> class NeoEsp8266I2sDmx512MethodBase : NeoEsp8266I2sMethodCore
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp8266I2sDmx512MethodBase(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize)
    {
        size_t dmaPixelBits = c_i2sBitsPerPixelBytes * elementSize;
        size_t dmaSettingsBits = c_i2sBitsPerPixelBytes * settingsSize;

        // bits + half rounding byte of bits / bits per byte
        size_t i2sBufferSize = (pixelCount * dmaPixelBits + dmaSettingsBits + 4) / 8;
        i2sBufferSize = i2sBufferSize + sizeof(T_SPEED::BreakMab);
        // size is rounded up to nearest 22 byte boundary
        i2sBufferSize = i2sBufferSize + c_i2sByteBoundarySize - (i2sBufferSize % c_i2sByteBoundarySize);

        // 4 us per bit, add half rounding byte of bits / bits per byte
        size_t i2sZeroesBitsSize = (T_SPEED::MtbpUs) / 4;
        size_t i2sZeroesSize = (i2sZeroesBitsSize + 4) / 8;

        // protocol limits use of full block size to c_i2sByteBoundarySize
        size_t is2BufMaxBlockSize = (c_maxDmaBlockSize / c_i2sByteBoundarySize) * c_i2sByteBoundarySize;

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()

        AllocateI2s(i2sBufferSize, i2sZeroesSize, is2BufMaxBlockSize, T_SPEED::MtbpLevel);
    }

    NeoEsp8266I2sDmx512MethodBase([[maybe_unused]] uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) : 
        NeoEsp8266I2sDmx512MethodBase(pixelCount, elementSize, settingsSize)
    {
    }

    ~NeoEsp8266I2sDmx512MethodBase()
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
        while ((micros() - time) < ((getPixelTime() + T_SPEED::MtbpUs) * waits))
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
    uint8_t*  _data;        // Holds LED color values

    void FillBuffers()
    {
        // stops bits then start bits merged into one (SSs)
        const uint8_t controlBits = (T_SPEED::MtbpLevel * 0x06) | (T_SPEED::MtbpLevel ^ 1);

        uint8_t* pDma = _i2sBuffer;
        uint8_t* pEnd = _data + _sizeData;

        // first put Break and MAB at front
        // 
        // BREAK 100us @ 4us per bit 
        // MAB 8us
        *(pDma++) = T_SPEED::BreakMab >> 24;
        *(pDma++) = T_SPEED::BreakMab >> 16;
        *(pDma++) = T_SPEED::BreakMab >> 8;
        *(pDma++) = T_SPEED::BreakMab;

        // DATA stream, one start, two stop
        for (uint8_t* pData = _data; pData < pEnd; pData++)
        {
            uint8_t byte0 = *(pData++);
            uint8_t byte1 = *(pData++);
            uint8_t byte2 = *(pData++);
            uint8_t byte3 = *(pData++);
            uint8_t byte4 = *(pData++);
            uint8_t byte5 = *(pData++);
            uint8_t byte6 = *(pData++);
            uint8_t byte7 = *(pData++);

            // s0123456 7SSs0123
            *(pDma++) = (controlBits << 7) | (byte0 >> 1);
            *(pDma++) = (byte0 << 7) | (controlBits << 4) | (byte1 >> 4);
           
            // 4567SSs0 1234567S 
            *(pDma++) = (byte1 << 4) | (controlBits << 1) | (byte2 >> 7);
            *(pDma++) = (byte2 << 1) | (controlBits >> 2);

            // Ss012345 67SSs012
            *(pDma++) = (controlBits << 6) | (byte3 >> 2);
            *(pDma++) = (byte3 << 6) | (controlBits << 3) | (byte4 >> 5);

            // 34567SSs 01234567
            *(pDma++) = (byte4 << 3) | (controlBits);
            *(pDma++) = (byte5);

            // SSs01234 567SSs01
            *(pDma++) = (controlBits << 5) | (byte6 >> 3);
            *(pDma++) = (byte6 << 5) | (controlBits << 2) | (byte7 >> 6);

            // 234567SS 
            *(pDma++) = (byte7 << 2) | (controlBits > 1);
        }
        
    }

    uint32_t getPixelTime() const
    {
        return (T_SPEED::ByteSendTimeUs * this->_sizeData);
    };

};


// normal
typedef NeoEsp8266I2sDmx512MethodBase<NeoEsp8266I2sDmx512Speed> NeoEsp8266Dmx512Method;

// inverted
typedef NeoEsp8266I2sDmx512MethodBase<NeoEsp8266I2sDmx512InvertedSpeed> NeoEsp8266Dmx512InvertedMethod;

#endif
