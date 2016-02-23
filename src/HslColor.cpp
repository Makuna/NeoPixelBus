/*-------------------------------------------------------------------------
HslColor provides a color object that can be directly consumed by NeoPixelBus

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

#include "RgbColor.h"
#include "HslColor.h"


HslColor::HslColor(RgbColor color)
{
    // convert colors to float between (0.0 - 1.0)
    float r = color.R / 255.0f;
    float g = color.G / 255.0f;
    float b = color.B / 255.0f;

    float max = (r > g && r > b) ? r : (g > b) ? g : b;
    float min = (r < g && r < b) ? r : (g < b) ? g : b;

    float h, s, l;
    l = (max + min) / 2.0f;

    if (max == min) 
    {
        h = s = 0.0f;
    }
    else 
    {
        float d = max - min;
        s = (l > 0.5f) ? d / (2.0f - (max + min)) : d / (max + min);

        if (r > g && r > b)
        {
            h = (g - b) / d + (g < b ? 6.0f : 0.0f);
        }
        else if (g > b)
        {
            h = (b - r) / d + 2.0f;
        }
        else
        {
            h = (r - g) / d + 4.0f;
        }
        h /= 6.0f;
    }

    H = h;
    S = s;
    L = l;
}

HslColor HslColor::LinearBlend(HslColor left, HslColor right, float progress)
{
    return HslColor(left.H + ((right.H - left.H) * progress),
        left.S + ((right.S - left.S) * progress),
        left.L + ((right.L - left.L) * progress));
}