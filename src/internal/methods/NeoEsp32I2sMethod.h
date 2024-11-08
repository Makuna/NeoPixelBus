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
//----------------------------------------------------------

// --------------------------------------------------------

// 11 step cadences, specifically for DMX output
//

class NeoEsp32I2sCadence11Step11BitLookup  // this is the quickest, include start & stop bits in the lookup
                                           // and use a double lookup to reduce bit-shifting
{
//#define _USE32BIT_  // the 32_bit lookup table is faster since it omits the clearing of the lsWord
                      // but of course uses more memory on the stack					  
public: 
    const static size_t DmaBitsPerPixelByte = 11;
	
	static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
    {
		uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
		uint16_t dmaValue[2] = {0, 0};  // 16-bit dual buffer, used for reading the created word(s)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__   // See 11-Step DMX Encoding.PNG for schematic explanation
		const uint8_t msW = 0, lsW = 1;
#else
		const uint8_t lsW = 0, msW = 1;
#endif
		uint32_t* dmaV32 = reinterpret_cast<uint32_t*>(dmaValue);  // pointer for the 32-bit version
                    // the 32-bit version is used to shift the bits so the lsBits shift into the lsWord
					// and are not just discarded
		const uint8_t* pSrc = data;
		const uint8_t* pEnd = pSrc + sizeData;
		const uint8_t BitsInSample = 16; //sizeof(dmaValue) * 8;
		uint8_t destBitsLeft = BitsInSample;
		*(pDma++) = 0x0000; // start the break.
		*(pDma++) = 0x0000; // with the headebyte added it doesn't fit into 4 bytes anymore
		*(pDma++) = 0xf803; // 0b1111 1000 0000 0011
		*pDma = 0x0; // clear the first word   !!

#ifndef _USE32BIT_
		const uint16_t LookUpMsN[16] = {0x003, 0x023, 0x013, 0x033, 0x00b, 0x02b, 0x01b, 0x03b,
										0x007, 0x027, 0x017, 0x037, 0x00f, 0x02f, 0x01f, 0x03f
										};
		const uint16_t LookUpLsN[16] = {0x000, 0x200, 0x100, 0x300, 0x080, 0x280, 0x180, 0x380,
										0x040, 0x240, 0x140, 0x340, 0x0c0, 0x2c0, 0x1c0, 0x3c0
										};
#else
		const uint32_t LookUpMsN[16] = {0x0030000, 0x0230000, 0x0130000, 0x0330000, 0x00b0000, 0x02b0000, 0x01b0000, 0x03b0000,
										0x0070000, 0x0270000, 0x0170000, 0x0370000, 0x00f0000, 0x02f0000, 0x01f0000, 0x03f0000
										};
		const uint32_t LookUpLsN[16] = {0x0000000, 0x2000000, 0x1000000, 0x3000000, 0x0800000, 0x2800000, 0x1800000, 0x3800000,
										0x0400000, 0x2400000, 0x1400000, 0x3400000, 0x0c00000, 0x2c00000, 0x1c00000, 0x3c00000
										};
#endif
		
		while (pSrc < pEnd)
		{
#ifndef _USE32BIT_
			dmaValue[msW] = LookUpLsN[(*pSrc) & 0x0f] | LookUpMsN[(*pSrc) >> 4];  // lookup value into the msWord
			// works like an automatic 16-bit Shift														            
#else
			*dmaV32 = LookUpLsN[(*pSrc) & 0x0f] | LookUpMsN[(*pSrc) >> 4];  
		    // basically read it as a 32-bit (asignment clears lsWord automatically)
#endif
			pSrc++;
			if (destBitsLeft < 11) // Split the Bits
			{
				*dmaV32 = *dmaV32 >> (11 - destBitsLeft);  // shift it as a 32-bit, so the rightshifted bits end up in the lsWord
				*(pDma++) |= dmaValue[msW]; // the msWord  & and up the 16-bit Dma buffer pointer
				*pDma = dmaValue[lsW]; // lsWord aka whatever got shifted out of the variable
				// instead of shifting again to obtain these bits, they are available and ready for 'or'
#ifndef _USE32BIT_
				dmaValue[lsW] = 0;  // clear the lsWord after use, not needed when using 32-bit lookup
#endif
				destBitsLeft += 5;  // 11 bits subtracted but rolled over backwards = 5 bits added.
			}
			else 
			{
				*dmaV32 = *dmaV32 << (destBitsLeft - 11); // shift to be the next bit
				*pDma |= dmaValue[msW]; // 'or' the most significant Word
				destBitsLeft -= 11;   // substract the 11 bits added
			}
		}
		if (destBitsLeft) 
		{
			*pDma |= 0xffff >> (16 - destBitsLeft);
		}		
	}
};



class NeoEsp32I2sCadence11Step8BitLookup
{
public: 
    const static size_t DmaBitsPerPixelByte = 11;
	
	static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
    {
		const uint8_t* pSrc = data;
		const uint8_t* pEnd = pSrc + sizeData;
		uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
		const uint8_t Reverse8BitsLookup[16] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};
              // significantly quicker than calling a separate function, at least in my speed test.

		int8_t outputBit = 16;
		uint16_t output = 0;

		*(pDma++) = 0x0000; // start the break.
		*(pDma++) = 0x0000; // with the headebyte added it doesn't fit into 4 bytes anymore
		*(pDma++) = 0xf803; // 0b1111 1000 0000 0011
		while (pSrc < pEnd)
		{
			uint8_t invertedByte = (Reverse8BitsLookup[*pSrc & 0xf] << 4) | Reverse8BitsLookup[*pSrc >> 4];
			pSrc++;
			if (outputBit > 10)
			{
				outputBit -= 1;
				output |= 0 << outputBit;
				outputBit -= 8;
				output |= invertedByte << outputBit;
				outputBit -= 2;
				output |= 3 << outputBit;
			}
			else
			{
			// split across an output uint16_t
			// handle start bit
				if (outputBit < 1)
				{
					*(pDma++) = output;
					output = 0;
					outputBit += 16;
				}
				outputBit -= 1;
				output |= 0 << outputBit;
				// handle data bits
				if (outputBit < 8)
				{
					output |= invertedByte >> (8 - outputBit);
					*(pDma++) = output;
					output = 0;
					outputBit += 16;
				}
				outputBit -= 8;
				output |= invertedByte << outputBit;
				// handle stop bits
				if (outputBit < 2)
				{
					output |= 3 >> (2 - outputBit);
					*(pDma++) = output;
					output = 0;
					outputBit += 16;
				}
				outputBit -= 2;
				output |= 3 << outputBit;
			}
		}
		if (outputBit > 0)
		{
			// padd last output uint16_t with Mtbp
			output |= 0xffff >> (16 - outputBit);
			*(pDma++) = output;
		}		
	}	
};
	

class NeoEsp32I2sCadence11StepNoLookup
{
public: 
    const static size_t DmaBitsPerPixelByte = 11;
	
	static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
    {

		uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
        uint16_t dmaValue = 0;
        const uint8_t* pSrc = data;
        const uint8_t* pEnd = pSrc + sizeData;
		const size_t BitsInSample = sizeof(dmaValue) * 8;
		
		uint8_t destBitsLeft = BitsInSample;
		

	    *(pDma++) = 0x0000; // start the break.
		*(pDma++) = 0x0000; // with the headerbyte added it doesn't fit into 4 bytes anymore
		*(pDma++) = 0xf803; // 0b1111 1000 0000 0011
						

        while (pSrc < pEnd)
        {
			uint8_t source = *(pSrc++); 
			if (!destBitsLeft)
			{
				destBitsLeft = BitsInSample;
				*(pDma++) = dmaValue;
				//dmaValue = 0;  // not needed and a waste of time.
			}
			dmaValue = dmaValue << 1;  // start Bit
			destBitsLeft--;
						
			for (uint8_t i = 0; i < 8; i++) // data bits
			{
				if (!destBitsLeft)
				{
					destBitsLeft = BitsInSample;
					*(pDma++) = dmaValue;
					//dmaValue = 0;  // not needed all bits will have been shifted out
				}
				dmaValue = dmaValue << 1;
				dmaValue |= (source & 1);
				source = source >> 1;
				destBitsLeft--;
			}
			
			for (uint8_t i = 0; i < 2; i++) // stop bits
			{
				if (!destBitsLeft)
				{
					destBitsLeft = BitsInSample;
					*(pDma++) = dmaValue;
					//dmaValue = 0;					
				}
				dmaValue = dmaValue << 1;
				dmaValue |= 1;
				destBitsLeft--;
			}			
		}
        if (destBitsLeft) {
			dmaValue = dmaValue << destBitsLeft;  // shift the significant bits fully left
			dmaValue |= (0xffff >> (BitsInSample - destBitsLeft)); // fill it up with HIGh bits
			*pDma++ = dmaValue;  // and store the remaining bits
		}
	}
};
	

// 4 step cadence, so pulses are 1/4 and 3/4 of pulse width
//
class NeoEsp32I2sCadence4Step
{
public:
    //const static size_t DmaBitsPerPixelBit = 4; // 4 step cadence, matches encoding
    const static size_t DmaBitsPerPixelByte = 32; // 4 step cadence, matches encoding   

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
            *(pDma++) = bitpatterns[((*pSrc) >> 4) /*& 0x0f*/];  // the mask is obsolete, an 8-bit shifted 4
			                                                     // bits doesn't have any other bits left.
            *(pDma++) = bitpatterns[((*pSrc) & 0x0f)];
        }
    }
};

class NeoEsp32I2sCadence3Stepfast
{
public:
    //const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches encoding
	const static size_t DmaBitsPerPixelByte = 24; // 3 step cadence, matches encoding
	
    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
    {		
        const uint32_t bitpatternsLow[16] =
        {
            0x000924, 0x000926, 0x000934, 0x000936, 0x0009A4, 0x0009A6, 0x0009B4, 0x0009B6, 
			0x000D24, 0x000D26, 0x000D34, 0x000D36, 0x000DA4, 0x000DA6, 0x000DB4, 0x000DB6

        };
		const uint32_t bitpatternsHigh[16] =
        {
            0x924000, 0x926000, 0x934000, 0x936000, 0x9A4000, 0x9A6000, 0x9B4000, 0x9B6000, 
			0xD24000, 0xD26000, 0xD34000, 0xD36000, 0xDA4000, 0xDA6000, 0xDB4000, 0xDB6000

        };
		uint32_t output[2];  // 2x 24-bit bitPattern in consequetive location
		uint8_t * output8 = reinterpret_cast<uint8_t *>(output);

        uint8_t* pDma = dmaBuffer;
        const uint8_t* pEnd = data + sizeData - 1;  // Encode 2 bytes at a time, make sure they are there
        const uint8_t* pSrc = data; 
		while (pSrc < pEnd)  
        {
            output[0] = bitpatternsHigh[((*pSrc) >> 4)] | bitpatternsLow[((*pSrc) & 0x0f)];
			pSrc++;
			output[1] = bitpatternsHigh[((*pSrc) >> 4)] | bitpatternsLow[((*pSrc) & 0x0f)];
			pSrc++;   // note: the mask for the bitpatternsHigh index should be obsolete
			          // To get the 2x 3-byte values in the right order copy them Byte by Byte
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__   // see Endian Dependant Byte Order.PNG for schematic explanation
			memcpy(pDma++, output8 + 1 , 1);
			memcpy(pDma++, output8 + 2 , 1);
			memcpy(pDma++, output8 + 3 , 1);
			memcpy(pDma++, output8 + 5 , 1);
			memcpy(pDma++, output8 + 6 , 1);
			memcpy(pDma++, output8 + 7 , 1);
#else					  
			memcpy(pDma++, output8 + 1 , 1);  
			memcpy(pDma++, output8 + 2 , 1);  
			memcpy(pDma++, output8 + 6 , 1);  
			memcpy(pDma++, output8 + 0 , 1);  
			memcpy(pDma++, output8 + 4 , 1);  
			memcpy(pDma++, output8 + 5 , 1); 
#endif
        }
		if (pSrc == pEnd)  // the last pixelbuffer byte if it exists
		{
			output[0] = bitpatternsHigh[((*pSrc) >> 4) & 0x0f] | bitpatternsLow[((*pSrc) & 0x0f)];
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			memcpy(pDma++, output8 + 1 , 1);
			memcpy(pDma++, output8 + 2 , 1);
			memcpy(pDma++, output8 + 3 , 1);
#else
			memcpy(pDma++, output8 + 1 , 1); 
			memcpy(pDma++, output8 + 2 , 1); 
			pDma++;
			memcpy(pDma++, output8 + 0 , 1); 
#endif			
		}		
    }
};

/*              Not used, kept for reference, slower than the previous method
class NeoEsp32I2sCadence3Stepfast2
{
public:
    //const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches encoding
	const static size_t DmaBitsPerPixelByte = 24; // 3 step cadence, matches encoding
	static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
	{
		const uint16_t bitpatterns[6][16] = {
		{
			0x9240, 0x9260, 0x9340, 0x9360, 0x9A40, 0x9A60, 0x9B40, 0x9B60,
			0xD240, 0xD260, 0xD340, 0xD360, 0xDA40, 0xDA60, 0xDB40, 0xDB60
		},
		{
			0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009,
			0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D
		},
		{
			0x0092, 0x0092, 0x0093, 0x0093, 0x009A, 0x009A, 0x009B, 0x009B,
			0x00D2, 0x00D2, 0x00D3, 0x00D3, 0x00DA, 0x00DA, 0x00DB, 0x00DB
		},
		{
			0x2400, 0x2600, 0x3400, 0x3600, 0xA400, 0xA600, 0xB400, 0xB600,
			0x2400, 0x2600, 0x3400, 0x3600, 0xA400, 0xA600, 0xB400, 0xB600
		},
		{
			0x4000, 0x6000, 0x4000, 0x6000, 0x4000, 0x6000, 0x4000, 0x6000,
			0x4000, 0x6000, 0x4000, 0x6000, 0x4000, 0x6000, 0x4000, 0x6000
		},
		{
			0x0924, 0x0926, 0x0934, 0x0936, 0x09A4, 0x09A6, 0x09B4, 0x09B6,
			0x0D24, 0x0D26, 0x0D34, 0x0D36, 0x0DA4, 0x0DA6, 0x0DB4, 0x0DB6
		}};


		uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
		const uint8_t* pEnd = data + sizeData - 1;  // Encode 2 bytes at a time, make sure they are there
		const uint8_t* pSrc = data;

		while (pSrc < pEnd)
		{
			uint8_t msNibble = ((*pSrc) >> 4) & 0x0f, lsNibble = (*pSrc) & 0x0f;
			*(pDma++) = bitpatterns[0][msNibble] | bitpatterns[1][lsNibble];
			pSrc++;
			msNibble = ((*pSrc) >> 4) & 0x0f;
			*(pDma++) = bitpatterns[2][msNibble] | bitpatterns[3][lsNibble];
			lsNibble = (*pSrc) & 0x0f;
			*(pDma++) = bitpatterns[4][msNibble] | bitpatterns[5][lsNibble];
			pSrc++;
		}
		if (pSrc == pEnd)  // the last pixelbuffer byte if it exists
		{
			uint8_t msNibble = ((*pSrc) >> 4) & 0x0f, lsNibble = (*pSrc) & 0x0f;
			*(pDma++) = bitpatterns[0][msNibble] | bitpatterns[1][lsNibble];
			*(pDma++) = bitpatterns[3][lsNibble];
		}
	}
};
*/


// fedc ba98 7654 3210
// 0000 0000 0000 0000
//                 111
// 3 step cadence, so pulses are 1/3 and 2/3 of pulse width
//
class NeoEsp32I2sCadence3Step
{
public:
    //const static size_t DmaBitsPerPixelBit = 3; // 3 step cadence, matches encoding
	const static size_t DmaBitsPerPixelByte = 24; // 3 step cadence, matches encoding

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
            //T_CADENCE::DmaBitsPerPixelBit,
			T_CADENCE::DmaBitsPerPixelByte / 8,   //   divide by 8 == DmaBitsPerPixelBit 
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
        /*size_t dmaSettingsSize = T_CADENCE::DmaBitsPerPixelBit * settingsSize;
        size_t dmaPixelSize = T_CADENCE::DmaBitsPerPixelBit * pixelSize;
        size_t resetSize = NeoUtil::RoundUp(T_CADENCE::DmaBitsPerPixelBit * T_SPEED::ResetTimeUs / (T_SPEED::BitSendTimeNs * 8 / 1000), 4);*/
		size_t dmaSettingsSize = T_CADENCE::DmaBitsPerPixelByte * settingsSize / 8;  // for these 2 calculations
        size_t dmaPixelSize = T_CADENCE::DmaBitsPerPixelByte * pixelSize / 8;  // the division is done after the multiplication
        size_t resetSize = NeoUtil::RoundUp((T_CADENCE::DmaBitsPerPixelByte / 8) * T_SPEED::ResetTimeUs / (T_SPEED::BitSendTimeNs * 8 / 1000), 4);
		// The version below was for the 5500Ns BitSendTimeNs by doing the division by 8 here, as with the call to i2sInit()
		// the division is done in integer and 11/ 8 = 1
		//size_t resetSize = NeoUtil::RoundUp(T_CADENCE::DmaBitsPerPixelByte * T_SPEED::ResetTimeUs / (T_SPEED::BitSendTimeNs * 8 / 1000) / 8, 4);

        _i2sBufferSize = NeoUtil::RoundUp(pixelCount * dmaPixelSize + dmaSettingsSize, 4) +
                resetSize;

        _i2sBuffer = static_cast<uint8_t*>(heap_caps_malloc(_i2sBufferSize, MALLOC_CAP_DMA));
        // no need to initialize all of it, but since it contains
        // "reset" bits that don't later get overwritten we just clear it all

		memset(_i2sBuffer, 0x00, _i2sBufferSize);
		
    }
};

//typedef NeoEsp32I2sCadence11StepNoLookup NeoEsp32I2sCadence11Step;

//typedef NeoEsp32I2sCadence11Step8BitLookup NeoEsp32I2sCadence11Step;

typedef NeoEsp32I2sCadence11Step11BitLookup NeoEsp32I2sCadence11Step;

typedef NeoEsp32I2sMethodBase<NeoBitsSpeedDmx512, NeoEsp32I2sBusZero, NeoBitsNotInverted, NeoEsp32I2sCadence11Step> NeoEsp32I2s0Dmx512Method; 
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedDmx512, NeoEsp32I2sBusZero, NeoBitsInverted, NeoEsp32I2sCadence11Step> NeoEsp32I2s0Dmx512InvertedMethod; 
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedDmx512, NeoEsp32I2sBusOne, NeoBitsNotInverted, NeoEsp32I2sCadence11Step> NeoEsp32I2s1Dmx512Method; 
typedef NeoEsp32I2sMethodBase<NeoBitsSpeedDmx512, NeoEsp32I2sBusOne, NeoBitsInverted, NeoEsp32I2sCadence11Step> NeoEsp32I2s1Dmx512InvertedMethod; 



#if defined(NPB_CONF_4STEP_CADENCE)

typedef NeoEsp32I2sCadence4Step NeoEsp32I2sCadence;

#else
	
//typedef NeoEsp32I2sCadence3Step NeoEsp32I2sCadence;

typedef NeoEsp32I2sCadence3Stepfast NeoEsp32I2sCadence;

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
