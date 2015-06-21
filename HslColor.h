/*--------------------------------------------------------------------
NeoPixel is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

NeoPixel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
--------------------------------------------------------------------*/
#pragma once

#include <Arduino.h>
#define HSL_FLOAT

// ------------------------------------------------------------------------
// HslColor represents a color object that is represented by Hue, Saturation, Lightness
// component values.  It contains helpful color routines to manipulate the 
// color.
// ------------------------------------------------------------------------
struct HslColor
{
#ifdef HSL_FLOAT
    // ------------------------------------------------------------------------
    // Construct a HslColor using H, S, L values (0.0 - 1.0)
    // ------------------------------------------------------------------------
    HslColor(float h, float s, float l) :
        H(h), S(s), L(l)
    {
    };
#else
    // ------------------------------------------------------------------------
    // Construct a HslColor using H, S, L values (0 - 255)
    // generally, L should be limited to between (0-127)
    // ------------------------------------------------------------------------
    HslColor(uint8_t h, uint8_t s, uint8_t l) :
        H(h), S(s), L(l)
    {
    };
#endif

    // ------------------------------------------------------------------------
    // Construct a HslColor using RgbColor
    // ------------------------------------------------------------------------
    HslColor(RgbColor color);

    // ------------------------------------------------------------------------
    // Construct a HslColor that will have its values set in latter operations
    // CAUTION:  The H,S,L members are not initialized and may not be consistent
    // ------------------------------------------------------------------------
    HslColor()
    {
    };

    // ------------------------------------------------------------------------
    // LinearBlend between two colors by the amount defined by progress variable
    // left - the color to start the blend at
    // right - the color to end the blend at
    // progress - (0.0 - 1.0) value where 0.0 will return left and 1.0 will return right
    //     and a value between will blend the color weighted linearly between them
    // ------------------------------------------------------------------------
    static HslColor LinearBlend(HslColor left, HslColor right, float progress);

    // ------------------------------------------------------------------------
    // Hue, Saturation, Lightness color members 
    // ------------------------------------------------------------------------
#ifdef HSL_FLOAT
    float H;
    float S;
    float L;
#else
    uint8_t H;
    uint8_t S;
    uint8_t L;
#endif
};

