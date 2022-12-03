/*-------------------------------------------------------------------------
MonoColor provides a color object that can be directly consumed by NeoPixelBus

Written by Grzegorz Bialokziewicz (based on Michael C. Miller code).

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

#include "MonoColor.h"
#include "RgbColor.h"
#include "Rgb48Color.h"
#include "HslColor.h"
#include "HsbColor.h"
#include "HtmlColor.h"

MonoColor::MonoColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    W = max(static_cast<uint8_t>(r * 0.3 + g * 0.59 + b * 0.11), w);
}

MonoColor::MonoColor(uint8_t brightness)
{
    W = brightness;
}

MonoColor::MonoColor(const RgbColor& color)
{
    W = color.R * 0.3 + color.G * 0.59 + color.B * 0.11;
}

MonoColor::MonoColor(const HtmlColor& color)
{
    uint32_t temp = color.Color;
    W = max(static_cast<uint8_t>(((temp >> 16) & 0xff) * 0.3 + ((temp >> 8) & 0xff) * 0.59 + (temp & 0xff) * 0.11), static_cast<uint8_t>((temp >> 24) & 0xff));
};

MonoColor::MonoColor(const HslColor& color)
{
    W = color.L * Max;
}

MonoColor::MonoColor(const HsbColor& color)
{
    W = color.B * Max;
}

uint8_t MonoColor::CalculateBrightness() const
{
    return W;
}

MonoColor MonoColor::Dim(uint8_t ratio) const
{
    // specifically avoids float math
    return MonoColor(_elementDim(W, ratio));
}

MonoColor MonoColor::Brighten(uint8_t ratio) const
{
    // specifically avoids float math
    return MonoColor(_elementBrighten(W, ratio));
}

void MonoColor::Darken(uint8_t delta)
{
    if (W > delta)
    {
        W -= delta;
    }
    else
    {
        W = 0;
    }
}

void MonoColor::Lighten(uint8_t delta)
{
    if (W < 255 - delta)
    {
        W += delta;
    }
    else
    {
        W = 255;
    }
}

MonoColor MonoColor::LinearBlend(const MonoColor& left, const MonoColor& right, float progress)
{
    return MonoColor(left.W + ((right.W - left.W) * progress));
}

MonoColor MonoColor::BilinearBlend(const MonoColor& c00, 
    const MonoColor& c01,
    const MonoColor& c10,
    const MonoColor& c11,
    float x,
    float y)
{
    float v00 = (1.0f - x) * (1.0f - y);
    float v10 = x * (1.0f - y);
    float v01 = (1.0f - x) * y;
    float v11 = x * y;

    return MonoColor(c00.W * v00 + c10.W * v10 + c01.W * v01 + c11.W * v11);
}