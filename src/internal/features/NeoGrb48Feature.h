/*-------------------------------------------------------------------------
NeoGrb48Feature provides feature classes to describe color order and
color depth for NeoPixelBus template class

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

class NeoGrb48Feature : 
    public NeoWordElements<6, Rgb48Color>,
    public NeoElementsNoSettings
{
public:
    static void applyPixelColor(uint8_t* pPixels, uint16_t indexPixel, ColorObject color)
    {
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        // due to endianness the byte order must be copied to output
        *p++ = color.G >> 8;
        *p++ = color.G & 0xff;
        *p++ = color.R >> 8;
        *p++ = color.R & 0xff;
        *p++ = color.B >> 8;
        *p = color.B & 0xff;
    }

    static ColorObject retrievePixelColor(const uint8_t* pPixels, uint16_t indexPixel)
    {
        ColorObject color;
        const uint8_t* p = getPixelAddress(pPixels, indexPixel);

        // due to endianness the byte order must be copied to output
        color.G = (static_cast<uint16_t>(*p++) << 8);
        color.G |= *p++;
        color.R = (static_cast<uint16_t>(*p++) << 8);
        color.R |= *p++;
        color.B = (static_cast<uint16_t>(*p++) << 8);
        color.B |= *p;

        return color;
    }

    static ColorObject retrievePixelColor_P(PGM_VOID_P pPixels, uint16_t indexPixel)
    {
        ColorObject color;
        const uint16_t* p = reinterpret_cast<const uint16_t*>(getPixelAddress(reinterpret_cast<const uint8_t*>(pPixels), indexPixel));

        // PROGMEM unit of storage expected to be the same size as color element
        //    so no endianness issues to worry about
        color.G = pgm_read_word(p++);
        color.R = pgm_read_word(p++);
        color.B = pgm_read_word(p);

        return color;
    }
};
