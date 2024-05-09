#pragma once

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)

extern "C"
{
#include <rom/gpio.h>
//#include "Esp32_i2s.h"
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

//
// true size of mux channel, 16 bit
//
class NeoEspI2sMuxBusSize16Bit
{
public:
    NeoEspI2sMuxBusSize16Bit() {};

    const static size_t MuxBusDataSize = 2;

    static void EncodeIntoDma(uint8_t** dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
#if defined(CONFIG_IDF_TARGET_ESP32S2)
        // 1234 5678 - order
        // 3412 7856 = actual due to endianness
        // not swap                         0000000000000001 
        const uint64_t EncodedZeroBit64 = 0x0000000000010000;
        //  no swap                         0000000100010001 
        const uint64_t EncodedOneBit64 =  0x0001000000010001; 
        // can be shifted by 8!
        Fillx16(dmaBuffer,
            data,
            sizeData,
            muxId,
            EncodedZeroBit64,
            EncodedOneBit64);
#else

        // 16 channel bits layout for DMA 64bit value
        // note, right to left, destination is 32bit chunks
        // due to indianness between peripheral and cpu, 
        // bytes within the words are swapped and words within dwords
        // in the literal constants
        //  {       } {       }
        //  0123 4567 89ab cdef - order of bytes in literal constant
        //  efcd ab89 6745 2301 - order of memory on ESP32 due to Endianness
        //  6745 2301 efcd ab89 - 32bit dest means only map using 32bits so swap upper and lower
        //
        // Due to final bit locations, can't shift encoded one bit
        // either left more than 7 or right more than 7 so we have to
        // split the updates and use different encodings
        if (muxId < 8)
        {
            // endian + dest swap               0000000000000001 
            const uint64_t EncodedZeroBit64 = 0x0000000001000000;
            //  endian + dest swap             0000000100010001 
            const uint64_t EncodedOneBit64 = 0x0100000001000100; 
            // cant be shifted by 8!
            Fillx16(dmaBuffer,
                data,
                sizeData,
                muxId,
                EncodedZeroBit64,
                EncodedOneBit64);
        }
        else
        {
            // endian + dest swap               0000000000000001 
            // then pre shift by 8              0000000000000100
            const uint64_t EncodedZeroBit64 = 0x0000000000010000;
            //  endian + dest swap             0000000100010001 
            // then pre shift by 8             0000010001000100
            const uint64_t EncodedOneBit64 = 0x0001000000010001;
            Fillx16(dmaBuffer,
                data,
                sizeData,
                muxId - 8, // preshifted
                EncodedZeroBit64,
                EncodedOneBit64);
        }
#endif
    }

protected:
    static void Fillx16(uint8_t** dmaBuffer, 
        const uint8_t* data,
        size_t sizeData,
        uint8_t muxShift,
        const uint64_t EncodedZeroBit64,
        const uint64_t EncodedOneBit64)
    {
        uint64_t* pDma64 = reinterpret_cast<uint64_t*>(*dmaBuffer);
        const uint8_t* pEnd = data + sizeData;

        for (const uint8_t* pPixel = data; pPixel < pEnd; pPixel++)
        {
            uint8_t value = *pPixel;

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                uint64_t dma64 = *(pDma64);

                dma64 |= (((value & 0x80) ? EncodedOneBit64 : EncodedZeroBit64) << (muxShift));
                *(pDma64++) = dma64;
                value <<= 1;
            }
        }
        // return the buffer pointer advanced by encoding progress
        *dmaBuffer = reinterpret_cast<uint8_t*>(pDma64);
    }
};

//
// tracks mux channels used and if updated
// 
// T_FLAG - type used to store bit flags, UINT8_t for 8 channels, UINT16_t for 16
// T_MUXSIZE - true size of mux channel = NeoEspI2sMuxBusSize8Bit or NeoEspI2sMuxBusSize16Bit
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
    const static size_t DmaBitsPerPixelBit = 4;

    size_t I2sBufferSize; // total size of I2sBuffer
    uint8_t* I2sBuffer;    // holds the DMA buffer that is referenced by I2sBufDesc
    T_MUXMAP MuxMap;

    // as a static instance, all members get initialized to zero
    // and the constructor is called at inconsistent time to other globals
    // so its not useful to have or rely on, 
    // but without it presence they get zeroed far too late
    NeoEspI2sMonoBuffContext()
        //:
        //I2sBufferSize(0),
        //I2sBuffer(nullptr),
        //I2sEditBuffer(nullptr),
        //MuxMap()
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
                i2sSampleRate,
#if defined(CONFIG_IDF_TARGET_ESP32S2)
// using these modes on ESP32S2 actually allows it to function
// in both x8 and x16
                I2S_CHAN_STEREO,
                I2S_FIFO_16BIT_DUAL,
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

    void ResetBuffer()
    {
        // to keep the inner loops for EncodeIntoDma smaller
        // they will just OR in their values
        // so the buffer must be cleared first
        if (MuxMap.IsNoMuxBusesUpdate())
        {
            // clear all the data in preperation for each mux channel to add
            memset(I2sBuffer, 0x00, I2sBufferSize);
        }
    }

    void FillBuffer(uint8_t** dmaBuffer,
            const uint8_t* data, 
            size_t sizeData, 
            uint8_t muxId)
    {
        MuxMap.EncodeIntoDma(dmaBuffer,
            data,
            sizeData,
            muxId);
    }


    void StartWrite(uint8_t i2sBusNumber)
    {
        if (MuxMap.IsAllMuxBusesUpdated())
        {
            MuxMap.ResetMuxBusesUpdated();
            i2sWrite(i2sBusNumber);
        }
    }
};


//
// Implementation of the low level interface into i2s mux bus
// 
// T_BUSCONTEXT - the context to use, currently only NeoEspI2sMonoBuffContext
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

    void Initialize(uint8_t pin, uint32_t i2sSampleRate, bool invert)
    {
        s_context.Construct(T_BUS::I2sBusNumber, i2sSampleRate);
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

    bool IsWriteDone() const
    {
        return i2sWriteDone(T_BUS::I2sBusNumber);
    }

    uint8_t* BeginUpdate()
    {
        s_context.ResetBuffer();
        return s_context.I2sBuffer;
    }

    void FillBuffer(uint8_t** dmaBuffer,
        const uint8_t* data,
        size_t sizeData)
    {
        s_context.FillBuffer(dmaBuffer, data, sizeData, _muxId);
    }

    void EndUpdate()
    {
        s_context.MuxMap.MarkMuxBusUpdated(_muxId);
        s_context.StartWrite(T_BUS::I2sBusNumber); // only when all buses are update is actual write started
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
template<typename T_SPEED, typename T_BUS, typename T_INVERT> 
class NeoEsp32I2sXMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32I2sXMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _pin(pin),
        _pixelCount(pixelCount),
        _bus()
    {
        _bus.RegisterNewMuxBus((pixelCount * elementSize + settingsSize) + T_SPEED::ResetTimeUs / T_SPEED::ByteSendTimeUs);
    }

    ~NeoEsp32I2sXMethodBase()
    {
        while (!_bus.IsWriteDone())
        {
            yield();
        }

        _bus.DeregisterMuxBus(_pin);
    }

    bool IsReadyToUpdate() const
    {
        return _bus.IsWriteDone();
    }

    void Initialize()
    {
        _bus.Initialize(_pin, T_SPEED::I2sSampleRate, T_INVERT::Inverted);
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
        while (!_bus.IsWriteDone())
        {
            yield();
        }

        const size_t sendDataSize = T_COLOR_FEATURE::SettingsSize >= T_COLOR_FEATURE::PixelSize ? T_COLOR_FEATURE::SettingsSize : T_COLOR_FEATURE::PixelSize;
        uint8_t sendData[sendDataSize];
        uint8_t* data = _bus.BeginUpdate();

        // if there are settings at the front
        //
        if (T_COLOR_FEATURE::applyFrontSettings(sendData, sendDataSize, featureSettings))
        {
            _bus.FillBuffer(&data, sendData, T_COLOR_FEATURE::SettingsSize);
        }

        // apply primary color data
        //
        T_COLOR_OBJECT* pixel = pixels;
        const T_COLOR_OBJECT* pixelEnd = pixel + countPixels;
        uint16_t stripCount = _pixelCount;

        while (stripCount--)
        {
            typename T_COLOR_FEATURE::ColorObject color = shader.Apply(*pixel);
            T_COLOR_FEATURE::applyPixelColor(sendData, sendDataSize, color);

            _bus.FillBuffer(&data, sendData, T_COLOR_FEATURE::PixelSize);

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
            _bus.FillBuffer(&data, sendData, T_COLOR_FEATURE::SettingsSize);
        }

        _bus.EndUpdate(); // triggers actual write after all mux busses have updated
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    const uint8_t _pin;            // output pin number
    const uint16_t _pixelCount; // count of pixels in the strip

    T_BUS _bus;          // holds instance for mux bus support
};

typedef NeoEsp32I2sMuxBus<NeoEspI2sMonoBuffContext<NeoEspI2sMuxMap<uint16_t, NeoEspI2sMuxBusSize16Bit>>, NeoEsp32I2sBusZero> NeoEsp32I2s0Mux16Bus;

#endif

// NORMAL
//

// I2s0x16
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2812x, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Ws2812xMethod;
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedWs2805,  NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Ws2805Method;
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedSk6812,  NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Sk6812Method;
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1814,  NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted>    NeoEsp32I2s0X16Tm1814Method;
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1829,  NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted>    NeoEsp32I2s0X16Tm1829Method;
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedTm1914,  NeoEsp32I2s0Mux16Bus, NeoEsp32I2sInverted>    NeoEsp32I2s0X16Tm1914Method;
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed800Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16800KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeed400Kbps, NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16400KbpsMethod;
typedef NeoEsp32I2sXMethodBase<NeoEsp32I2sSpeedApa106,  NeoEsp32I2s0Mux16Bus, NeoEsp32I2sNotInverted> NeoEsp32I2s0X16Apa106Method;

typedef NeoEsp32I2s0X16Ws2805Method NeoEsp32I2s0X16Ws2814Method;
typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2813Method;
typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2812dMethod;
typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2811Method;
typedef NeoEsp32I2s0X16Ws2812xMethod NeoEsp32I2s0X16Ws2816Method;
typedef NeoEsp32I2s0X16800KbpsMethod NeoEsp32I2s0X16Ws2812Method;
typedef NeoEsp32I2s0X16Sk6812Method  NeoEsp32I2s0X16Lc8812Method;


#endif // defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
