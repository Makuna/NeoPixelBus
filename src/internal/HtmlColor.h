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

// Due to Arduino's lack of Project settings for symbols, library code (c,cpp)
// files can't react to defines in sketch, so this must be defined here
// #define USE_CSS3_COLORS 1

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
    // nameSize - the max size of name to check
    //
    // returns - zero if failed, or the number of chars parsed
    //
    // It will stop parsing name when a null terminator is reached, 
    // nameSize is reached, no match is found in the name/color pair table, or
    // a non-alphanumeric is read like seperators or whitespace.
    //
    // It accepts all standard HTML4 names and, if USE_CSS3_COLORS macro is
    // defined, the extended color names defined in CSS3 standard also.
    //
    // It also accepts 3 or 6 digit hexadecimal notation (#rgb or #rrggbb),
    // but it doesn't accept RGB, RGBA nor HSL values.
    //
    // See https://www.w3.org/TR/css3-color/#SRGB
    //
    // name must point to the first non-whitespace character to be parsed
    // parsing will stop at the first non-alpha numeric
    //
    // Name MUST NOT be a PROGMEM pointer
    // ------------------------------------------------------------------------
    size_t Parse(const char* name, size_t nameSize);

    size_t Parse(const char* name)
    { 
        return Parse(name, strlen(name) + 1); 
    }

    size_t Parse(String const &name)
    { 
        return Parse(name.c_str(), name.length() + 1); 
    }

    // ------------------------------------------------------------------------
    // Converts this color code to its HTML4/CSS3 name
    //
    // buf - buffer to write the string
    // bufSize - actual size of buf array
    //
    // It returns the number of chars required not including the NUL terminator.
    //
    // If there is not enough space in the buffer, it will write as many
    // characters as allowed and will always finish the buffer with a NUL char
    // ------------------------------------------------------------------------
    size_t ToString(char *buf, size_t bufSize) const;

    // ------------------------------------------------------------------------
    // Converts this color code to its HTML4/CSS3 numerical name
    //
    // buf - buffer to write the string
    // bufSize - actual size of buf array
    //
    // It returns the number of chars required not including the NUL terminator.
    //
    // If there is not enough space in the buffer, it will write as many
    // characters as allowed and will always finish the buffer with a NUL char
    // ------------------------------------------------------------------------
    size_t ToNumericalString(char* buf, size_t bufSize) const;

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

