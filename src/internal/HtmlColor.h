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

#if defined(USE_CSS3_COLORS)
#define MAX_HTML_COLOR_NAME_LEN 21
#else
#define MAX_HTML_COLOR_NAME_LEN 8
#endif

// ------------------------------------------------------------------------
// HtmlColor represents an association between a name and a HTML color code
// ------------------------------------------------------------------------
struct HtmlColorName
{
    PGM_P Name;
    uint32_t Color;
};

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
    // Parse a HTML4/CSS3 color name
    //
    // name - the color name
    // namelen - length of the name string
    //
    // It accepts all standard HTML4 names and, if USE_CSS3_COLORS macro is
    // defined, the extended color names defined in CSS3 standard also.
    //
    // It also accepts 3 or 6 digit hexadecimal notation (#rgb or #rrggbb),
    // but it doesn't accept RGB, RGBA nor HSL values.
    //
    // See https://www.w3.org/TR/css3-color/#SRGB
    //
    // Name MUST be stripped of any leading or tailing whitespace.
    //
    // Name MUST NOT be a PROGMEM pointer
    // ------------------------------------------------------------------------
    bool Parse(const char* name, size_t namelen);
    bool Parse(const char* name) { return Parse(name, strlen(name)); }
    bool Parse(String const &name) { return Parse(name.c_str(), name.length()); }

    // ------------------------------------------------------------------------
    // Converts this color code to its HTML4/CSS3 name
    //
    // buf - array of at least MAX_HTML_COLOR_NAME_LEN chars to store the name
    // len - actual length of buf array
    //
    // It returns the space needed to write the color name not including the
    // final NUL char.
    //
    // If there is not enough space in the buffer, it will write as many
    // characters as allowed and will always finish the buffer with a NUL char
    // ------------------------------------------------------------------------
    size_t ToString(char *buf, size_t len) const;

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

private:
    // ------------------------------------------------------------------------
    // Array with all color names and its corresponding color codes
    // The array ends with a NULL color name.
    // ------------------------------------------------------------------------
    static const HtmlColorName ColorNames[] PROGMEM;
};

