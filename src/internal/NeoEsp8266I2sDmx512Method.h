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


class NeoEsp8266I2sDmx512SpeedBase
{
public:
    // 4.2 us bit send, 250Kbps
    static const uint32_t I2sClockDivisor = 21; // 0-63
    static const uint32_t I2sBaseClockDivisor = 32; // 0-63
    static const uint32_t ByteSendTimeUs = 47; // us it takes to send a single pixel element
    static const uint32_t MtbpUs = 100; // min 88
    // DMX requires the first slot to be zero
    static const size_t HeaderSize = 1;
};

class NeoEsp8266I2sDmx512Speed : public NeoEsp8266I2sDmx512SpeedBase
{
public:
    static const uint8_t MtbpLevel = 0x1; // high
    static const uint8_t StartBit = 0b00000000; 
    static const uint8_t StopBits = 0b00000011; 
    static const uint32_t BreakMab = 0x00000007; // Break + Mab

    static uint8_t Convert(uint8_t value)
    {
        // DMX requires LSB order
        return NeoUtil::Reverse8Bits( value );
    }
};

class NeoEsp8266I2sDmx512InvertedSpeed : public NeoEsp8266I2sDmx512SpeedBase
{
public:
    static const uint8_t MtbpLevel = 0x00; // low
    static const uint8_t StartBit = 0b00000001;
    static const uint8_t StopBits = 0b00000000;
    static const uint32_t BreakMab = 0xfffffff8; // Break + Mab

    static uint8_t Convert(uint8_t value)
    {
        // DMX requires LSB order
        return NeoUtil::Reverse8Bits( ~value );
    }
};


class NeoEsp8266I2sWs2821SpeedBase
{
public:
    // 1.4 us bit send, 750Kbps
    static const uint32_t I2sClockDivisor = 7; // 0-63
    static const uint32_t I2sBaseClockDivisor = 32; // 0-63
    static const uint32_t ByteSendTimeUs = 16; // us it takes to send a single pixel element
    static const uint32_t MtbpUs = 33; // min 88
    // DMX/WS2821 requires the first slot to be zero
    static const size_t HeaderSize = 1;
};

class NeoEsp8266I2sWs2821Speed : public NeoEsp8266I2sWs2821SpeedBase
{
public:
    static const uint8_t MtbpLevel = 0x1; // high
    static const uint8_t StartBit = 0b00000000;
    static const uint8_t StopBits = 0b00000011;
    static const uint32_t BreakMab = 0x00000007; // Break + Mab

    static uint8_t Convert(uint8_t value)
    {
        // DMX requires LSB order
        return NeoUtil::Reverse8Bits(value);
    }
};

class NeoEsp8266I2sWs2821InvertedSpeed : public NeoEsp8266I2sWs2821SpeedBase
{
public:
    static const uint8_t MtbpLevel = 0x00; // low
    static const uint8_t StartBit = 0b00000001;
    static const uint8_t StopBits = 0b00000000;
    static const uint32_t BreakMab = 0xfffffff8; // Break + Mab

    static uint8_t Convert(uint8_t value)
    {
        // DMX requires LSB order
        return NeoUtil::Reverse8Bits(~value);
    }
};

template<typename T_SPEED> class NeoEsp8266I2sDmx512MethodBase : NeoEsp8266I2sMethodCore
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp8266I2sDmx512MethodBase(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize + T_SPEED::HeaderSize)
    {
        size_t dmaPixelBits = I2sBitsPerPixelBytes * elementSize;
        size_t dmaSettingsBits = I2sBitsPerPixelBytes * (settingsSize + T_SPEED::HeaderSize);

        // bits + half rounding byte of bits / bits per byte
        size_t i2sBufferSize = (pixelCount * dmaPixelBits + dmaSettingsBits + 4) / 8;

        i2sBufferSize = i2sBufferSize + sizeof(T_SPEED::BreakMab);

        // size is rounded up to nearest I2sByteBoundarySize
        i2sBufferSize = NeoUtil::RoundUp(i2sBufferSize, I2sByteBoundarySize);

        // 4.2 us per bit
        size_t i2sZeroesBitsSize = (T_SPEED::MtbpUs) / 4;
        size_t i2sZeroesSize = NeoUtil::RoundUp(i2sZeroesBitsSize, 8) / 8;
        
        // protocol limits use of full block size to I2sByteBoundarySize
        size_t is2BufMaxBlockSize = (c_maxDmaBlockSize / I2sByteBoundarySize) * I2sByteBoundarySize;

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // first "slot" cleared due to protocol requiring it to be zero
        memset(_data, 0x00, 1);

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
        return _data + T_SPEED::HeaderSize;
    };

    size_t getDataSize() const
    {
        return _sizeData - T_SPEED::HeaderSize;
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    // given 11 sending bits per pixel byte, 
    static const uint16_t I2sBitsPerPixelBytes = 11;
    // i2s sends 4 byte elements, 
    static const uint16_t I2sByteBoundarySize = 4;

    const size_t  _sizeData;    // Size of '_data' buffer 
    uint8_t*  _data;        // Holds LED color values

    // encodes the data with start and stop bits
    // input buffer is bytes
    // output stream is uint31_t
    static void Encoder(const uint8_t* pSrc, const uint8_t* pSrcEnd, 
            uint32_t* pOutput, const uint32_t* pOutputEnd)
    {
        static const uint32_t Mtbp = 0xffffffff * T_SPEED::MtbpLevel;
        const uint8_t* pData = pSrc;

        int8_t outputBit = 32;
        uint32_t output = 0;

        // DATA stream, one start, two stop
        while (pData < pSrcEnd)
        {
            uint8_t data = T_SPEED::Convert( *(pData++) );

            if (outputBit > 10)
            {
                // simple
                outputBit -= 1;
                output |= T_SPEED::StartBit << outputBit;

                outputBit -= 8;
                output |= data << outputBit;

                outputBit -= 2;
                output |= T_SPEED::StopBits << outputBit;
            }
            else
            {
                // split across an output uint32_t
                // handle start bit
                if (outputBit < 1)
                {
                    *(pOutput++) = output;
                    output = 0;
                    outputBit += 32;
                }
                outputBit -= 1;
                output |= (T_SPEED::StartBit << outputBit);

                // handle data bits
                if (outputBit < 8)
                {
                    output |= data >> (8 - outputBit);

                    *(pOutput++) = output;
                    output = 0;
                    outputBit += 32;
                }
                outputBit -= 8;
                output |= data << outputBit;

                // handle stop bits
                if (outputBit < 2)
                {
                    output |= T_SPEED::StopBits >> (2 - outputBit);

                    *(pOutput++) = output;
                    output = 0;
                    outputBit += 32;
                }
                outputBit -= 2;
                output |= T_SPEED::StopBits << outputBit;
            }
        }
        if (outputBit > 0)
        {
            // padd last output uint32_t with Mtbp
            output |= Mtbp >> (32 - outputBit);
            *(pOutput++) = output;
        }
        // fill the rest of the output with Mtbp
        while (pOutput < pOutputEnd)
        {
            *(pOutput++) = Mtbp;
        }
    }


    void FillBuffers()
    {
        uint32_t* pDma32 = reinterpret_cast<uint32_t*>(_i2sBuffer);
        const uint32_t* pDma32End = reinterpret_cast<uint32_t*>(_i2sBuffer + _i2sBufferSize);

        // first put Break and MAB at front
        // 
        // BREAK 121.8us @ 4.2us per bit 
        // MAB 12.6us
        *(pDma32++) = T_SPEED::BreakMab;
        
        Encoder(_data, _data + _sizeData, pDma32, pDma32End);
    }

    uint32_t getPixelTime() const
    {
        return (T_SPEED::ByteSendTimeUs * this->_sizeData);
    };

};


// normal
typedef NeoEsp8266I2sDmx512MethodBase<NeoEsp8266I2sDmx512Speed> NeoEsp8266Dmx512Method;
typedef NeoEsp8266I2sDmx512MethodBase<NeoEsp8266I2sWs2821Speed> NeoEsp8266Ws2821Method;

// inverted
typedef NeoEsp8266I2sDmx512MethodBase<NeoEsp8266I2sDmx512InvertedSpeed> NeoEsp8266Dmx512InvertedMethod;
typedef NeoEsp8266I2sDmx512MethodBase<NeoEsp8266I2sWs2821InvertedSpeed> NeoEsp8266Ws2821InvertedMethod;

#endif
