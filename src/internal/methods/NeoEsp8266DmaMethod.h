/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp8266.


Written by Michael C. Miller.
Thanks to g3gg0.de for porting the initial DMA support which lead to this.
Thanks to github/cnlohr for the original work on DMA support, which opend
all our minds to a better way (located at https://github.com/cnlohr/esp8266ws2812i2s).

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

#pragma once

#ifdef ARDUINO_ARCH_ESP8266
#include "NeoEsp8266I2sMethodCore.h"

class NeoEsp8266DmaNormalPattern
{
public:
    static const uint8_t IdleLevel = 0;
    static const uint16_t OneBit3Step = 0b00000110;
    static const uint16_t ZeroBit3Step = 0b00000100;

    static uint16_t Convert4Step(uint8_t value)
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

class NeoEsp8266DmaInvertedPattern
{
public:
    static const uint8_t IdleLevel = 1;
    static const uint16_t OneBit3Step =  0b00000001;
    static const uint16_t ZeroBit3Step = 0b00000011;

    static uint16_t Convert4Step(uint8_t value)
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

template<typename T_PATTERN> class NeoEsp8266Dma3StepEncode : public T_PATTERN
{
public:
    const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches encoding

    static size_t SpacingPixelSize(size_t sizePixel)
    {
        return sizePixel;
    }

    static void FillBuffers(uint8_t* i2sBuffer,
        const uint8_t* data,
        size_t sizeData,
        [[maybe_unused]] size_t sizePixel)
    {
        const uint8_t SrcBitMask = 0x80;
        const size_t BitsInSample = sizeof(uint32_t) * 8;

        uint32_t* pDma = reinterpret_cast<uint32_t*>(i2sBuffer);
        uint32_t dmaValue = 0;
        uint8_t destBitsLeft = BitsInSample;

        const uint8_t* pSrc = data;
        const uint8_t* pEnd = pSrc + sizeData;

        while (pSrc < pEnd)
        {
            uint8_t value = *(pSrc++);

            for (uint8_t bitSrc = 0; bitSrc < 8; bitSrc++)
            {
                const uint16_t Bit = ((value & SrcBitMask) ? T_PATTERN::OneBit3Step : T_PATTERN::ZeroBit3Step);

                if (destBitsLeft > 3)
                {
                    destBitsLeft -= 3;
                    dmaValue |= Bit << destBitsLeft;

#if defined(NEO_DEBUG_DUMP_I2S_BUFFER)
                    NeoUtil::PrintBin<uint32_t>(dmaValue);
                    Serial.print(" < ");
                    Serial.println(destBitsLeft);
#endif
                }
                else if (destBitsLeft <= 3)
                {
                    uint8_t bitSplit = (3 - destBitsLeft);
                    dmaValue |= Bit >> bitSplit;

#if defined(NEO_DEBUG_DUMP_I2S_BUFFER)
                    NeoUtil::PrintBin<uint32_t>(dmaValue);
                    Serial.print(" > ");
                    Serial.println(bitSplit);
#endif
                    // next dma value, store and reset
                    *(pDma++) = dmaValue;
                    dmaValue = 0;

                    destBitsLeft = BitsInSample - bitSplit;
                    if (bitSplit)
                    {
                        dmaValue |= Bit << destBitsLeft;
                    }

#if defined(NEO_DEBUG_DUMP_I2S_BUFFER)
                    NeoUtil::PrintBin<uint32_t>(dmaValue);
                    Serial.print(" v ");
                    Serial.println(bitSplit);
#endif
                }

                // Next
                value <<= 1;
            }
        }
        // store the remaining bits
        *pDma++ = dmaValue;
    }
};

template<typename T_PATTERN> class NeoEsp8266Dma4StepEncode : public T_PATTERN
{
public:
    const static size_t DmaBitsPerPixelBit = 4; // 4 step cadence, matches encoding

    static size_t SpacingPixelSize(size_t sizePixel)
    {
        return sizePixel;
    }

    static void FillBuffers(uint8_t* i2sBuffer,
        const uint8_t* data,
        size_t sizeData,
        [[maybe_unused]] size_t sizePixel)
    {
        uint16_t* pDma = (uint16_t*)i2sBuffer;
        const uint8_t* pEnd = data + sizeData;
        for (const uint8_t* pData = data; pData < pEnd; pData++)
        {
            *(pDma++) = T_PATTERN::Convert4Step(((*pData) & 0x0f));
            *(pDma++) = T_PATTERN::Convert4Step(((*pData) >> 4) & 0x0f);
        }
    }
};

template<typename T_ENCODER, typename T_SPEED> class NeoEsp8266DmaMethodBase : NeoEsp8266I2sMethodCore
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp8266DmaMethodBase(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizePixel(elementSize),
        _sizeData(pixelCount * elementSize + settingsSize)
    {
        size_t dmaPixelSize = T_ENCODER::DmaBitsPerPixelBit * T_ENCODER::SpacingPixelSize(_sizePixel);
        size_t dmaSettingsSize = T_ENCODER::DmaBitsPerPixelBit * settingsSize;

        size_t i2sBufferSize = pixelCount * dmaPixelSize + dmaSettingsSize;
        // size is rounded up to nearest c_I2sByteBoundarySize
        i2sBufferSize = NeoUtil::RoundUp(i2sBufferSize, c_I2sByteBoundarySize);

        // calculate a buffer size that takes reset amount of time
        size_t i2sResetSize = T_SPEED::ResetTimeUs * T_ENCODER::DmaBitsPerPixelBit / T_SPEED::ByteSendTimeUs(T_SPEED::BitSendTimeNs);
        // size is rounded up to nearest c_I2sByteBoundarySize
        i2sResetSize = NeoUtil::RoundUp(i2sResetSize, c_I2sByteBoundarySize);
        size_t is2BufMaxBlockSize = (c_maxDmaBlockSize / dmaPixelSize) * dmaPixelSize;

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()

        AllocateI2s(i2sBufferSize, i2sResetSize, is2BufMaxBlockSize, T_ENCODER::IdleLevel);
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
        uint8_t i2sBaseClockDivisor; 
        uint8_t i2sClockDivisor;

        FindBestClockDivisors(&i2sClockDivisor, 
            &i2sBaseClockDivisor, 
            T_SPEED::BitSendTimeNs,
            T_ENCODER::DmaBitsPerPixelBit);

        InitializeI2s(i2sClockDivisor, i2sBaseClockDivisor);
    }

    void IRAM_ATTR Update(bool)
    {
        // wait for not actively sending data
        while (!IsReadyToUpdate())
        {
            yield();
        }
        T_ENCODER::FillBuffers(_i2sBuffer, _data, _sizeData, _sizePixel);
        
        WriteI2s();
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called only if changes to buffer
        return false;
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
    const size_t _sizePixel; // size of a pixel in _data
    const size_t  _sizeData;    // Size of '_data' buffer 
    uint8_t*  _data;        // Holds LED color values

    uint32_t getPixelTime() const
    {
        return (T_SPEED::ByteSendTimeUs(T_SPEED::BitSendTimeNs) * GetSendSize() / T_ENCODER::DmaBitsPerPixelBit);
    };

};

#if defined(NPB_CONF_4STEP_CADENCE)

template <typename T_PATTERN>
using NeoEsp8266I2sCadence = NeoEsp8266Dma4StepEncode<T_PATTERN>;

#else

template <typename T_PATTERN>
using NeoEsp8266I2sCadence = NeoEsp8266Dma3StepEncode<T_PATTERN>;

#endif

// normal
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaNormalPattern>,NeoBitsSpeedWs2812x> NeoEsp8266DmaWs2812xMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaNormalPattern>,NeoBitsSpeedWs2805> NeoEsp8266DmaWs2805Method;
typedef NeoEsp8266DmaWs2805Method NeoEsp8266DmaWs2814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaNormalPattern>,NeoBitsSpeedSk6812> NeoEsp8266DmaSk6812Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaInvertedPattern>,NeoBitsSpeedTm1814> NeoEsp8266DmaTm1814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaInvertedPattern>,NeoBitsSpeedTm1829> NeoEsp8266DmaTm1829Method;
typedef NeoEsp8266DmaTm1814Method NeoEsp8266DmaTm1914Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaNormalPattern>,NeoBitsSpeed800Kbps> NeoEsp8266Dma800KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaNormalPattern>,NeoBitsSpeed400Kbps> NeoEsp8266Dma400KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaNormalPattern>,NeoBitsSpeedApa106> NeoEsp8266DmaApa106Method;


// inverted
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaInvertedPattern>, NeoBitsSpeedWs2812x> NeoEsp8266DmaInvertedWs2812xMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaInvertedPattern>, NeoBitsSpeedWs2805> NeoEsp8266DmaInvertedWs2805Method;
typedef NeoEsp8266DmaInvertedWs2805Method NeoEsp8266DmaInvertedWs2814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaInvertedPattern>, NeoBitsSpeedSk6812> NeoEsp8266DmaInvertedSk6812Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaNormalPattern>, NeoBitsSpeedTm1814> NeoEsp8266DmaInvertedTm1814Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaNormalPattern>, NeoBitsSpeedTm1829> NeoEsp8266DmaInvertedTm1829Method;
typedef NeoEsp8266DmaInvertedTm1814Method NeoEsp8266DmaInvertedTm1914Method;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaInvertedPattern>, NeoBitsSpeed800Kbps> NeoEsp8266DmaInverted800KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaInvertedPattern>, NeoBitsSpeed400Kbps> NeoEsp8266DmaInverted400KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266I2sCadence<NeoEsp8266DmaInvertedPattern>, NeoBitsSpeedApa106> NeoEsp8266DmaInvertedApa106Method;


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

typedef NeoEsp8266DmaInvertedWs2812xMethod Neo800KbpsInvertedMethod;
typedef NeoEsp8266DmaInverted400KbpsMethod Neo400KbpsInvertedMethod;
#endif
