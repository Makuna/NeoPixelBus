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
#include "../NeoUtil.h"
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
 * @param bufSize the maximum buffer size (must be at least 8 characters)
 * @return The amount of chars written to buf including the null terminator.
 */
size_t HtmlColor::ToNumericalString(char* buf, size_t bufSize) const
{
    if (bufSize >= 8)
    {
        buf[0] = '#';

        uint32_t color = Color;
        for (size_t indexDigit = 6; indexDigit > 0; --indexDigit) // note pre-decrement
        {
            buf[indexDigit] = hexdigit(color & 0x0000000f);
            color >>= 4;
        }
        buf[7] = '\0';

        return 8;
    }
    return 0;
}
