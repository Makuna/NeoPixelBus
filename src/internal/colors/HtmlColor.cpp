/*-------------------------------------------------------------------------
This file contains the HtmlColor implementation

Written by Unai Uribarri

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

#include <Arduino.h>
#include "../NeoSettings.h"
#include "RgbColorBase.h"
#include "RgbColor.h"
#include "HtmlColor.h"

static inline char hexdigit(uint8_t v)
{
    return v + (v < 10 ? '0' : 'a' - 10);
}


/**
 * Generates a Html encoded hex color string (#aabbcc) with null termination.
 *
 * @param buf the buffer to write the string to
 * @param bufSize the maximum buffer size (8 recommended for # + 123456 + null terminator)
 * @return The amount of chars written to buf including the null terminator.
 */
size_t HtmlColor::ToNumericalString(char* buf, size_t bufSize) const
{
    if (bufSize > 0)
    {
        buf[0] = '#';

        uint32_t color = Color;
        size_t indexLast = min(7U, bufSize - 1);
        for (size_t indexDigit = 6U; indexDigit > 0; --indexDigit) // note pre-decrement
        {
            // note: the amount of digits may not match the buffer size and the most significant bits are on the left.
            // For 0xaabbcc and a bufSize of 4 we want "#aa\0", not "#cc\0".
            if (indexDigit <= indexLast)
            {
                buf[indexDigit] = hexdigit(color & 0x0000000f);
            }
            color >>= 4;
        }
        buf[indexLast] = '\0';

        return indexLast + 1;
    }
    return 0;
}
