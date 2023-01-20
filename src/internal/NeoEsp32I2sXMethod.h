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

//
// true size of mux channel, 8 bit
//
class NeoEspI2sMuxBusSize8Bit
{
public:
    NeoEspI2sMuxBusSize8Bit() {};

    const static size_t MuxBusDataSize = 1;
};

//
// true size of mux channel, 16 bit
//
class NeoEspI2sMuxBusSize16Bit
{
public:
    NeoEspI2sMuxBusSize16Bit() {};

    const static size_t MuxBusDataSize = 2;
};

//
// tracks mux channels used and if updated
// 
// T_FLAG - type used to store bit flags, UINT8_t for 8 channels, UINT16_t for 16
// T_MUXSIZE - true size of mux channel = NeoEspI2sMuxBusSize8Bit or NeoEspI2sMuxBusSize16Bit
//
template<typename T_FLAG, typename T_MUXSIZE> class NeoEspI2sMuxMap : public T_MUXSIZE
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
    NeoEspI2sMuxMap() 
    //    //:
    //    //MaxBusDataSize(0),
    //    //UpdateMap(0),
    //    //UpdateMapMask(0),
    //    //BusCount(0)
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

//
// Implementation of a Double Buffered version of a I2sContext
// Manages the underlying I2S details including the buffer(s)
// This creates a front buffer that can be filled while actively sending
// the back buffer, thus improving async operation of the i2s DMA.
// Note that the back buffer must be DMA memory, a limited resource, so
// the front buffer uses normal memory and copies rather than swap pointers
// 
// T_MUXMAP - NeoEspI2sMuxMap - tracking class for mux state
//
template<typename T_MUXMAP> class NeoEspI2sDblBuffContext 
{
public:
    const static size_t DmaBitsPerPixelBit = 4;

    size_t I2sBufferSize; // total size of I2sBuffer
    uint8_t* I2sBuffer;    // holds the DMA buffer that is referenced by I2sBufDesc
    uint8_t* I2sEditBuffer; // hold a editable buffer that is copied to I2sBuffer
    T_MUXMAP MuxMap;

    // as a static instance, all members get initialized to zero
    // and the constructor is called at inconsistent time to other globals
    // so its not useful to have or rely on, 
    // but without it presence they get zeroed far too late
    NeoEspI2sDblBuffContext() 
    //    //:
    //    //I2sBufferSize(0),
    //    //I2sBuffer(nullptr),
    //    //I2sEditBuffer(nullptr),
    //    //MuxMap()
    {
    }

    void Construct(const uint8_t busNumber, uint32_t i2sSampleRate)
    {
        // construct only once on first time called
        if (I2sBuffer == nullptr)
        {
            // MuxMap.MaxBusDataSize = max size in bytes of a single channel
            // DmaBitsPerPixelBit = how many dma bits/byte are needed for each source (pixel) bit/byte
            // T_MUXMAP::MuxBusDataSize = the true size of data for selected mux mode (not exposed size as i2s0 only supports 16bit mode)
            I2sBufferSize = MuxMap.MaxBusDataSize * 8 * DmaBitsPerPixelBit * T_MUXMAP::MuxBusDataSize;

            // must have a 4 byte aligned buffer for i2s
            uint32_t alignment = I2sBufferSize % 4;
            if (alignment)
            {
                I2sBufferSize += 4 - alignment;
            }

            size_t dmaBlockCount = (I2sBufferSize + I2S_DMA_MAX_DATA_LEN - 1) / I2S_DMA_MAX_DATA_LEN;

            // parallel modes need higher frequency on esp32
#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)           
            if (T_MUXMAP::MuxBusDataSize == 1 || T_MUXMAP::MuxBusDataSize == 2)
            {
                // Does this scale to 24 bit mode? 
                // true 8 bits mode uses half the frequency than 16 bit parallel mode
                i2sSampleRate *= (T_MUXMAP::MuxBusDataSize);
            }
#endif

            I2sBuffer = static_cast<uint8_t*>(heap_caps_malloc(I2sBufferSize, MALLOC_CAP_DMA));
            if (I2sBuffer == nullptr)
            {
                log_e("send buffer memory allocation failure (size %u)",
                    I2sBufferSize);
            }
            memset(I2sBuffer, 0x00, I2sBufferSize);

            I2sEditBuffer = static_cast<uint8_t*>(malloc(I2sBufferSize));
            if (I2sEditBuffer == nullptr)
            {
                log_e("edit buffer memory allocation failure (size %u)",
                    I2sBufferSize);
            }
            memset(I2sEditBuffer, 0x00, I2sBufferSize);

            i2sInit(busNumber,
                true,
                T_MUXMAP::MuxBusDataSize * 8,
                i2sSampleRate,
                I2S_CHAN_RIGHT_TO_LEFT,
                I2S_FIFO_16BIT_SINGLE,
                dmaBlockCount,
                I2sBuffer,
                I2sBufferSize);
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

        free(I2sEditBuffer);
        heap_caps_free(I2sBuffer);

        I2sBufferSize = 0;
        I2sBuffer = nullptr;
        I2sEditBuffer = nullptr;

        MuxMap.Reset();
    }
};

//
// Implementation of the low level interface into i2s mux bus
// 
// T_BUSCONTEXT - the context to use, currently only NeoEspI2sDblBuffContext but there is
//      a plan to provide one that doesn't implement the front buffer but would be less
//      async as it would have to wait until the last frame was completely sent before
//      updating and new data
// T_BUS - the bus id, NeoEsp32I2sBusZero, NeoEsp32I2sBusOne
//
template<typename T_BUSCONTEXT, typename T_BUS> class NeoEsp32I2sMuxBus
{
public:    
    NeoEsp32I2sMuxBus() :
        _muxId(s_context.MuxMap.InvalidMuxId)
    {
    }

    void RegisterNewMuxBus(size_t dataSize)
    {
        _muxId = s_context.MuxMap.RegisterNewMuxBus(dataSize);
    }

    void Initialize(uint8_t pin, uint32_t i2sSampleRate, bool invert)
    {
        s_context.Construct(T_BUS::I2sBusNumber, i2sSampleRate);
        i2sSetPins(T_BUS::I2sBusNumber, pin, _muxId, invert);
    }

    void DeregisterMuxBus()
    {
        if (s_context.MuxMap.DeregisterMuxBus(_muxId))
        {
            s_context.Destruct(T_BUS::I2sBusNumber);
        }
        // disconnect muxed pin?
        _muxId = s_context.MuxMap.InvalidMuxId;
    }

    void StartWrite()
    {
        if (s_context.MuxMap.IsAllMuxBusesUpdated())
        {
            s_context.MuxMap.ResetMuxBusesUpdated();

            // wait for not actively sending data
            while (!IsWriteDone())
            {
                yield();
            }
            // copy edit buffer to sending buffer
            memcpy(s_context.I2sBuffer, s_context.I2sEditBuffer, s_context.I2sBufferSize);
            i2sWrite(T_BUS::I2sBusNumber);
        }
    }

    bool IsWriteDone()
    {
        return i2sWriteDone(T_BUS::I2sBusNumber);
    }

    void FillBuffers(const uint8_t* data, size_t sizeData)
    {
        if (s_context.MuxMap.IsNoMuxBusesUpdate())
        {
            // clear all the data in preperation for each mux channel to add
            memset(s_context.I2sEditBuffer, 0x00, s_context.I2sBufferSize);
        }

        // 8 channel bits layout for DMA 32bit value
        //
        //  mux bus id     01234567 01234567 01234567 01234567
        //  encode bit #   0        1        2        3
        //  value zero     1        0        0        0
        //  value one      1        1        1        0    
        
        // due to indianess between peripheral and cpu, bytes within the words are swapped in the const
        const uint32_t EncodedZeroBit = 0x00010000;
        const uint32_t EncodedOneBit = 0x01010001;

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
        const uint64_t EncodedZeroBit64 = 0x0000000000000001;
        const uint64_t EncodedOneBit64 = 0x0000000100010001;
#else
        const uint64_t EncodedZeroBit64Inv = 0x0000000001000000;
        const uint64_t EncodedOneBit64Inv = 0x0100000001000100;
#endif

        uint32_t* pDma = reinterpret_cast<uint32_t*>(s_context.I2sEditBuffer);
        uint64_t* pDma64 = reinterpret_cast<uint64_t*>(s_context.I2sEditBuffer);

        const uint8_t* pEnd = data + sizeData;

        for (const uint8_t* pPixel = data; pPixel < pEnd; pPixel++)
        {
            uint8_t value = *pPixel;

            // $REVIEW - while 16 bit parrallel is support
            // this fill buffers only works for 8 bits
            // due to this for loop!
            for (uint8_t bit = 0; bit < 8; bit++)
            {
                if (s_context.MuxMap.MuxBusDataSize == 2)
                {  
                    // 16 bit mux
                    uint64_t dma64 = *(pDma64);

#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
                    dma64 |= (((value & 0x80) ? EncodedOneBit64 : EncodedZeroBit64) << (_muxId));
#else
                    dma64 |= (((value & 0x80) ? EncodedOneBit64Inv : EncodedZeroBit64Inv) << (_muxId));
#endif
                    *(pDma64++) = dma64;
                }
                else
                {
                    // 8 bit mux
                    uint32_t dma = *(pDma);
                    
                    dma |= (((value & 0x80) ? EncodedOneBit : EncodedZeroBit) << (_muxId));

                    *(pDma++) = dma;
                }

                value <<= 1;
            }
        }

        s_context.MuxMap.MarkMuxBusUpdated(_muxId);
    }

    void MarkUpdated()
    {
        s_context.MuxMap.MarkMuxBusUpdated(_muxId);
    }

private:
    static T_BUSCONTEXT s_context;
    uint8_t _muxId; 
};

template<typename T_BUSCONTEXT, typename T_BUS> T_BUSCONTEXT NeoEsp32I2sMuxBus<T_BUSCONTEXT, T_BUS>::s_context = T_BUSCONTEXT();

//
// wrapping layer of the i2s mux bus as a NeoMethod
// 
// T_SPEED - NeoEsp32I2sSpeed* (ex NeoEsp32I2sSpeedWs2812x) used to define output signal form
// T_BUS - NeoEsp32I2sMuxBus, the bus to use
// T_INVERT - NeoEsp32I2sNotInverted or NeoEsp32I2sInverted, will invert output signal
//
template<typename T_SPEED, typename T_BUS, typename T_INVERT> class NeoEsp32I2sXMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32I2sXMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin),
        _bus()
    {
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

typedef NeoEsp32I2sMuxBus<NeoEspI2sDblBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize16Bit>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux8Bus;
typedef NeoEsp32I2sMuxBus<NeoEspI2sDblBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux16Bus;


// NORMAL
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)

    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Ws2812xMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Sk6812Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8Tm1814Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8Tm1829Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8Tm1914Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8800KbpsMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8400KbpsMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Apa106Method;

    typedef NeoEsp32I2s0X8Ws2812xMethod NeoEsp32I2s0X8Ws2813Method;
    typedef NeoEsp32I2s0X8Ws2812xMethod NeoEsp32I2s0X8Ws2812dMethod;
    typedef NeoEsp32I2s0X8Ws2812xMethod NeoEsp32I2s0X8Ws2811Method;
    typedef NeoEsp32I2s0X8Ws2812xMethod NeoEsp32I2s0X8Ws2816Method;
    typedef NeoEsp32I2s0X8800KbpsMethod NeoEsp32I2s0X8Ws2812Method;
    typedef NeoEsp32I2s0X8Sk6812Method NeoEsp32I2s0X8Sk6812Method;
    typedef NeoEsp32I2s0X8Tm1814Method NeoEsp32I2s0X8Tm1814Method;
    typedef NeoEsp32I2s0X8Tm1829Method NeoEsp32I2s0X8Tm1829Method;
    typedef NeoEsp32I2s0X8Tm1914Method NeoEsp32I2s0X8Tm1914Method;
    typedef NeoEsp32I2s0X8Sk6812Method NeoEsp32I2s0X8Lc8812Method;
    typedef NeoEsp32I2s0X8Apa106Method NeoEsp32I2s0X8Apa106Method;

#else

typedef NeoEsp32I2sMuxBus<NeoEspI2sDblBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit>>, NeoEsp32I2sBusOne> NeoEsp32I2s1Mux8Bus;
typedef NeoEsp32I2sMuxBus<NeoEspI2sDblBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit>>, NeoEsp32I2sBusOne> NeoEsp32I2s1Mux16Bus;

    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Ws2812xMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Sk6812Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16Tm1814Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16Tm1829Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16Tm1914Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16800KbpsMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16400KbpsMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Apa106Method;

    typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2813Method;
    typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2812dMethod;
    typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2811Method;
    typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2816Method;
    typedef NeoEsp32I2s0X16800KbpsMethod NeoEsp32I2s0X16Ws2812Method;
    typedef NeoEsp32I2s0X16Sk6812Method NeoEsp32I2s0X16Sk6812Method;
    typedef NeoEsp32I2s0X16Tm1814Method NeoEsp32I2s0X16Tm1814Method;
    typedef NeoEsp32I2s0X16Tm1829Method NeoEsp32I2s0X16Tm1829Method;
    typedef NeoEsp32I2s0X16Tm1914Method NeoEsp32I2s0X16Tm1914Method;
    typedef NeoEsp32I2s0X16Sk6812Method NeoEsp32I2s0X16Lc8812Method;
    typedef NeoEsp32I2s0X16Apa106Method NeoEsp32I2s0X16Apa106Method;

    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8Ws2812xMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8Sk6812Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8Tm1814Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8Tm1829Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8Tm1914Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8800KbpsMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8400KbpsMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8Apa106Method;

    typedef NeoEsp32I2s1X8Ws2812xMethod NeoEsp32I2s1X8Ws2813Method;
    typedef NeoEsp32I2s1X8Ws2812xMethod NeoEsp32I2s1X8Ws2812dMethod;
    typedef NeoEsp32I2s1X8Ws2812xMethod NeoEsp32I2s1X8Ws2811Method;
    typedef NeoEsp32I2s1X8Ws2812xMethod NeoEsp32I2s1X8Ws2816Method;
    typedef NeoEsp32I2s1X8800KbpsMethod NeoEsp32I2s1X8Ws2812Method;
    typedef NeoEsp32I2s1X8Sk6812Method NeoEsp32I2s1X8Sk6812Method;
    typedef NeoEsp32I2s1X8Tm1814Method NeoEsp32I2s1X8Tm1814Method;
    typedef NeoEsp32I2s1X8Tm1829Method NeoEsp32I2s1X8Tm1829Method;
    typedef NeoEsp32I2s1X8Tm1914Method NeoEsp32I2s1X8Tm1914Method;
    typedef NeoEsp32I2s1X8Sk6812Method NeoEsp32I2s1X8Lc8812Method;
    typedef NeoEsp32I2s1X8Apa106Method NeoEsp32I2s1X8Apa106Method;

    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X16Ws2812xMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X16Sk6812Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X16Tm1814Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X16Tm1829Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X16Tm1914Method;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X16800KbpsMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X16400KbpsMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X16Apa106Method;

    typedef NeoEsp32I2s1X16Ws2812xMethod NeoEsp32I2s1X16Ws2813Method;
    typedef NeoEsp32I2s1X16Ws2812xMethod NeoEsp32I2s1X16Ws2812dMethod;
    typedef NeoEsp32I2s1X16Ws2812xMethod NeoEsp32I2s1X16Ws2811Method;
    typedef NeoEsp32I2s1X16Ws2812xMethod NeoEsp32I2s1X16Ws2816Method;
    typedef NeoEsp32I2s1X16800KbpsMethod NeoEsp32I2s1X16Ws2812Method;
    typedef NeoEsp32I2s1X16Sk6812Method NeoEsp32I2s1X16Sk6812Method;
    typedef NeoEsp32I2s1X16Tm1814Method NeoEsp32I2s1X16Tm1814Method;
    typedef NeoEsp32I2s1X16Tm1829Method NeoEsp32I2s1X16Tm1829Method;
    typedef NeoEsp32I2s1X16Tm1914Method NeoEsp32I2s1X16Tm1914Method;
    typedef NeoEsp32I2s1X16Sk6812Method NeoEsp32I2s1X16Lc8812Method;
    typedef NeoEsp32I2s1X16Apa106Method NeoEsp32I2s1X16Apa106Method;
#endif


// INVERTED
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)

    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8Ws2812xInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8Sk6812InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Tm1814InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Tm1829InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X8Tm1914InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8800KbpsInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8400KbpsInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2s0Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X8Apa106InvertedMethod;

    typedef NeoEsp32I2s0X8Ws2812xInvertedMethod NeoEsp32I2s0X8Ws2813InvertedMethod;
    typedef NeoEsp32I2s0X8Ws2812xInvertedMethod NeoEsp32I2s0X8Ws2812xInvertedMethod;
    typedef NeoEsp32I2s0X8Ws2812xInvertedMethod NeoEsp32I2s0X8Ws2811InvertedMethod;
    typedef NeoEsp32I2s0X8Ws2812xInvertedMethod NeoEsp32I2s0X8Ws2816InvertedMethod;
    typedef NeoEsp32I2s0X8800KbpsInvertedMethod NeoEsp32I2s0X8Ws2812InvertedMethod;
    typedef NeoEsp32I2s0X8Sk6812InvertedMethod NeoEsp32I2s0X8Sk6812InvertedMethod;
    typedef NeoEsp32I2s0X8Tm1814InvertedMethod NeoEsp32I2s0X8Tm1814InvertedMethod;
    typedef NeoEsp32I2s0X8Tm1829InvertedMethod NeoEsp32I2s0X8Tm1829InvertedMethod;
    typedef NeoEsp32I2s0X8Tm1914InvertedMethod NeoEsp32I2s0X8Tm1914InvertedMethod;
    typedef NeoEsp32I2s0X8Sk6812InvertedMethod NeoEsp32I2s0X8Lc8812InvertedMethod;
    typedef NeoEsp32I2s0X8Apa106InvertedMethod NeoEsp32I2s0X8Apa106InvertedMethod;

#else

    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16Ws2812xInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16Sk6812InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Tm1814InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Tm1829InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Tm1914InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16800KbpsInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16400KbpsInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s0X16Apa106InvertedMethod;
    
    typedef NeoEsp32I2s0X16Ws2812xInvertedMethod NeoEsp32I2s0X16Ws2813InvertedMethod;
    typedef NeoEsp32I2s0X16Ws2812xInvertedMethod NeoEsp32I2s0X16Ws2812xInvertedMethod;
    typedef NeoEsp32I2s0X16Ws2812xInvertedMethod NeoEsp32I2s0X16Ws2811InvertedMethod;
    typedef NeoEsp32I2s0X16Ws2812xInvertedMethod NeoEsp32I2s0X16Ws2816InvertedMethod;
    typedef NeoEsp32I2s0X16800KbpsInvertedMethod NeoEsp32I2s0X16Ws2812InvertedMethod;
    typedef NeoEsp32I2s0X16Sk6812InvertedMethod NeoEsp32I2s0X16Sk6812InvertedMethod;
    typedef NeoEsp32I2s0X16Tm1814InvertedMethod NeoEsp32I2s0X16Tm1814InvertedMethod;
    typedef NeoEsp32I2s0X16Tm1829InvertedMethod NeoEsp32I2s0X16Tm1829InvertedMethod;
    typedef NeoEsp32I2s0X16Tm1914InvertedMethod NeoEsp32I2s0X16Tm1914InvertedMethod;
    typedef NeoEsp32I2s0X16Sk6812InvertedMethod NeoEsp32I2s0X16Lc8812InvertedMethod;
    typedef NeoEsp32I2s0X16Apa106InvertedMethod NeoEsp32I2s0X16Apa106InvertedMethod;

    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8Ws2812xInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8Sk6812InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8Tm1814InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8Tm1829InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X8Tm1914InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8800KbpsInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8400KbpsInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2s1Mux8Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X8Apa106InvertedMethod;
    
    typedef NeoEsp32I2s1X8Ws2812xInvertedMethod NeoEsp32I2s1X8Ws2813InvertedMethod;
    typedef NeoEsp32I2s1X8Ws2812xInvertedMethod NeoEsp32I2s1X8Ws2812xInvertedMethod;
    typedef NeoEsp32I2s1X8Ws2812xInvertedMethod NeoEsp32I2s1X8Ws2811InvertedMethod;
    typedef NeoEsp32I2s1X8Ws2812xInvertedMethod NeoEsp32I2s1X8Ws2816InvertedMethod;
    typedef NeoEsp32I2s1X8800KbpsInvertedMethod NeoEsp32I2s1X8Ws2812InvertedMethod;
    typedef NeoEsp32I2s1X8Sk6812InvertedMethod NeoEsp32I2s1X8Sk6812InvertedMethod;
    typedef NeoEsp32I2s1X8Tm1814InvertedMethod NeoEsp32I2s1X8Tm1814InvertedMethod;
    typedef NeoEsp32I2s1X8Tm1829InvertedMethod NeoEsp32I2s1X8Tm1829InvertedMethod;
    typedef NeoEsp32I2s1X8Tm1914InvertedMethod NeoEsp32I2s1X8Tm1914InvertedMethod;
    typedef NeoEsp32I2s1X8Sk6812InvertedMethod NeoEsp32I2s1X8Lc8812InvertedMethod;
    typedef NeoEsp32I2s1X8Apa106InvertedMethod NeoEsp32I2s1X8Apa106InvertedMethod;

    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X16Ws2812xInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X16Sk6812InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X16Tm1814InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X16Tm1829InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s1X16Tm1914InvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X16800KbpsInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X16400KbpsInvertedMethod;
    typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted> NeoEsp32I2s1X16Apa106InvertedMethod;

    typedef NeoEsp32I2s1X16Ws2812xInvertedMethod NeoEsp32I2s1X16Ws2813InvertedMethod;
    typedef NeoEsp32I2s1X16Ws2812xInvertedMethod NeoEsp32I2s1X16Ws2812xInvertedMethod;
    typedef NeoEsp32I2s1X16Ws2812xInvertedMethod NeoEsp32I2s1X16Ws2811InvertedMethod;
    typedef NeoEsp32I2s1X16Ws2812xInvertedMethod NeoEsp32I2s1X16Ws2816InvertedMethod;
    typedef NeoEsp32I2s1X16800KbpsInvertedMethod NeoEsp32I2s1X16Ws2812InvertedMethod;
    typedef NeoEsp32I2s1X16Sk6812InvertedMethod NeoEsp32I2s1X16Sk6812InvertedMethod;
    typedef NeoEsp32I2s1X16Tm1814InvertedMethod NeoEsp32I2s1X16Tm1814InvertedMethod;
    typedef NeoEsp32I2s1X16Tm1829InvertedMethod NeoEsp32I2s1X16Tm1829InvertedMethod;
    typedef NeoEsp32I2s1X16Tm1914InvertedMethod NeoEsp32I2s1X16Tm1914InvertedMethod;
    typedef NeoEsp32I2s1X16Sk6812InvertedMethod NeoEsp32I2s1X16Lc8812InvertedMethod;
    typedef NeoEsp32I2s1X16Apa106InvertedMethod NeoEsp32I2s1X16Apa106InvertedMethod;
#endif

#endif