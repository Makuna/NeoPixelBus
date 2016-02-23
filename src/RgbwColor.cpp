/*-------------------------------------------------------------------------
RgbwColor provides a color object that can be directly consumed by NeoPixelBus

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
#include "HsbColor.h"
#include "RgbwColor.h"

static float _CalcColor(float p, float q, float t)
{
    if (t < 0.0f)
        t += 1.0f;
    if (t > 1.0f)
        t -= 1.0f;

    if (t < 1.0f / 6.0f)
        return p + (q - p) * 6.0f * t;

    if (t < 0.5f)
        return q;

    if (t < 2.0f / 3.0f)
        return p + ((q - p) * (2.0f / 3.0f - t) * 6.0f);

    return p;
}

RgbwColor::RgbwColor(HslColor color)
{
    RgbColor rgbColor(color);
    *this = rgbColor;
}

RgbwColor::RgbwColor(HsbColor color)
{
    RgbColor rgbColor(color);
    *this = rgbColor;
}

uint8_t RgbwColor::CalculateBrightness() const
{
    if (IsColorLess())
    {
        return W;
    }
    else
    {
        return (uint8_t)(((uint16_t)R + (uint16_t)G + (uint16_t)B) / 3);
    }
}

void RgbwColor::Darken(uint8_t delta)
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

    if (W > delta)
    {
        W -= delta;
    }
    else
    {
        W = 0;
    }
}

void RgbwColor::Lighten(uint8_t delta)
{
    if (IsColorLess())
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

RgbwColor RgbwColor::LinearBlend(RgbwColor left, RgbwColor right, float progress)
{
	return RgbwColor( left.R + ((right.R - left.R) * progress),
		left.G + ((right.G - left.G) * progress),
		left.B + ((right.B - left.B) * progress),
        left.W + ((right.W - left.W) * progress) );
}