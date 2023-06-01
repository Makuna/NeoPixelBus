/*-------------------------------------------------------------------------
NeoByteElements provides feature base classes to describe color elements
for NeoPixelBus Color Feature template classes

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

class NeoByteElements
{
public:
    static uint8_t* getPixelAddress(uint8_t* pPixels, uint16_t indexPixel, size_t pixelSize)
    {
        return pPixels + indexPixel * pixelSize;
    }
    static const uint8_t* getPixelAddress(const uint8_t* pPixels, uint16_t indexPixel, size_t pixelSize)
    {
        return pPixels + indexPixel * pixelSize;
    }

    static void replicatePixel(uint8_t* pPixelDest, const uint8_t* pPixelSrc, uint16_t count, size_t pixelSize)
    {
        uint8_t* pEnd = pPixelDest + (count * pixelSize);

        while (pPixelDest < pEnd)
        {
            for (uint8_t iElement = 0; iElement < pixelSize; iElement++)
            {
                *pPixelDest++ = pPixelSrc[iElement];
            }
        }
    }

    static void movePixelsInc(uint8_t* pPixelDest, const uint8_t* pPixelSrc, uint16_t count, size_t pixelSize)
    {
        uint8_t* pEnd = pPixelDest + (count * pixelSize);

        while (pPixelDest < pEnd)
        {
            *pPixelDest++ = *pPixelSrc++;
        }
    }

    static void movePixelsInc_P(uint8_t* pPixelDest, PGM_VOID_P pPixelSrc, uint16_t count, size_t pixelSize)
    {
        uint8_t* pEnd = pPixelDest + (count * pixelSize);
        const uint8_t* pSrc = (const uint8_t*)pPixelSrc;
    
        while (pPixelDest < pEnd)
        {
            *pPixelDest++ = pgm_read_byte(pSrc++);
        }
    }

    static void movePixelsDec(uint8_t* pPixelDest, const uint8_t* pPixelSrc, uint16_t count, size_t pixelSize)
    {
        uint8_t* pDestBack = pPixelDest + (count * pixelSize);
        const uint8_t* pSrcBack = pPixelSrc + (count * pixelSize);

        while (pDestBack > pPixelDest)
        {
            *--pDestBack = *--pSrcBack;
        }
    }
};