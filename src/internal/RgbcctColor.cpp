/*-------------------------------------------------------------------------
RgbcctColor provides a color object that can be directly consumed by NeoPixelBus

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
#include "RgbwColor.h"
#include "HslColor.h"
#include "HsbColor.h"
#include "RgbcctColor.h"
#include "HtmlColor.h"

RgbcctColor::RgbcctColor(const HtmlColor& color)
{
    uint32_t temp = color.Color;
    B = (temp & 0xff);
    temp = temp >> 8;
    G = (temp & 0xff);
    temp = temp >> 8;
    R = (temp & 0xff);
    temp = temp >> 8;
    WW = (temp & 0xff);
};

RgbcctColor::RgbcctColor(const HslColor& color)
{
    RgbColor rgbColor(color);
    *this = rgbColor;
}

RgbcctColor::RgbcctColor(const HsbColor& color)
{
    RgbColor rgbColor(color);
    *this = rgbColor;
}

uint8_t RgbcctColor::CalculateBrightness() const
{
    uint8_t colorB = (uint8_t)(((uint16_t)R + (uint16_t)G + (uint16_t)B) / 3);
    if (WW > colorB)
    {
        return WW;
    }
    else if (WC > colorB)
    {
        return WC;
    }
    else
    {
        return colorB;
    }
}

void RgbcctColor::Darken(uint8_t delta)
{
    if (R > delta)
    {
        R -= delta;
    }
    else
    {
        R = 0;
    }

    if (G > delta)
    {
        G -= delta;
    }
    else
    {
        G = 0;
    }

    if (B > delta)
    {
        B -= delta;
    }
    else
    {
        B = 0;
    }

    if (WW > delta)
    {
        WW -= delta;
    }
    else
    {
        WW = 0;
    }
}

void RgbcctColor::Lighten(uint8_t delta)
{
    if (IsColorLess())
    {
        if (WW < 255 - delta)
        {
            WW += delta;
        }
        else
        {
            WW = 255;
        }
    }
    else
    {
        if (R < 255 - delta)
        {
            R += delta;
        }
        else
        {
            R = 255;
        }

        if (G < 255 - delta)
        {
            G += delta;
        }
        else
        {
            G = 255;
        }

        if (B < 255 - delta)
        {
            B += delta;
        }
        else
        {
            B = 255;
        }
    }
}

RgbcctColor RgbcctColor::LinearBlend(const RgbcctColor& left, const RgbcctColor& right, float progress)
{
    return RgbcctColor( 
        left.R +  ((right.R  - left.R)  * progress),
        left.G +  ((right.G  - left.G)  * progress),
        left.B +  ((right.B  - left.B)  * progress),
        left.WW + ((right.WW - left.WW) * progress),
        left.WC + ((right.WC - left.WC) * progress) 
    );
}

RgbcctColor RgbcctColor::BilinearBlend(const RgbcctColor& c00, 
    const RgbcctColor& c01, 
    const RgbcctColor& c10, 
    const RgbcctColor& c11, 
    float x, 
    float y)
{
    float v00 = (1.0f - x) * (1.0f - y);
    float v10 = x * (1.0f - y);
    float v01 = (1.0f - x) * y;
    float v11 = x * y;

    return RgbcctColor(
        c00.R * v00 + c10.R * v10 + c01.R * v01 + c11.R * v11,
        c00.G * v00 + c10.G * v10 + c01.G * v01 + c11.G * v11,
        c00.B * v00 + c10.B * v10 + c01.B * v01 + c11.B * v11,
        c00.WW * v00 + c10.WW * v10 + c01.WW * v01 + c11.WW * v11 );
}