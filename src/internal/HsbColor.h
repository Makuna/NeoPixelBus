
/*-------------------------------------------------------------------------
HsbColor provides a color object that can be directly consumed by NeoPixelBus

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

// ------------------------------------------------------------------------
// HsbColor represents a color object that is represented by Hue, Saturation, Brightness
// component values.  It contains helpful color routines to manipulate the 
// color.
// ------------------------------------------------------------------------
struct HsbColor
{
    // ------------------------------------------------------------------------
    // Construct a HsbColor using H, S, B values (0.0 - 1.0)
    // ------------------------------------------------------------------------
    HsbColor(float h, float s, float b) :
        H(h), S(s), B(b)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a HsbColor using RgbColor
    // ------------------------------------------------------------------------
    HsbColor(const RgbColor& color);

    // ------------------------------------------------------------------------
    // Construct a HsbColor that will have its values set in latter operations
    // CAUTION:  The H,S,B members are not initialized and may not be consistent
    // ------------------------------------------------------------------------
    HsbColor()
    {
    };

    // ------------------------------------------------------------------------
    // LinearBlend between two colors by the amount defined by progress variable
    // left - the color to start the blend at
    // right - the color to end the blend at
    // progress - (0.0 - 1.0) value where 0.0 will return left and 1.0 will return right
    //     and a value between will blend the color weighted linearly between them
    // ------------------------------------------------------------------------
    template <typename T_NEOHUEBLEND> static HsbColor LinearBlend(const HsbColor& left,
        const HsbColor& right,
        float progress)
    {
        return HsbColor(T_NEOHUEBLEND::HueBlend(left.H, right.H, progress),
            left.S + ((right.S - left.S) * progress),
            left.B + ((right.B - left.B) * progress));
    }

    // ------------------------------------------------------------------------
    // Hue, Saturation, Brightness color members 
    // ------------------------------------------------------------------------

    float H;
    float S;
    float B;
};

