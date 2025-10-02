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


// fedc ba98 7654 3210
// 0000 0000 0000 0000
//                 111
// 3 step cadence, so pulses are 1/3 and 2/3 of pulse width
//
class NeoEsp32Ucs7406Cadence3Step
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

        // First the header
        constexpr uint8_t header[15] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, //"work code"
            0x03, 0x07,//Not sure what this does. The 12 bit controller used 0x03, 0x0b. Found this combination to work with 8 bit colors using trial and error
            //   ^ there will be a gap here
            0x0f, 0x0f, 0x0f, 0x0f,//probably  max current per channel
            0x00, 0x00
        };
        const uint8_t* hSrc   = header;
        const uint8_t* hEnd   = header + sizeof(header);
        const uint8_t* space  = header + 8;
        while (hSrc < hEnd)
        {
            uint8_t value = *(hSrc++);

            for (uint8_t bitSrc = 0; bitSrc < 8; bitSrc++)
            {
                const uint16_t Bit = ((value & SrcBitMask) ? OneBit : ZeroBit);

                if (destBitsLeft > 3)
                {
                    destBitsLeft -= 3;
                    dmaValue |= Bit << destBitsLeft;
                }
                else if (destBitsLeft <= 3)
                {
                    uint8_t bitSplit = (3 - destBitsLeft);
                    dmaValue |= Bit >> bitSplit;

                    // next dma value, store and reset
                    *(pDma++) = dmaValue; 
                    dmaValue = 0;
                    
                    destBitsLeft = BitsInSample - bitSplit;
                    if (bitSplit)
                    {
                        dmaValue |= Bit << destBitsLeft;
                    }
                }
                
                // Next
                value <<= 1;
            }
            // Space/gap
            if(hSrc == space) {
                *pDma++ = 0x00;
                *pDma++ = 0x00;
            }
        }
        
        // Then send the data
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



#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)
// (SOC_I2S_NUM == 2)

typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32Ucs7406Cadence3Step> NeoEsp32I2s0Ucs7604Method;
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedWs2812x, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32Ucs7406Cadence3Step> NeoEsp32I2s1Ucs7604Method;


#endif

#if !defined(NEOPIXEL_ESP32_RMT_DEFAULT) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3) 

// I2s Bus 1 method is the default method for Esp32
// Esp32 S2 & C3 & S3 will use RMT as the default always
typedef NeoEsp32I2s1Ucs7604Method NeoUcs7604Method;

#endif // !defined(NEOPIXEL_ESP32_RMT_DEFAULT) && !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) && !defined(CONFIG_IDF_TARGET_ESP32S3)

#endif
