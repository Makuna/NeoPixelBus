/*-------------------------------------------------------------------------
Rgbww80Color provides a color object that can be directly consumed by NeoPixelBus

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by donating (see https://github.com/Makuna/NeoPixelBus)

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
#include "RgbwColor.h"
#include "Rgb48Color.h"
#include "Rgbw64Color.h"
#include "HslColor.h"
#include "HsbColor.h"
#include "HtmlColor.h"

#include "Rgbww80Color.h"

Rgbww80Color::Rgbww80Color(const RgbwColor& color)
{
    *this = Rgbw64Color(color);
};

Rgbww80Color::Rgbww80Color(const Rgbw64Color& color) :
    R(color.R),
    G(color.G),
    B(color.B),
    WW(color.W),
    CW(color.W)
{
};

uint16_t Rgbww80Color::CalculateBrightness() const
{
    uint16_t colorB = static_cast<uint16_t>((static_cast<uint32_t>(R) + static_cast<uint32_t>(G) + static_cast<uint32_t>(B)) / 3);
    uint16_t whiteB = static_cast<uint16_t>((static_cast<uint32_t>(WW) + static_cast<uint32_t>(CW)) / 2);

    return (whiteB > colorB) ? whiteB : colorB;
}

Rgbww80Color Rgbww80Color::Dim(uint16_t ratio) const
{
    // specifically avoids float math
    return Rgbww80Color(_elementDim(R, ratio), 
        _elementDim(G, ratio), 
        _elementDim(B, ratio), 
        _elementDim(WW, ratio), 
        _elementDim(CW, ratio));
}

Rgbww80Color Rgbww80Color::Brighten(uint16_t ratio) const
{
    // specifically avoids float math
    return Rgbww80Color(_elementBrighten(R, ratio), 
        _elementBrighten(G, ratio), 
        _elementBrighten(B, ratio), 
        _elementBrighten(WW, ratio), 
        _elementBrighten(CW, ratio));
}

void Rgbww80Color::Darken(uint16_t delta)
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

    if (CW > delta)
    {
        CW -= delta;
    }
    else
    {
        CW = 0;
    }
}

void Rgbww80Color::Lighten(uint16_t delta)
{
    if (IsColorLess())
    {
        if (WW < Max - delta)
        {
            WW += delta;
        }
        else
        {
            WW = Max;
        }

        if (CW < Max - delta)
        {
            CW += delta;
        }
        else
        {
            CW = Max;
        }
    }
    else
    {
        if (R < Max - delta)
        {
            R += delta;
        }
        else
        {
            R = Max;
        }

        if (G < Max - delta)
        {
            G += delta;
        }
        else
        {
            G = Max;
        }

        if (B < Max - delta)
        {
            B += delta;
        }
        else
        {
            B = Max;
        }
    }
}

Rgbww80Color Rgbww80Color::LinearBlend(const Rgbww80Color& left, const Rgbww80Color& right, float progress)
{
    return Rgbww80Color( left.R + ((static_cast<int32_t>(right.R) - left.R) * progress),
        left.G + ((static_cast<int32_t>(right.G) - left.G) * progress),
        left.B + ((static_cast<int32_t>(right.B) - left.B) * progress),
        left.WW + ((static_cast<int32_t>(right.WW) - left.WW) * progress),
        left.CW + ((static_cast<int32_t>(right.CW) - left.CW) * progress));
}

Rgbww80Color Rgbww80Color::LinearBlend(const Rgbww80Color& left, const Rgbww80Color& right, uint8_t progress)
{
    return Rgbww80Color(left.R + (((static_cast<int64_t>(right.R) - left.R) * static_cast<int64_t>(progress) + 1) >> 8),
        left.G + (((static_cast<int64_t>(right.G) - left.G) * static_cast<int64_t>(progress) + 1) >> 8),
        left.B + (((static_cast<int64_t>(right.B) - left.B) * static_cast<int64_t>(progress) + 1) >> 8),
        left.WW + (((static_cast<int64_t>(right.WW) - left.WW) * static_cast<int64_t>(progress) + 1) >> 8),
        left.CW + (((static_cast<int64_t>(right.CW) - left.CW) * static_cast<int64_t>(progress) + 1) >> 8));
}

Rgbww80Color Rgbww80Color::BilinearBlend(const Rgbww80Color& c00, 
    const Rgbww80Color& c01, 
    const Rgbww80Color& c10, 
    const Rgbww80Color& c11, 
    float x, 
    float y)
{
    float v00 = (1.0f - x) * (1.0f - y);
    float v10 = x * (1.0f - y);
    float v01 = (1.0f - x) * y;
    float v11 = x * y;

    return Rgbww80Color(
        c00.R * v00 + c10.R * v10 + c01.R * v01 + c11.R * v11,
        c00.G * v00 + c10.G * v10 + c01.G * v01 + c11.G * v11,
        c00.B * v00 + c10.B * v10 + c01.B * v01 + c11.B * v11,
        c00.WW * v00 + c10.WW * v10 + c01.WW * v01 + c11.WW * v11,
        c00.CW * v00 + c10.CW * v10 + c01.CW * v01 + c11.CW * v11);
}