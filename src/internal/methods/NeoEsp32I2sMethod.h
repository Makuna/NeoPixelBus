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

#pragma once

// ESP32 C3 & S3 I2S is not supported yet due to significant changes to interface
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)


extern "C"
{
#include <rom/gpio.h>
#include "Esp32_i2s.h"
}

// --------------------------------------------------------
class NeoEsp32I2sBusZero
{
public:
    NeoEsp32I2sBusZero() {};

    const static uint8_t I2sBusNumber = 0;
};

class NeoEsp32I2sBusOne
{
public:
    NeoEsp32I2sBusOne() {};

    const static uint8_t I2sBusNumber = 1;
};

// dynamic channel support
class NeoEsp32I2sBusN
{
public:
    NeoEsp32I2sBusN(NeoBusChannel channel) :
        I2sBusNumber(static_cast<uint8_t>(channel))
    {
    }
    NeoEsp32I2sBusN() = delete; // no default constructor

    const uint8_t I2sBusNumber;
};

// --------------------------------------------------------


// 4 step cadence, so pulses are 1/4 and 3/4 of pulse width
//
class NeoEsp32I2sCadence4Step
{
public:
    const static size_t DmaBitsPerPixelBit = 4; // 4 step cadence, matches encoding

    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
    {
        const uint16_t bitpatterns[16] =
        {
            0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
            0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
            0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
            0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
        };

        uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
        const uint8_t* pEnd = data + sizeData;
        for (const uint8_t* pSrc = data; pSrc < pEnd; pSrc++)
        {
            *(pDma++) = bitpatterns[((*pSrc) >> 4) & 0x0f];
            *(pDma++) = bitpatterns[((*pSrc) & 0x0f)];
        }
    }
};

// fedc ba98 7654 3210
// 0000 0000 0000 0000
//                 111
// 3 step cadence, so pulses are 1/3 and 2/3 of pulse width
//
class NeoEsp32I2sCadence3Step
{
public:
    const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches encoding

    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
    {
        const uint16_t OneBit =  0b00000110;
        const uint16_t ZeroBit = 0b00000100;
        const uint8_t SrcBitMask = 0x80;
        const size_t BitsInSample = sizeof(uint16_t) * 8;

        uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
        uint16_t dmaValue = 0;
        uint8_t destBitsLeft = BitsInSample;

        const uint8_t* pSrc = data;
        const uint8_t* pEnd = pSrc + sizeData;

        while (pSrc < pEnd)
        {
            uint8_t value = *(pSrc++);

            for (uint8_t bitSrc = 0; bitSrc < 8; bitSrc++)
            {
                const uint16_t Bit = ((value & SrcBitMask) ? OneBit : ZeroBit);

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

// --------------------------------------------------------
template<typename T_SPEED, typename T_BUS, typename T_INVERT, typename T_CADENCE> class NeoEsp32I2sMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32I2sMethodBase(uint8_t pin, uint16_t pixelCount, size_t pixelSize, size_t settingsSize)  :
        _sizeData(pixelCount * pixelSize + settingsSize),
        _pin(pin)
    {
        construct(pixelCount, pixelSize, settingsSize);
    }

    NeoEsp32I2sMethodBase(uint8_t pin, uint16_t pixelCount, size_t pixelSize, size_t settingsSize, NeoBusChannel channel) :
        _sizeData(pixelCount * pixelSize + settingsSize),
        _pin(pin),
        _bus(channel)
    {
        construct(pixelCount, pixelSize, settingsSize);
    }

    ~NeoEsp32I2sMethodBase()
    {
        while (!IsReadyToUpdate())
        {
            yield();
        }

        i2sDeinit(_bus.I2sBusNumber);

        gpio_matrix_out(_pin, SIG_GPIO_OUT_IDX, false, false);
        pinMode(_pin, INPUT);

        free(_data);
        heap_caps_free(_i2sBuffer);
    }

    bool IsReadyToUpdate() const
    {
        return (i2sWriteDone(_bus.I2sBusNumber));
    }

    void Initialize()
    {
        size_t dmaBlockCount = (_i2sBufferSize + I2S_DMA_MAX_DATA_LEN - 1) / I2S_DMA_MAX_DATA_LEN;

        i2sInit(_bus.I2sBusNumber, 
            false,
            2, // bytes per sample 
            T_CADENCE::DmaBitsPerPixelBit,
            T_SPEED::BitSendTimeNs,
            I2S_CHAN_STEREO, 
            I2S_FIFO_16BIT_DUAL, 
            dmaBlockCount,
            _i2sBuffer,
            _i2sBufferSize);
        i2sSetPins(_bus.I2sBusNumber, _pin, -1, -1, T_INVERT::Inverted);
    }

    void Update(bool)
    {
        // wait for not actively sending data
        while (!IsReadyToUpdate())
        {
            yield();
        }

        T_CADENCE::EncodeIntoDma(_i2sBuffer, _data, _sizeData);

#if defined(NEO_DEBUG_DUMP_I2S_BUFFER)
        // dump the is2buffer
        uint8_t* pDma = _i2sBuffer;
        uint8_t* pEnd = pDma + _i2sBufferSize;
        size_t index = 0;

        Serial.println();
        Serial.println("NeoEspI2sMethod - i2sBufferDump: ");
        while (pDma < pEnd)
        {
            uint8_t value = *pDma;

            // a single bit pulse of data
            if ((index % 4) == 0)
            {
                Serial.println();
            }

            NeoUtil::PrintBin<uint8_t>(value);

            Serial.print(" ");
            pDma++;
            index++;

        }
        Serial.println();

#endif // NEO_DEBUG_DUMP_I2S_BUFFER

        i2sWrite(_bus.I2sBusNumber);
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
    const size_t  _sizeData;    // Size of '_data' buffer 
    const uint8_t _pin;            // output pin number
    const T_BUS _bus; // holds instance for multi bus support

    uint8_t*  _data;        // Holds LED color values

    size_t _i2sBufferSize; // total size of _i2sBuffer
    uint8_t* _i2sBuffer;  // holds the DMA buffer that is referenced by _i2sBufDesc

    void construct(uint16_t pixelCount, size_t pixelSize, size_t settingsSize) 
    {
        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()

        // must have a 4 byte aligned buffer for i2s
        // since the reset/silence at the end is used for looping
        // it also needs to 4 byte aligned
        size_t dmaSettingsSize = T_CADENCE::DmaBitsPerPixelBit * settingsSize;
        size_t dmaPixelSize = T_CADENCE::DmaBitsPerPixelBit * pixelSize;
        size_t resetSize = NeoUtil::RoundUp(T_CADENCE::DmaBitsPerPixelBit * T_SPEED::ResetTimeUs / T_SPEED::ByteSendTimeUs(T_SPEED::BitSendTimeNs), 4);

        _i2sBufferSize = NeoUtil::RoundUp(pixelCount * dmaPixelSize + dmaSettingsSize, 4) +
                resetSize;

        _i2sBuffer = static_cast<uint8_t*>(heap_caps_malloc(_i2sBufferSize, MALLOC_CAP_DMA));
        // no need to initialize all of it, but since it contains
        // "reset" bits that don't latter get overwritten we just clear it all
        memset(_i2sBuffer, 0x00, _i2sBufferSize);
    }


};

#if defined(NPB_CONF_4STEP_CADENCE)

typedef NeoEsp32I2sCadence4Step NeoEsp32I2sCadence;

#else

typedef NeoEsp32I2sCadence3Step NeoEsp32I2sCadence;

#endif

typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Ws2812xMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Ws2805Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedSk6812, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Sk6812Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1814, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Tm1814Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1829, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Tm1829Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1914, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Tm1914Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0800KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0400KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedApa106, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Apa106Method;
typedef NeoEsp32I2s0Ws2805Method NeoEsp32I2s0Ws2814Method;

typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Ws2812xInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0SWs2805InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedSk6812, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Sk6812InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1814, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Tm1814InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1914, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Tm1914InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1829, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Tm1829InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0800KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0400KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedApa106, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s0Apa106InvertedMethod;
typedef NeoEsp32I2s0SWs2805InvertedMethod NeoEsp32I2s0SWs2814InvertedMethod;

#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
// (SOC_I2S_NUM == 2)

typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Ws2812xMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Ws2805Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedSk6812, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Sk6812Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1814, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Tm1814Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1829, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Tm1829Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1914, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Tm1914Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1800KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1400KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedApa106, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Apa106Method;
typedef NeoEsp32I2s1Ws2805Method NeoEsp32I2s1Ws2814Method;

typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Ws2812xInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Ws2805InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedSk6812, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Sk6812InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1814, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Tm1814InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1829, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Tm1829InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1914, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Tm1914InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1800KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1400KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedApa106, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2s1Apa106InvertedMethod;
typedef NeoEsp32I2s1Ws2805InvertedMethod NeoEsp32I2s1Ws2814InvertedMethod;

typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sNWs2812xMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sNWs2805Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedSk6812, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sNSk6812Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1814, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sNTm1814Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1829, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sNTm1829Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1914, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sNTm1914Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sN800KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sN400KbpsMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedApa106, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sNApa106Method;
typedef NeoEsp32I2sNWs2805Method NeoEsp32I2sNWs2814Method;

typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sNWs2812xInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2805, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sNWs2805InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedSk6812, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sNSk6812InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1814, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sNTm1814InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1829, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sNTm1829InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedTm1914, NeoEsp32I2sBusN, NeoBitsNotInverted, NeoEsp32I2sCadence> NeoEsp32I2sNTm1914InvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed800Kbps, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sN800KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeed400Kbps, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sN400KbpsInvertedMethod;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedApa106, NeoEsp32I2sBusN, NeoBitsInverted, NeoEsp32I2sCadence> NeoEsp32I2sNApa106InvertedMethod;
typedef NeoEsp32I2sNWs2805InvertedMethod NeoEsp32I2sNWs2814InvertedMethod;

#endif

#if !defined(NEOPIXEL_ESP32_RMT_DEFAULT) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) 

// I2s Bus 1 method is the default method for Esp32
// Esp32 S2 & C3 & S3 will use RMT as the default always
typedef NeoEsp32I2s1Ws2812xMethod NeoWs2813Method;
typedef NeoEsp32I2s1Ws2812xMethod NeoWs2812xMethod;
typedef NeoEsp32I2s1800KbpsMethod NeoWs2812Method;
typedef NeoEsp32I2s1Ws2812xMethod NeoWs2811Method;
typedef NeoEsp32I2s1Ws2812xMethod NeoWs2816Method;
typedef NeoEsp32I2s1Ws2805Method NeoWs2805Method;
typedef NeoEsp32I2s1Ws2814Method NeoWs2814Method;
typedef NeoEsp32I2s1Sk6812Method NeoSk6812Method;
typedef NeoEsp32I2s1Tm1814Method NeoTm1814Method;
typedef NeoEsp32I2s1Tm1829Method NeoTm1829Method;
typedef NeoEsp32I2s1Tm1914Method NeoTm1914Method;
typedef NeoEsp32I2s1Sk6812Method NeoLc8812Method;
typedef NeoEsp32I2s1Apa106Method NeoApa106Method;

typedef NeoEsp32I2s1Ws2812xMethod Neo800KbpsMethod;
typedef NeoEsp32I2s1400KbpsMethod Neo400KbpsMethod;

typedef NeoEsp32I2s1Ws2812xInvertedMethod NeoWs2813InvertedMethod;
typedef NeoEsp32I2s1Ws2812xInvertedMethod NeoWs2812xInvertedMethod;
typedef NeoEsp32I2s1Ws2812xInvertedMethod NeoWs2811InvertedMethod;
typedef NeoEsp32I2s1Ws2812xInvertedMethod NeoWs2816InvertedMethod;
typedef NeoEsp32I2s1Ws2805InvertedMethod NeoWs2805InvertedMethod;
typedef NeoEsp32I2s1Ws2814InvertedMethod NeoWs2814InvertedMethod;
typedef NeoEsp32I2s1800KbpsInvertedMethod NeoWs2812InvertedMethod;
typedef NeoEsp32I2s1Sk6812InvertedMethod NeoSk6812InvertedMethod;
typedef NeoEsp32I2s1Tm1814InvertedMethod NeoTm1814InvertedMethod;
typedef NeoEsp32I2s1Tm1829InvertedMethod NeoTm1829InvertedMethod;
typedef NeoEsp32I2s1Tm1914InvertedMethod NeoTm1914InvertedMethod;
typedef NeoEsp32I2s1Sk6812InvertedMethod NeoLc8812InvertedMethod;
typedef NeoEsp32I2s1Apa106InvertedMethod NeoApa106InvertedMethod;

typedef NeoEsp32I2s1Ws2812xInvertedMethod Neo800KbpsInvertedMethod;
typedef NeoEsp32I2s1400KbpsInvertedMethod Neo400KbpsInvertedMethod;

#endif // !defined(NEOPIXEL_ESP32_RMT_DEFAULT) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)

#endif
