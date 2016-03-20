/*-------------------------------------------------------------------------
HtmlColor provides a color object that can be directly consumed by NeoPixelBus

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

#include <Arduino.h>
#include "RgbColor.h"


// ------------------------------------------------------------------------
// HtmlColor represents a color object that is represented by a single uint32
// value.  It contains minimal routines and used primarily as a helper to
// initialize other color objects
// ------------------------------------------------------------------------
struct HtmlColor
{
    // ------------------------------------------------------------------------
    // Construct a HtmlColor using a single value (0-0xffffff)
    // This works well for hexidecimal color definitions
    // 0xff0000 = red, 0x00ff00 = green, and 0x0000ff = blue
    // ------------------------------------------------------------------------
    HtmlColor(uint32_t color) :
        Color(color)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a HtmlColor using RgbColor
    // ------------------------------------------------------------------------
    HtmlColor(const RgbColor& color)
    {
        Color = color.R << 16 | color.G << 8 | color.B;
    }

    // ------------------------------------------------------------------------
    // Construct a HtmlColor that will have its values set in latter operations
    // CAUTION:  The Color member is not initialized and may not be consistent
    // ------------------------------------------------------------------------
    HtmlColor()
    {
    };

    // ------------------------------------------------------------------------
    // Comparison operators
    // ------------------------------------------------------------------------
    bool operator==(const HtmlColor& other) const
    {
        return (Color == other.Color);
    };

    bool operator!=(const HtmlColor& other) const
    {
        return !(*this == other);
    };

    // ------------------------------------------------------------------------
    // BilinearBlend between four colors by the amount defined by 2d variable
    // c00 - upper left quadrant color
    // c01 - upper right quadrant color
    // c10 - lower left quadrant color
    // c11 - lower right quadrant color
    // x - unit value (0.0 - 1.0) that defines the blend progress in horizontal space
    // y - unit value (0.0 - 1.0) that defines the blend progress in vertical space
    // ------------------------------------------------------------------------
    static HtmlColor BilinearBlend(const HtmlColor& c00,
        const HtmlColor& c01,
        const HtmlColor& c10,
        const HtmlColor& c11,
        float x,
        float y)
    {
        return RgbColor::BilinearBlend(c00, c01, c10, c11, x, y);
    }

    // ------------------------------------------------------------------------
    // Color member (0-0xffffff) where 
    // 0xff0000 is red
    // 0x00ff00 is green
    // 0x0000ff is blue
    // ------------------------------------------------------------------------
    uint32_t Color;
};

