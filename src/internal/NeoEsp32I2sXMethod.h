#pragma once

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

extern "C"
{
#include <Arduino.h>
#include "Esp32_i2s.h"
}

//16 channel
//
//01234567 89abcdef 01234567 89abcdef 01234567 89abcdef 01234567 89abcdef
//encode 0          encode 1          encode 2          encode 3
//1                 0                 0                 0
//1                 1                 1                 0
#pragma once

// ESP32C3 I2S is not supported yet due to significant changes to interface
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3)

// $REVIEW duplicate/refactor to support i2s bus one also
//
class NeoEsp32I2sX8BusZero
{
public:
    const static size_t DmaBitsPerPixelBits = 4; // encoding needs 4 bits to provide pulse for a bit
    const static uint8_t BusMaxCount = 8;
    const static uint8_t I2sBusNumber = 0;
    const static uint8_t InvalidBusId = 255;
    const static size_t DmaBytesPerPixelBytes = BusMaxCount * DmaBitsPerPixelBits;

    NeoEsp32I2sX8BusZero() :
        _busId(InvalidBusId)
    {
    }

    uint8_t RegisterNewMatrixBus(size_t dataSize)
    {
        // find first available bus id
        uint8_t busId = 0;
        while (busId < BusMaxCount)
        {
            uint8_t busIdField = (1 << busId);
            if ((s_UpdateMapMask & busIdField) == 0)
            {
                // complete registration
                s_BusCount++;
                s_UpdateMapMask |= busIdField;
                if (dataSize > s_MaxBusDataSize)
                {
                    s_MaxBusDataSize = dataSize;
                }
                break;
            }
            busId++;
        }
        _busId = busId;
        return busId;
    }

    void Initialize(uint8_t pin, uint32_t i2sSampleRate, bool invert)
    {
        if (s_i2sBuffer == nullptr)
        {
            // only construct once
            construct(i2sSampleRate);
        }
        i2sSetPins(I2sBusNumber, pin, _busId, invert);
    }

    void DeregisterMatrixBus()
    {
        uint8_t busIdField = (1 << _busId);
        if (s_UpdateMapMask & busIdField)
        {
            // complete deregistration
            s_BusCount--;
            s_UpdateMapMask &= ~busIdField;
            if (s_UpdateMapMask == 0)
            {
                destruct();
            }
            // disconnect pin?
        }
        _busId = InvalidBusId;
    }

    bool IsAllMatrixBusesUpdated()
    {
        return (s_UpdateMap == s_UpdateMapMask);
    }

    void StartWrite()
    {
        resetMatrixBusesUpdated();
        i2sWrite(I2sBusNumber, reinterpret_cast<uint8_t*>(s_i2sBuffer), s_i2sBufferSize, false, false);
    }

    bool IsWriteDone()
    {
        return i2sWriteDone(I2sBusNumber);
    }

    void FillBuffers(const uint8_t* data, size_t  sizeData)
    {
        // 8 channel bits layout for DMA 32bit value
        //
        //  matrix bus id  01234567 01234567 01234567 01234567
        //  encode bit #   0        1        2        3
        //  value zero     1        0        0        0
        //  value one      1        1        1        0       

        uint32_t* pDma = s_i2sBuffer;
        const uint8_t* pEnd = data + sizeData;
        for (const uint8_t* pPixel = data; pPixel < pEnd; pPixel++)
        {
            uint8_t value = *pPixel++;
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint32_t dma = *(pDma);

                // clear previous data for matrix bus
                dma &= ~(0x80808080 >> _busId);
                // apply new data for matrix bus
                dma |= (((value & 0x80) ? 0x80808000 : 0x80000000) >> _busId);

                *(pDma++) = dma;
                value <<= 1;
            }
        }

        markMatrixBusUpdated();
    }

private:
    static uint32_t s_i2sBufferSize; // total size of _i2sBuffer
    static uint32_t* s_i2sBuffer;  // holds the DMA buffer that is referenced by _i2sBufDesc

    static size_t s_MaxBusDataSize; // max size of stream data from any single matrix bus
    static uint8_t s_UpdateMap; // bitmap flags of matrix buses to track update state
    static uint8_t s_UpdateMapMask; // mask to used bits in s_UpdateMap
    static uint8_t s_BusCount; // count of matrix buses

    uint8_t _busId; 

    void construct(uint32_t i2sSampleRate)
    {
        s_i2sBufferSize = DmaBytesPerPixelBytes * s_MaxBusDataSize;

        // must have a 4 byte aligned buffer for i2s
        uint32_t alignment = s_i2sBufferSize % 4;
        if (alignment)
        {
            s_i2sBufferSize += 4 - alignment;
        }

        size_t dmaBlockCount = (s_i2sBufferSize + I2S_DMA_MAX_DATA_LEN - 1) / I2S_DMA_MAX_DATA_LEN;

        i2sInit(I2sBusNumber,
            8,
            i2sSampleRate,
            I2S_CHAN_RIGHT_TO_LEFT,
            I2S_FIFO_32BIT_SINGLE,
            dmaBlockCount,
            0);

        s_i2sBuffer = static_cast<uint32_t*>(heap_caps_malloc(s_i2sBufferSize, MALLOC_CAP_DMA));
        // no need to initialize all of it, but since it contains
        // "reset" bits that don't latter get overwritten we just clear it all
        memset(s_i2sBuffer, 0x00, s_i2sBufferSize);
    }

    void destruct()
    {
        if (s_i2sBuffer == nullptr)
        {
            return;
        }

        i2sSetPins(I2sBusNumber, -1, -1, false);
        i2sDeinit(I2sBusNumber);

        heap_caps_free(s_i2sBuffer);

        s_i2sBufferSize = 0;
        s_i2sBuffer = nullptr;
        s_MaxBusDataSize = 0;
        s_UpdateMap = 0;
        s_UpdateMapMask = 0;
        s_BusCount = 0;
    }

    void markMatrixBusUpdated()
    {
        s_UpdateMap |= (1 << _busId);
    }

    void resetMatrixBusesUpdated()
    {
        s_UpdateMap = 0;
    }
};

template<typename T_SPEED, typename T_BUS, typename T_INVERT> class NeoEsp32I2sXMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32I2sXMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin)
    {
        _bus.RegisterNewMatrixBus(_sizeData + T_SPEED::ResetTimeUs / T_SPEED::ByteSendTimeUs);
    }

    ~NeoEsp32I2sXMethodBase()
    {
        while (!_bus.IsWriteDone())
        {
            yield();
        }

        _bus.DeregisterMatrixBus();

        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        return _bus.IsWriteDone();
    }

    void Initialize()
    {
        _bus.Initialize(_pin, T_SPEED::I2sSampleRate, T_INVERT::Inverted);

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()
    }

    void Update(bool)
    {
        // wait for not actively sending data
        while (!_bus.IsWriteDone())
        {
            yield();
        }

        _bus.FillBuffers(_data, _sizeData);

        if (_bus.IsAllMatrixBusesUpdated())
        {
            _bus.StartWrite();
        }
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
    const uint8_t _pin;            // output pin number

    T_BUS _bus; // holds instance for matrix bus support
    uint8_t* _data;        // Holds LED color values
};

// NORMAL
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2sX8BusZero, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Ws2812xMethod;


// INVERTED
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2sX8BusZero, NeoEsp32I2sInverted> NeoEsp32I2s0X8Ws2812xInvertedMethod;

#endif