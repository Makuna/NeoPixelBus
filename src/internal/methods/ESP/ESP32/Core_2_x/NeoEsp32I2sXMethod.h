#pragma once

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

// ESP32C3/S3 I2S is not supported yet due to significant changes to interface
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)

extern "C"
{
#include <rom/gpio.h>
#include "Esp32_i2s.h"
}

#pragma once

// ESP32 Endian Map
// uint16_t
//   1234
//   3412
// uint32_t
//   12345678
//   78563412
// uint64_t
//   0123456789abcdef
//   efcdab8967452301


// 3 step cadence, so pulses are 1/3 and 2/3 of pulse width
//
class NeoEspI2sMuxBusSize8Bit3Step
{
public:
    NeoEspI2sMuxBusSize8Bit3Step() {};

    const static size_t MuxBusDataSize = 1;
    const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches encoding

    // by using a 3 step cadence, the dma data can't be updated with a single OR operation as
    //    its value resides across a non-uint16_t aligned 3 element type, so it requires two separate OR
    //    operations to update a single pixel bit, the last element can be skipped as its always 0
    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
        uint8_t* pDma = dmaBuffer;
        const uint8_t* pValue = data;
        const uint8_t* pEnd = pValue + sizeData;
        const uint8_t muxBit = 0x1 << muxId;
#if defined(CONFIG_IDF_TARGET_ESP32S2)
        const uint8_t offsetMap[] = { 0, 1, 2, 3 }; // i2s sample is two 16bit values

#else
        const uint8_t offsetMap[] = { 2,3,0,1 }; // i2s sample is two 16bit values

#endif

        uint8_t offset = 0;

        while (pValue < pEnd)
        {
            uint8_t value = *(pValue++);

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                // first cadence step init to 1
                pDma[offsetMap[offset]] |= muxBit;
                offset++;
                if (offset > 3)
                {
                    offset %= 4;
                    pDma += 4;
                }
                
                // second cadence step set based on bit
                if (value & 0x80)
                {
                    pDma[offsetMap[offset]] |= muxBit;
                }
                // last cadence step already init to 0, skip it
                offset += 2;
                if (offset > 3)
                {
                    offset %= 4;
                    pDma += 4;
                }

                // Next
                value <<= 1;
            }
        }
    }
};


// 3 step cadence, so pulses are 1/3 and 2/3 of pulse width
//
class NeoEspI2sMuxBusSize16Bit3Step
{
public:
    NeoEspI2sMuxBusSize16Bit3Step() {};

    const static size_t MuxBusDataSize = 2;
    const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches encoding

    // by using a 3 step cadence, the dma data can't be updated with a single OR operation as
    //    its value resides across a non-uint32_t aligned 3 element type, so it requires two seperate OR
    //    operations to update a single pixel bit, the last element can be skipped as its always 0
    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
        uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
        const uint8_t* pValue = data;
        const uint8_t* pEnd = pValue + sizeData;
        const uint16_t muxBit = 0x1 << muxId;
#if defined(CONFIG_IDF_TARGET_ESP32S2)
        const uint8_t offsetMap[] = { 0, 1, 2, 3 }; // i2s sample is two 16bit values
#else
        const uint8_t offsetMap[] = { 1, 0, 3, 2 }; // i2s sample is two 16bit values
#endif
        uint8_t offset = 0;

        while (pValue < pEnd)
        {
            uint8_t value = *(pValue++);

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                // first cadence step init to 1
                pDma[offsetMap[offset]] |= muxBit;
                offset++;
                if (offset > 3)
                {
                    offset %= 4;
                    pDma += 4;
                }

                // second cadence step set based on bit
                if (value & 0x80)
                {
                    pDma[offsetMap[offset]] |= muxBit;
                }
                offset++;

                // last cadence step already 0, skip it
                offset++;
                if (offset > 3)
                {
                    offset %= 4;
                    pDma += 4;
                }

                // Next
                value <<= 1;
            }
        }
    }
};


// 4 step cadence, so pulses are 1/4 and 3/4 of pulse width
//
class NeoEspI2sMuxBusSize8Bit4Step
{
public:
    NeoEspI2sMuxBusSize8Bit4Step() {};

    const static size_t MuxBusDataSize = 1;
    const static size_t DmaBitsPerPixelBit = 4; // 4 step cadence, matches encoding

    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
#if defined(CONFIG_IDF_TARGET_ESP32S2)

        const uint32_t EncodedZeroBit = 0x00000001;
        const uint32_t EncodedOneBit = 0x00010101;

#else
        //  8 channel bits layout for DMA 32bit value
        //  note, right to left
        //  mux bus bit/id     76543210 76543210 76543210 76543210
        //  encode bit #       3        2        1        0
        //  value zero         0        0        0        1
        //  value one          0        1        1        1    
        //
        // due to indianness between peripheral and cpu, bytes within the words are swapped in the const
        // 1234  - order
        // 3412  = actual due to endianness
        //                                00000001
        const uint32_t EncodedZeroBit = 0x00010000;
        //                               00010101
        const uint32_t EncodedOneBit = 0x01010001;
#endif

        uint32_t* pDma = reinterpret_cast<uint32_t*>(dmaBuffer);
        const uint8_t* pEnd = data + sizeData;
        const uint32_t OneBit = EncodedOneBit << muxId;
        const uint32_t ZeroBit = EncodedZeroBit << muxId;

        for (const uint8_t* pPixel = data; pPixel < pEnd; pPixel++)
        {
            uint8_t value = *pPixel;

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint32_t dma = *(pDma);

                dma |= (value & 0x80) ? OneBit : ZeroBit;
                *(pDma++) = dma;
                value <<= 1;
            }
        }
    }
};

// 4 step cadence, so pulses are 1/4 and 3/4 of pulse width
//
class NeoEspI2sMuxBusSize16Bit4Step
{
public:
    NeoEspI2sMuxBusSize16Bit4Step() {};

    const static size_t MuxBusDataSize = 2;
    const static size_t DmaBitsPerPixelBit = 4; // 4 step cadence, matches encoding

    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
#if defined(CONFIG_IDF_TARGET_ESP32S2)
        const uint64_t EncodedZeroBit64 = 0x0000000000000001;
        const uint64_t EncodedOneBit64 = 0x0000000100010001;

#else
        // 1234 5678 - order
        // 3412 7856 = actual due to endianness
        // not swap                         0000000000000001 
        const uint64_t EncodedZeroBit64 = 0x0000000000010000;
        //  no swap                         0000000100010001 
        const uint64_t EncodedOneBit64 =  0x0001000000010001; 

#endif
        
        Fillx16(dmaBuffer,
            data,
            sizeData,
            muxId,
            EncodedZeroBit64,
            EncodedOneBit64);
    }

protected:
    static void Fillx16(uint8_t* dmaBuffer, 
        const uint8_t* data,
        size_t sizeData,
        uint8_t muxShift,
        const uint64_t EncodedZeroBit64,
        const uint64_t EncodedOneBit64)
    {
        uint64_t* pDma64 = reinterpret_cast<uint64_t*>(dmaBuffer);
        const uint8_t* pSrc = data;
        const uint8_t* pEnd = pSrc + sizeData;
        const uint64_t OneBit = EncodedOneBit64 << muxShift;
        const uint64_t ZeroBit = EncodedZeroBit64 << muxShift;
 
        while (pSrc < pEnd)
        {
            uint8_t value = *(pSrc++);

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint64_t dma64 = *(pDma64);

                dma64 |= (value & 0x80) ? OneBit : ZeroBit;
                *(pDma64++) = dma64;
                value <<= 1;
            }
        }
       
    }
};

//
// tracks mux channels used and if updated
// 
// T_FLAG - type used to store bit flags, UINT8_t for 8 channels, UINT16_t for 16
// T_MUXSIZE - true size of mux channel = NeoEspI2sMuxBusSize8Bit4Step or NeoEspI2sMuxBusSize16Bit4Step
//
template<typename T_FLAG, typename T_MUXSIZE> 
class NeoEspI2sMuxMap : public T_MUXSIZE
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
        //:
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
        if (muxId == BusMaxCount)
        {
            log_e("exceded channel limit of %u on bus", BusMaxCount);
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
// Implementation of a Single Buffered version of a I2sContext
// Manages the underlying I2S details including the buffer
// This creates only a actively sending back buffer, 
// Note that the back buffer must be DMA memory, a limited resource
// 
// T_MUXMAP - NeoEspI2sMuxMap - tracking class for mux state
//
template<typename T_MUXMAP> 
class NeoEspI2sMonoBuffContext 
{
public:
    size_t I2sBufferSize; // total size of I2sBuffer
    uint8_t* I2sBuffer;    // holds the DMA buffer that is referenced by I2sBufDesc
    T_MUXMAP MuxMap;

    // as a static instance, all members get initialized to zero
    // and the constructor is called at inconsistent time to other globals
    // so its not useful to have or rely on, 
    // but without it presence they get zeroed far too late
    NeoEspI2sMonoBuffContext()
        //:
    {
    }

    void Construct(const uint8_t busNumber, uint16_t nsBitSendTime)
    {
        // construct only once on first time called
        if (I2sBuffer == nullptr)
        {
            // MuxMap.MaxBusDataSize = max size in bytes of a single channel
            // DmaBitsPerPixelBit = how many dma bits/byte are needed for each source (pixel) bit/byte
            // T_MUXMAP::MuxBusDataSize = the true size of data for selected mux mode (not exposed size as i2s0 only supports 16bit mode)
            I2sBufferSize = MuxMap.MaxBusDataSize * 8 * T_MUXMAP::DmaBitsPerPixelBit * T_MUXMAP::MuxBusDataSize;

            // must have a 4 byte aligned buffer for i2s
            uint32_t alignment = I2sBufferSize % 4;
            if (alignment)
            {
                I2sBufferSize += 4 - alignment;
            }

            size_t dmaBlockCount = (I2sBufferSize + I2S_DMA_MAX_DATA_LEN - 1) / I2S_DMA_MAX_DATA_LEN;

            I2sBuffer = static_cast<uint8_t*>(heap_caps_malloc(I2sBufferSize, MALLOC_CAP_DMA));
            if (I2sBuffer == nullptr)
            {
                log_e("send buffer memory allocation failure (size %u)",
                    I2sBufferSize);
            }
            memset(I2sBuffer, 0x00, I2sBufferSize);

            i2sInit(busNumber,
                true,
                T_MUXMAP::MuxBusDataSize,
                T_MUXMAP::DmaBitsPerPixelBit,
                nsBitSendTime,
#if defined(CONFIG_IDF_TARGET_ESP32S2)
// using these modes on ESP32S2 actually allows it to function
// in both x8 and x16
                I2S_CHAN_RIGHT_TO_LEFT,
                I2S_FIFO_16BIT_SINGLE, // I2S_FIFO_32BIT_SINGLE
#else
// but they won't work on ESP32 in parallel mode, but these will
                I2S_CHAN_RIGHT_TO_LEFT,
                I2S_FIFO_16BIT_SINGLE,
#endif
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

        i2sSetPins(busNumber, -1, -1, -1, false);
        i2sDeinit(busNumber);

        heap_caps_free(I2sBuffer);

        I2sBufferSize = 0;
        I2sBuffer = nullptr;

        MuxMap.Reset();
    }

    void StartWrite(uint8_t i2sBusNumber)
    {
        if (MuxMap.IsAllMuxBusesUpdated())
        {
#if defined(NEO_DEBUG_DUMP_I2S_BUFFER)
            // dump the is2buffer
            uint8_t* pDma = I2sBuffer;
            uint8_t* pEnd = pDma + I2sBufferSize;
            size_t index = 0;

            Serial.println();
            Serial.println("NeoEspI2sMonoBuffContext - i2sBufferDump: ");
            while (pDma < pEnd)
            {
                uint8_t value = *pDma;

                // a single bit pulse of data
                if ((index % (T_MUXMAP::DmaBitsPerPixelBit * T_MUXMAP::MuxBusDataSize)) == 0)
                {
                    Serial.println();
                }

                for (uint8_t bit = 0; bit < 8; bit++)
                {
                    Serial.print((value & 0x80) ? "1" : "0");
                    value <<= 1;
                }
                Serial.print(" ");
                pDma++;
                index++;

            }
            Serial.println();

#endif // NEO_DEBUG_DUMP_I2S_BUFFER

            MuxMap.ResetMuxBusesUpdated();
            i2sWrite(i2sBusNumber);
        }
    }

    void FillBuffers(const uint8_t* data, 
            size_t sizeData, 
            uint8_t muxId,
            uint8_t i2sBusNumber)
    {
        // wait for not actively sending data
        while (!i2sWriteDone(i2sBusNumber))
        {
            yield();
        }

        // to keep the inner loops for EncodeIntoDma smaller
        // they will just OR in their values
        // so the buffer must be cleared first
        if (MuxMap.IsNoMuxBusesUpdate())
        {
            // clear all the data in preparation for each mux channel to add
            memset(I2sBuffer, 0x00, I2sBufferSize);
        }

        MuxMap.EncodeIntoDma(I2sBuffer,
            data,
            sizeData,
            muxId);

        MuxMap.MarkMuxBusUpdated(muxId);
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
template<typename T_MUXMAP>
class NeoEspI2sDblBuffContext
{
public:
    size_t I2sBufferSize; // total size of I2sBuffer
    uint8_t* I2sBuffer;    // holds the DMA buffer that is referenced by I2sBufDesc
    uint8_t* I2sEditBuffer; // hold a editable buffer that is copied to I2sBuffer
    T_MUXMAP MuxMap;

    // as a static instance, all members get initialized to zero
    // and the constructor is called at inconsistent time to other globals
    // so its not useful to have or rely on, 
    // but without it presence they get zeroed far too late
    NeoEspI2sDblBuffContext()
            //:
    {
    }

    void Construct(const uint8_t busNumber, uint16_t nsBitSendTime)
    {
        // construct only once on first time called
        if (I2sBuffer == nullptr)
        {
            // MuxMap.MaxBusDataSize = max size in bytes of a single channel
            // DmaBitsPerPixelBit = how many dma bits/byte are needed for each source (pixel) bit/byte
            // T_MUXMAP::MuxBusDataSize = the true size of data for selected mux mode (not exposed size as 
            // i2s0 only supports 16bit mode)
            I2sBufferSize = MuxMap.MaxBusDataSize * 8 * T_MUXMAP::DmaBitsPerPixelBit * T_MUXMAP::MuxBusDataSize;

            // must have a 4 byte aligned buffer for i2s
            uint32_t alignment = I2sBufferSize % 4;
            if (alignment)
            {
                I2sBufferSize += 4 - alignment;
            }

            size_t dmaBlockCount = (I2sBufferSize + I2S_DMA_MAX_DATA_LEN - 1) / I2S_DMA_MAX_DATA_LEN;

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
                T_MUXMAP::MuxBusDataSize,
                T_MUXMAP::DmaBitsPerPixelBit,
                nsBitSendTime,
#if defined(CONFIG_IDF_TARGET_ESP32S2)
                // using these modes on ESP32S2 actually allows it to function
                // in both x8 and x16
                I2S_CHAN_RIGHT_TO_LEFT,
                I2S_FIFO_16BIT_SINGLE,
#else
                // but they won't work on ESP32 in parallel mode, but these will
                I2S_CHAN_RIGHT_TO_LEFT,
                I2S_FIFO_16BIT_SINGLE,
#endif
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

        i2sSetPins(busNumber, -1, -1, -1, false);
        i2sDeinit(busNumber);

        free(I2sEditBuffer);
        heap_caps_free(I2sBuffer);

        I2sBufferSize = 0;
        I2sBuffer = nullptr;
        I2sEditBuffer = nullptr;

        MuxMap.Reset();
    }

    void StartWrite(uint8_t i2sBusNumber)
    {
        if (MuxMap.IsAllMuxBusesUpdated())
        {
            MuxMap.ResetMuxBusesUpdated();

            // wait for not actively sending data
            while (!i2sWriteDone(i2sBusNumber))
            {
                yield();
            }

            // copy edit buffer to sending buffer
            memcpy(I2sBuffer, I2sEditBuffer, I2sBufferSize);

            i2sWrite(i2sBusNumber);
        }
    }

    void FillBuffers(const uint8_t* data,
        size_t sizeData,
        uint8_t muxId,
        uint8_t i2sBusNumber)
    {
        // to keep the inner loops for EncodeIntoDma smaller
        // they will just OR in their values
        // so the buffer must be cleared first
        if (MuxMap.IsNoMuxBusesUpdate())
        {
            // clear all the data in preperation for each mux channel to add
            memset(I2sEditBuffer, 0x00, I2sBufferSize);
        }

        MuxMap.EncodeIntoDma(I2sEditBuffer,
            data,
            sizeData,
            muxId);

        MuxMap.MarkMuxBusUpdated(muxId);
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
template<typename T_BUSCONTEXT, typename T_BUS> 
class NeoEsp32I2sMuxBus
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

    void Initialize(uint8_t pin, uint16_t nsBitSendTime, bool invert)
    {
        s_context.Construct(T_BUS::I2sBusNumber, nsBitSendTime);
        i2sSetPins(T_BUS::I2sBusNumber, pin, _muxId, s_context.MuxMap.MuxBusDataSize, invert);
    }

    void DeregisterMuxBus(uint8_t pin)
    {
        if (s_context.MuxMap.DeregisterMuxBus(_muxId))
        {
            s_context.Destruct(T_BUS::I2sBusNumber);
        }

        // disconnect muxed pin
        gpio_matrix_out(pin, SIG_GPIO_OUT_IDX, false, false);
        pinMode(pin, INPUT);

        _muxId = s_context.MuxMap.InvalidMuxId;
    }

    void StartWrite()
    {
        s_context.StartWrite(T_BUS::I2sBusNumber);
    }

    bool IsWriteDone() const
    {
        return i2sWriteDone(T_BUS::I2sBusNumber);
    }

    void FillBuffers(const uint8_t* data, size_t sizeData)
    {
        s_context.FillBuffers(data, sizeData, _muxId, T_BUS::I2sBusNumber);
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
// T_SPEED - NeoBitsSpeed* (ex NeoBitsSpeedWs2812x) used to define output signal form
// T_BUS - NeoEsp32I2sMuxBus, the bus to use
// T_INVERT - NeoBitsNotInverted or NeoBitsInverted, will invert output signal
//
template<typename T_SPEED, typename T_BUS, typename T_INVERT> 
class NeoEsp32I2sXMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32I2sXMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin),
        _bus()
    {
        _bus.RegisterNewMuxBus(_sizeData + T_SPEED::ResetTimeUs / T_SPEED::ByteSendTimeUs(T_SPEED::BitSendTimeNs));
    }

    ~NeoEsp32I2sXMethodBase()
    {
        while (!_bus.IsWriteDone())
        {
            yield();
        }

        _bus.DeregisterMuxBus(_pin);

        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        return _bus.IsWriteDone();
    }

    void Initialize()
    {
        _bus.Initialize(_pin, T_SPEED::BitSendTimeNs, T_INVERT::Inverted);

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

    bool SwapBuffers()
    {
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
    const size_t  _sizeData;    // Size of '_data' buffer 
    const uint8_t _pin;         // output pin number

    T_BUS _bus;          // holds instance for mux bus support
    uint8_t* _data;      // Holds LED color values
};

#if defined(NPB_CONF_4STEP_CADENCE)

//------------------------------------
#if defined(CONFIG_IDF_TARGET_ESP32S2)

typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit4Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux8Bus;
typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit4Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux16Bus;

typedef NeoEsp32I2sMuxBus<NeoEspI2sDblBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit4Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0DblMux8Bus;

typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s0DblMux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8DblWs2812xMethod;

#else

typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize16Bit4Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux8Bus;
typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit4Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux16Bus;


typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit4Step>>, NeoEsp32I2sBusOne> NeoEsp32I2s1Mux8Bus;
typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit4Step>>, NeoEsp32I2sBusOne> NeoEsp32I2s1Mux16Bus;

typedef NeoEsp32I2sMuxBus<NeoEspI2sDblBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit4Step>>, NeoEsp32I2sBusOne> NeoEsp32I2s1DblMux8Bus;

#endif

#else // NPB_CONF_3STEP_CADENCE

//------------------------------------
#if defined(CONFIG_IDF_TARGET_ESP32S2)

typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit3Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux8Bus;
typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit3Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux16Bus;

typedef NeoEsp32I2sMuxBus<NeoEspI2sDblBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit3Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0DblMux8Bus;

typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s0DblMux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8DblWs2812xMethod;

#else

typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize16Bit3Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux8Bus;
typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit3Step>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux16Bus;


typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit3Step>>, NeoEsp32I2sBusOne> NeoEsp32I2s1Mux8Bus;
typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit3Step>>, NeoEsp32I2sBusOne> NeoEsp32I2s1Mux16Bus;

typedef NeoEsp32I2sMuxBus<NeoEspI2sDblBuffContext<NeoEspI2sMuxMap<uint8_t, NeoEspI2sMuxBusSize8Bit3Step>>, NeoEsp32I2sBusOne> NeoEsp32I2s1DblMux8Bus;

#endif
#endif

// NORMAL
//

// I2s0x8
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8Ws2812xMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8Ws2805Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedSk6812,  NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8Sk6812Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1814,  NeoEsp32I2s0Mux8Bus, NeoBitsInverted>    NeoEsp32I2s0X8Tm1814Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1829,  NeoEsp32I2s0Mux8Bus, NeoBitsInverted>    NeoEsp32I2s0X8Tm1829Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1914,  NeoEsp32I2s0Mux8Bus, NeoBitsInverted>    NeoEsp32I2s0X8Tm1914Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8800KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8400KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedApa106,  NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8Apa106Method;

typedef NeoEsp32I2s0X8Ws2805Method NeoEsp32I2s0X8Ws2814Method;
typedef NeoEsp32I2s0X8Ws2812xMethod NeoEsp32I2s0X8Ws2813Method;
typedef NeoEsp32I2s0X8Ws2812xMethod NeoEsp32I2s0X8Ws2812dMethod;
typedef NeoEsp32I2s0X8Ws2812xMethod NeoEsp32I2s0X8Ws2811Method;
typedef NeoEsp32I2s0X8Ws2812xMethod NeoEsp32I2s0X8Ws2816Method;
typedef NeoEsp32I2s0X8800KbpsMethod NeoEsp32I2s0X8Ws2812Method;
typedef NeoEsp32I2s0X8Sk6812Method  NeoEsp32I2s0X8Lc8812Method;

// I2s0x16
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16Ws2812xMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2805,  NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16Ws2805Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedSk6812,  NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16Sk6812Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1814,  NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16Tm1814Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1829,  NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16Tm1829Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1914,  NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16Tm1914Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16800KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16400KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedApa106,  NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16Apa106Method;

typedef NeoEsp32I2s0X16Ws2805Method NeoEsp32I2s0X16Ws2814Method;
typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2813Method;
typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2812dMethod;
typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2811Method;
typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2816Method;
typedef NeoEsp32I2s0X16800KbpsMethod NeoEsp32I2s0X16Ws2812Method;
typedef NeoEsp32I2s0X16Sk6812Method  NeoEsp32I2s0X16Lc8812Method;

#if !defined(CONFIG_IDF_TARGET_ESP32S2)

// I2s1x8
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s1DblMux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8DblWs2812xMethod;

typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8Ws2812xMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8Ws2805Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedSk6812,  NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8Sk6812Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1814,  NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8Tm1814Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1829,  NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8Tm1829Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1914,  NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8Tm1914Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8800KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8400KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedApa106,  NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8Apa106Method;

typedef NeoEsp32I2s1X8Ws2805Method NeoEsp32I2s1X8Ws2814Method;
typedef NeoEsp32I2s1X8Ws2812xMethod NeoEsp32I2s1X8Ws2813Method;
typedef NeoEsp32I2s1X8Ws2812xMethod NeoEsp32I2s1X8Ws2812dMethod;
typedef NeoEsp32I2s1X8Ws2812xMethod NeoEsp32I2s1X8Ws2811Method;
typedef NeoEsp32I2s1X8Ws2812xMethod NeoEsp32I2s1X8Ws2816Method;
typedef NeoEsp32I2s1X8800KbpsMethod NeoEsp32I2s1X8Ws2812Method;
typedef NeoEsp32I2s1X8Sk6812Method  NeoEsp32I2s1X8Lc8812Method;

// I2s1x16
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16Ws2812xMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16Ws2805Method; 
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedSk6812,  NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16Sk6812Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1814,  NeoEsp32I2s1Mux16Bus, NeoBitsInverted> NeoEsp32I2s1X16Tm1814Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1829,  NeoEsp32I2s1Mux16Bus, NeoBitsInverted> NeoEsp32I2s1X16Tm1829Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1914,  NeoEsp32I2s1Mux16Bus, NeoBitsInverted> NeoEsp32I2s1X16Tm1914Method;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16800KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16400KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedApa106,  NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16Apa106Method;

typedef NeoEsp32I2s1X16Ws2805Method NeoEsp32I2s1X16Ws2814Method;
typedef NeoEsp32I2s1X16Ws2812xMethod NeoEsp32I2s1X16Ws2813Method;
typedef NeoEsp32I2s1X16Ws2812xMethod NeoEsp32I2s1X16Ws2812dMethod;
typedef NeoEsp32I2s1X16Ws2812xMethod NeoEsp32I2s1X16Ws2811Method;
typedef NeoEsp32I2s1X16Ws2812xMethod NeoEsp32I2s1X16Ws2816Method;
typedef NeoEsp32I2s1X16800KbpsMethod NeoEsp32I2s1X16Ws2812Method;
typedef NeoEsp32I2s1X16Sk6812Method NeoEsp32I2s1X16Lc8812Method;


#endif // !defined(CONFIG_IDF_TARGET_ESP32S2)

// INVERTED
//
// I2s0x8 INVERTED
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s0Mux8Bus, NeoBitsInverted> NeoEsp32I2s0X8Ws2812xInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2s0Mux8Bus, NeoBitsInverted> NeoEsp32I2s0X8Ws2805InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedSk6812, NeoEsp32I2s0Mux8Bus, NeoBitsInverted> NeoEsp32I2s0X8Sk6812InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1814, NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8Tm1814InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1829, NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8Tm1829InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1914, NeoEsp32I2s0Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s0X8Tm1914InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2s0Mux8Bus, NeoBitsInverted> NeoEsp32I2s0X8800KbpsInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2s0Mux8Bus, NeoBitsInverted> NeoEsp32I2s0X8400KbpsInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedApa106, NeoEsp32I2s0Mux8Bus, NeoBitsInverted> NeoEsp32I2s0X8Apa106InvertedMethod;

typedef NeoEsp32I2s0X8Ws2805InvertedMethod NeoEsp32I2s0X8Ws2814InvertedMethod;
typedef NeoEsp32I2s0X8Ws2812xInvertedMethod NeoEsp32I2s0X8Ws2813InvertedMethod;
typedef NeoEsp32I2s0X8Ws2812xInvertedMethod NeoEsp32I2s0X8Ws2812xInvertedMethod;
typedef NeoEsp32I2s0X8Ws2812xInvertedMethod NeoEsp32I2s0X8Ws2811InvertedMethod;
typedef NeoEsp32I2s0X8Ws2812xInvertedMethod NeoEsp32I2s0X8Ws2816InvertedMethod;
typedef NeoEsp32I2s0X8800KbpsInvertedMethod NeoEsp32I2s0X8Ws2812InvertedMethod;
typedef NeoEsp32I2s0X8Sk6812InvertedMethod  NeoEsp32I2s0X8Lc8812InvertedMethod;


// I2s0x16 INVERTED
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16Ws2812xInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16Ws2805InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedSk6812,  NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16Sk6812InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1814,  NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16Tm1814InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1829,  NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16Tm1829InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1914,  NeoEsp32I2s0Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s0X16Tm1914InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16800KbpsInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16400KbpsInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedApa106,  NeoEsp32I2s0Mux16Bus, NeoBitsInverted>    NeoEsp32I2s0X16Apa106InvertedMethod;
    
typedef NeoEsp32I2s0X16Ws2805InvertedMethod NeoEsp32I2s0X16Ws2814InvertedMethod;
typedef NeoEsp32I2s0X16Ws2812xInvertedMethod NeoEsp32I2s0X16Ws2813InvertedMethod;
typedef NeoEsp32I2s0X16Ws2812xInvertedMethod NeoEsp32I2s0X16Ws2812xInvertedMethod;
typedef NeoEsp32I2s0X16Ws2812xInvertedMethod NeoEsp32I2s0X16Ws2811InvertedMethod;
typedef NeoEsp32I2s0X16Ws2812xInvertedMethod NeoEsp32I2s0X16Ws2816InvertedMethod;
typedef NeoEsp32I2s0X16800KbpsInvertedMethod NeoEsp32I2s0X16Ws2812InvertedMethod;
typedef NeoEsp32I2s0X16Sk6812InvertedMethod  NeoEsp32I2s0X16Lc8812InvertedMethod;

#if !defined(CONFIG_IDF_TARGET_ESP32S2)

// I2s1x8 INVERTED
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8Ws2812xInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8Ws2805InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedSk6812,  NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8Sk6812InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1814,  NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8Tm1814InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1829,  NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8Tm1829InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1914,  NeoEsp32I2s1Mux8Bus, NeoBitsNotInverted> NeoEsp32I2s1X8Tm1914InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8800KbpsInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8400KbpsInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedApa106,  NeoEsp32I2s1Mux8Bus, NeoBitsInverted>    NeoEsp32I2s1X8Apa106InvertedMethod;
    
typedef NeoEsp32I2s1X8Ws2805InvertedMethod NeoEsp32I2s1X8Ws2814InvertedMethod;
typedef NeoEsp32I2s1X8Ws2812xInvertedMethod NeoEsp32I2s1X8Ws2813InvertedMethod;
typedef NeoEsp32I2s1X8Ws2812xInvertedMethod NeoEsp32I2s1X8Ws2812xInvertedMethod;
typedef NeoEsp32I2s1X8Ws2812xInvertedMethod NeoEsp32I2s1X8Ws2811InvertedMethod;
typedef NeoEsp32I2s1X8Ws2812xInvertedMethod NeoEsp32I2s1X8Ws2816InvertedMethod;
typedef NeoEsp32I2s1X8800KbpsInvertedMethod NeoEsp32I2s1X8Ws2812InvertedMethod;
typedef NeoEsp32I2s1X8Sk6812InvertedMethod  NeoEsp32I2s1X8Lc8812InvertedMethod;

// I2s1x16 INVERTED
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2s1Mux16Bus, NeoBitsInverted>    NeoEsp32I2s1X16Ws2812xInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2s1Mux16Bus, NeoBitsInverted>    NeoEsp32I2s1X16Ws2805InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedSk6812,  NeoEsp32I2s1Mux16Bus, NeoBitsInverted>    NeoEsp32I2s1X16Sk6812InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1814,  NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16Tm1814InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1829,  NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16Tm1829InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedTm1914,  NeoEsp32I2s1Mux16Bus, NeoBitsNotInverted> NeoEsp32I2s1X16Tm1914InvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2s1Mux16Bus, NeoBitsInverted>    NeoEsp32I2s1X16800KbpsInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2s1Mux16Bus, NeoBitsInverted>    NeoEsp32I2s1X16400KbpsInvertedMethod;
typedef NeoEsp32I2sXMethodBase<NeoBitsSpeedApa106,  NeoEsp32I2s1Mux16Bus, NeoBitsInverted>    NeoEsp32I2s1X16Apa106InvertedMethod;

typedef NeoEsp32I2s1X16Ws2805InvertedMethod NeoEsp32I2s1X16Ws2814InvertedMethod;
typedef NeoEsp32I2s1X16Ws2812xInvertedMethod NeoEsp32I2s1X16Ws2813InvertedMethod;
typedef NeoEsp32I2s1X16Ws2812xInvertedMethod NeoEsp32I2s1X16Ws2812xInvertedMethod;
typedef NeoEsp32I2s1X16Ws2812xInvertedMethod NeoEsp32I2s1X16Ws2811InvertedMethod;
typedef NeoEsp32I2s1X16Ws2812xInvertedMethod NeoEsp32I2s1X16Ws2816InvertedMethod;
typedef NeoEsp32I2s1X16800KbpsInvertedMethod NeoEsp32I2s1X16Ws2812InvertedMethod;
typedef NeoEsp32I2s1X16Sk6812InvertedMethod  NeoEsp32I2s1X16Lc8812InvertedMethod;

#endif // !defined(CONFIG_IDF_TARGET_ESP32S2)

#endif // defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
