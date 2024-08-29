/*-------------------------------------------------------------------------
RgbwwwColor provides a color object that can be directly consumed by NeoPixelBus

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
#include "HslColor.h"
#include "HsbColor.h"
#include "HtmlColor.h"

#include "RgbwwwColor.h"

RgbwwwColor::RgbwwwColor(const HtmlColor& color)
{
    uint32_t temp = color.Color;
    B = (temp & 0xff);
    temp = temp >> 8;
    G = (temp & 0xff);
    temp = temp >> 8;
    R = (temp & 0xff);
    temp = temp >> 8;
    W1 = (temp & 0xff);
    W2 = W1;
    W3 = W1;
};

RgbwwwColor::RgbwwwColor(const HslColor& color)
{
    RgbColor rgbColor(color);
    *this = rgbColor;
}

RgbwwwColor::RgbwwwColor(const HsbColor& color)
{
    RgbColor rgbColor(color);
    *this = rgbColor;
}

uint8_t RgbwwwColor::CalculateBrightness() const
{
    uint8_t colorB = static_cast<uint8_t>((static_cast<uint16_t>(R) + static_cast<uint16_t>(G) + static_cast<uint16_t>(B)) / 3);
    uint8_t whiteB = static_cast<uint8_t>((static_cast<uint16_t>(W1) + static_cast<uint16_t>(W2) + static_cast<uint16_t>(W3)) / 3);

    return (whiteB > colorB) ? whiteB : colorB;
}

RgbwwwColor RgbwwwColor::Dim(uint8_t ratio) const
{
    // specifically avoids float math
    return RgbwwwColor(_elementDim(R, ratio), 
        _elementDim(G, ratio), 
        _elementDim(B, ratio), 
        _elementDim(W1, ratio), 
        _elementDim(W2, ratio),
        _elementDim(W3, ratio));
}

RgbwwwColor RgbwwwColor::Brighten(uint8_t ratio) const
{
    // specifically avoids float math
    return RgbwwwColor(_elementBrighten(R, ratio), 
        _elementBrighten(G, ratio), 
        _elementBrighten(B, ratio), 
        _elementBrighten(W1, ratio), 
        _elementBrighten(W2, ratio),
        _elementBrighten(W3, ratio));
}

void RgbwwwColor::Darken(uint8_t delta)
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

    if (W1 > delta)
    {
        W1 -= delta;
    }
    else
    {
        W1 = 0;
    }

    if (W2 > delta)
    {
        W2 -= delta;
    }
    else
    {
        W2 = 0;
    }

    if (W3 > delta)
    {
        W3 -= delta;
    }
    else
    {
        W3 = 0;
    }
}

void RgbwwwColor::Lighten(uint8_t delta)
{
    if (IsColorLess())
    {
        if (W1 < Max - delta)
        {
            W1 += delta;
        }
        else
        {
            W1 = Max;
        }

        if (W2 < Max - delta)
        {
            W2 += delta;
        }
        else
        {
            W2 = Max;
        }

        if (W3 < Max - delta)
        {
            W3 += delta;
        }
        else
        {
            W3 = Max;
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

RgbwwwColor RgbwwwColor::LinearBlend(const RgbwwwColor& left, const RgbwwwColor& right, float progress)
{
    return RgbwwwColor( left.R + ((static_cast<int16_t>(right.R) - left.R) * progress),
        left.G + ((static_cast<int16_t>(right.G) - left.G) * progress),
        left.B + ((static_cast<int16_t>(right.B) - left.B) * progress),
        left.W1 + ((static_cast<int16_t>(right.W1) - left.W1) * progress),
        left.W2 + ((static_cast<int16_t>(right.W2) - left.W2) * progress),
        left.W3 + ((static_cast<int16_t>(right.W3) - left.W3) * progress));
}

RgbwwwColor RgbwwwColor::LinearBlend(const RgbwwwColor& left, const RgbwwwColor& right, uint8_t progress)
{
    return RgbwwwColor(left.R + (((static_cast<int32_t>(right.R) - left.R) * static_cast<int32_t>(progress) + 1) >> 8),
        left.G + (((static_cast<int32_t>(right.G) - left.G) * static_cast<int32_t>(progress) + 1) >> 8),
        left.B + (((static_cast<int32_t>(right.B) - left.B) * static_cast<int32_t>(progress) + 1) >> 8),
        left.W1 + (((static_cast<int32_t>(right.W1) - left.W1) * static_cast<int32_t>(progress) + 1) >> 8),
        left.W2 + (((static_cast<int32_t>(right.W2) - left.W2) * static_cast<int32_t>(progress) + 1) >> 8),
        left.W3 + (((static_cast<int32_t>(right.W3) - left.W3) * static_cast<int32_t>(progress) + 1) >> 8));
}

RgbwwwColor RgbwwwColor::BilinearBlend(const RgbwwwColor& c00, 
    const RgbwwwColor& c01, 
    const RgbwwwColor& c10, 
    const RgbwwwColor& c11, 
    float x, 
    float y)
{
    float v00 = (1.0f - x) * (1.0f - y);
    float v10 = x * (1.0f - y);
    float v01 = (1.0f - x) * y;
    float v11 = x * y;

    return RgbwwwColor(
        c00.R * v00 + c10.R * v10 + c01.R * v01 + c11.R * v11,
        c00.G * v00 + c10.G * v10 + c01.G * v01 + c11.G * v11,
        c00.B * v00 + c10.B * v10 + c01.B * v01 + c11.B * v11,
        c00.W1 * v00 + c10.W1 * v10 + c01.W1 * v01 + c11.W1 * v11,
        c00.W2 * v00 + c10.W2 * v10 + c01.W2 * v01 + c11.W2 * v11,
        c00.W3 * v00 + c10.W3 * v10 + c01.W3 * v01 + c11.W3 * v11);
}