/*-------------------------------------------------------------------------
DotStarX4WordFeature provides feature base class to describe color order for
  3 color but 4 byte features when used with DotStars

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

template <uint8_t V_IC_1, uint8_t V_IC_2, uint8_t V_IC_3>
class DotStarX4WordFeature :
    public NeoElementsBase<8, Rgb48Color>
{
public:
    static void applyPixelColor(uint8_t* pixel, uint16_t pixelSize, ColorObject color)
    {
        if (PixelSize <= pixelSize)
        {
            uint8_t* p = pixel;

            *p++ = 0xff; // upper bit is always 1 and three 5 bit brightness at max
            *p++ = 0xff; // {1}{5}{5}{5}
            // due to endianness the byte order must be copied to output
            *p++ = color[V_IC_1] >> 8;
            *p++ = color[V_IC_1] & 0xff;
            *p++ = color[V_IC_2] >> 8;
            *p++ = color[V_IC_2] & 0xff;
            *p++ = color[V_IC_3] >> 8;
            *p = color[V_IC_3] & 0xff;
        }
    }
};
