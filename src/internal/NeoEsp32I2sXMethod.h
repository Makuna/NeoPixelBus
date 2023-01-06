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

//16 channel layout if it ever gets supported
//
//01234567 89abcdef 01234567 89abcdef 01234567 89abcdef 01234567 89abcdef
//encode 0          encode 1          encode 2          encode 3
//1                 0                 0                 0
//1                 1                 1                 0

#pragma once

// ESP32C3 I2S is not supported yet due to significant changes to interface
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3)

class NeoEspI2sContext
{
public:
    const static size_t DmaBytesPerPixelBits = 4;
    const uint8_t BusMaxCount;
    const static uint8_t InvalidMuxId = -1;

    uint32_t I2sBufferSize; // total size of I2sBuffer
    uint32_t* I2sBuffer;    // holds the DMA buffer that is referenced by I2sBufDesc

    size_t MaxBusDataSize; // max size of stream data from any single mux bus
    uint8_t UpdateMap;     // bitmap flags of mux buses to track update state
    uint8_t UpdateMapMask; // mask to used bits in s_UpdateMap
    uint8_t BusCount;      // count of mux buses

    NeoEspI2sContext(uint8_t _BusMaxCount) :
        BusMaxCount(_BusMaxCount),
        I2sBufferSize(0),
        I2sBuffer(nullptr),
        MaxBusDataSize(0),
        UpdateMap(0),
        UpdateMapMask(0),
        BusCount(0)
    {
    }

    uint8_t RegisterNewMuxBus(const size_t dataSize)
    {
        // find first available bus id
        uint8_t muxId = 0;
        while (muxId < BusMaxCount)
        {
            uint8_t muxIdField = (1 << muxId);
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
        return muxId;
    }

    bool DeregisterMuxBus(uint8_t muxId)
    {
        uint8_t muxIdField = (1 << muxId);
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

    void MarkMuxBusUpdated(uint8_t muxId)
    {
        UpdateMap |= (1 << muxId);
    }

    void ResetMuxBusesUpdated()
    {
        UpdateMap = 0;
    }

    void Construct(const uint8_t busNumber, uint32_t i2sSampleRate)
    {
        // construct only once on first time called
        if (I2sBuffer == nullptr)
        {
            // $REVIEW this change to evaluate for cleanup. Its best to keep branching like this to a single location and wrap the concept into a const like the original.
            // 1 byte on the input => 8 (bits) * DmaBytesPerPixelBits (bits of 4 bytes are used for neopixel) * only x8 on I2S1 is true 8bits mode and requires less memory than 16bits modes
            I2sBufferSize = MaxBusDataSize * 8 * DmaBytesPerPixelBits * ((busNumber == 1 && BusMaxCount == 8) ? 1 : 2);

            // must have a 4 byte aligned buffer for i2s
            uint32_t alignment = I2sBufferSize % 4;
            if (alignment)
            {
                I2sBufferSize += 4 - alignment;
            }

            size_t dmaBlockCount = (I2sBufferSize + I2S_DMA_MAX_DATA_LEN - 1) / I2S_DMA_MAX_DATA_LEN;

            // parallel modes need higher frequency on esp32
#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)           
            if (BusMaxCount == 8 || BusMaxCount == 16)
            {
                // 8 bits mode on i2s1 needs two times lower frequency than other parallel modes
                if (busNumber == 1 && BusMaxCount == 8)
                    i2sSampleRate *= 2;
                else
                    i2sSampleRate *= 4;
            }
#endif

            i2sInit(busNumber,
                BusMaxCount,
                i2sSampleRate,
                I2S_CHAN_RIGHT_TO_LEFT,
                I2S_FIFO_32BIT_SINGLE,
                dmaBlockCount,
                0);

            Serial.print("MaxBusDataSize = ");
            Serial.println(MaxBusDataSize);

            Serial.print("I2sBufferSize = ");
            Serial.println(I2sBufferSize);

            I2sBuffer = static_cast<uint32_t*>(heap_caps_malloc(I2sBufferSize, MALLOC_CAP_DMA));
            // no need to initialize all of it, but since it contains
            // "reset" bits that don't later get overwritten we just clear it all
            memset(I2sBuffer, 0x00, I2sBufferSize);
        }
    }

    void Destruct(const uint8_t busNumber)
    {
        if (I2sBuffer == nullptr)
        {
            return;
        }

        i2sSetPins(busNumber, -1, -1, false);
        i2sDeinit(busNumber);

        heap_caps_free(I2sBuffer);

        I2sBufferSize = 0;
        I2sBuffer = nullptr;
        MaxBusDataSize = 0;
        UpdateMap = 0;
        UpdateMapMask = 0;
        BusCount = 0;
    }
};

// $REVIEW duplicate/refactor to support i2s bus one also
//
class NeoEsp32I2sMuxBus
{
public:    
    NeoEsp32I2sMuxBus(uint8_t _I2sBusNumber, NeoEspI2sContext& _context) :
        _muxId(NeoEspI2sContext::InvalidMuxId),
        s_context(_context),
        I2sBusNumber(_I2sBusNumber)
    {
    }

    void RegisterNewMuxBus(size_t dataSize)
    {
        _muxId = s_context.RegisterNewMuxBus(dataSize);
    }

    void Initialize(uint8_t pin, uint32_t i2sSampleRate, bool invert)
    {
        s_context.Construct(I2sBusNumber, i2sSampleRate);
        i2sSetPins(I2sBusNumber, pin, _muxId, invert);

        Serial.print(" muxid ");
        Serial.println(_muxId);
    }

    void DeregisterMuxBus()
    {
        if (s_context.DeregisterMuxBus(_muxId))
        {
            s_context.Destruct(I2sBusNumber);
        }
        // disconnect muxed pin?
        _muxId = NeoEspI2sContext::InvalidMuxId;
    }

    void StartWrite()
    {
        if (s_context.IsAllMuxBusesUpdated())
        {
            Serial.println("writing");
            s_context.ResetMuxBusesUpdated();
            i2sWrite(I2sBusNumber, reinterpret_cast<uint8_t*>(s_context.I2sBuffer), s_context.I2sBufferSize, false, false);
        }
    }

    bool IsWriteDone()
    {
        return i2sWriteDone(I2sBusNumber);
    }

    void FillBuffers(const uint8_t* data, size_t  sizeData)
    {
        // 8 channel bits layout for DMA 32bit value
        //
        //  mux bus id     01234567 01234567 01234567 01234567
        //  encode bit #   0        1        2        3
        //  value zero     1        0        0        0
        //  value one      1        1        1        0    

        // due to indianess between peripheral and cpu, bytes within the words are swapped in the const
        const uint32_t EncodedZeroBit = 0x00800000;
        const uint32_t EncodedOneBit = 0x80800080;
        const uint32_t EncodedBitMask = 0x80808080;

        const uint64_t EncodedZeroBit64 = 0x0000000000000001;
        const uint64_t EncodedOneBit64 = 0x0000000100010001;
        const uint64_t EncodedBitMask64 = 0x0001000100010001;

        const uint64_t EncodedZeroBit64Inv = 0x0000000001000000;
        const uint64_t EncodedOneBit64Inv = 0x0100000001000100;
        const uint64_t EncodedBitMask64Inv = 0x0100010001000100;

        uint32_t* pDma = s_context.I2sBuffer;
        uint64_t* pDma64 = reinterpret_cast<uint64_t*>(s_context.I2sBuffer);

        const uint8_t* pEnd = data + sizeData;
        for (const uint8_t* pPixel = data; pPixel < pEnd; pPixel++)
        {
            uint8_t value = *pPixel;

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                // $REVIEW to revaulate. These are the sorts of things that would get templatized.
                if (I2sBusNumber == 0)
                {   
                    uint64_t dma64 = *(pDma64);
                    // clear previous data for mux bus

                    #if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
                       dma64 &= ~(EncodedBitMask64 << (_muxId));
                       dma64 |= (((value & 0x80) ? EncodedOneBit64 : EncodedZeroBit64) << (_muxId));
                    #else
                       dma64 &= ~(EncodedBitMask64Inv << (_muxId));
                       dma64 |= (((value & 0x80) ? EncodedOneBit64Inv : EncodedZeroBit64Inv) << (_muxId));
                    #endif
                    *(pDma64++) = dma64;
                }
                else
                {
                    uint32_t dma = *(pDma);
                    // clear previous data for mux bus
                    dma &= ~(EncodedBitMask >> (7-_muxId));
                    // apply new data for mux bus
                    dma |= (((value & 0x80) ? EncodedOneBit : EncodedZeroBit) >> (7-_muxId));
                    *(pDma++) = dma;
                }

                value <<= 1;
            }
        }

        s_context.MarkMuxBusUpdated(_muxId);
    }

    void MarkUpdated()
    {
        s_context.MarkMuxBusUpdated(_muxId);
    }

protected:
    uint8_t I2sBusNumber;

private:
    NeoEspI2sContext& s_context;
    uint8_t _muxId; 
};


#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
    class NeoEsp32I2s0Mux8Bus : public NeoEsp32I2sMuxBus
    {
    public:
        NeoEsp32I2s0Mux8Bus() : NeoEsp32I2sMuxBus(0, s_context0)
        {
        }
    private:
        static NeoEspI2sContext s_context0;
    };
#else
    class NeoEsp32I2s0Mux16Bus : public NeoEsp32I2sMuxBus
    {
    public:
        NeoEsp32I2s0Mux16Bus() : NeoEsp32I2sMuxBus(0, s_context0)
        {
        }
    private:
        static NeoEspI2sContext s_context0;
    };

    class NeoEsp32I2s1Mux8Bus : public NeoEsp32I2sMuxBus
    {
    public:
        NeoEsp32I2s1Mux8Bus() : NeoEsp32I2sMuxBus(1, s_context1)
        {
        }
    private:
        static NeoEspI2sContext s_context1;
    };
#endif



template<typename T_SPEED, typename T_BUS, typename T_INVERT> class NeoEsp32I2sXMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32I2sXMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin)
    {
        Serial.print("_sizeData = ");
        Serial.println(_sizeData);
        
        _bus.RegisterNewMuxBus(_sizeData + T_SPEED::ResetTimeUs / T_SPEED::ByteSendTimeUs);        
    }

    ~NeoEsp32I2sXMethodBase()
    {
        while (!_bus.IsWriteDone())
        {
            yield();
        }

        _bus.DeregisterMuxBus();

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
        _bus.StartWrite(); // only triggers actual write after all mux busses have updated
    }

    void MarkUpdated()
    {
        _bus.MarkUpdated();
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

// NORMAL
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Ws2812Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Sk6812Method;
#else
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Ws2812Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8Ws2812Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Sk6812Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8Sk6812Method;
#endif


// INVERTED
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8Ws2812InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8Sk6812InvertedMethod;    
#else
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16Ws2812InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8Ws2812InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16Sk6812InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8Sk6812InvertedMethod;
#endif

#endif