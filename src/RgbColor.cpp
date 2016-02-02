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

#include "RgbColor.h"
#include "HslColor.h"
#include "HsbColor.h"

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

RgbColor::RgbColor(HslColor color)
{
    //Serial.print("HSL to RGB : ");
    //Serial.print(color.H);
    //Serial.print(", ");
    //Serial.print(color.S);
    //Serial.print(", ");
    //Serial.print(color.L);
    //Serial.print(" => ");

    float r;
    float g;
    float b;
#ifdef HSL_FLOAT
    float h = color.H;
    float s = color.S;
    float l = color.L;
#else
    float h = color.H / 255.0f;
    float s = color.S / 255.0f;
    float l = color.L / 255.0f;
#endif

    if (color.S == 0.0f || color.L == 0.0f)
    {
        r = g = b = l; // achromatic or black
    }
    else 
    {
        float q = l < 0.5f ? l * (1.0f + s) : l + s - (l * s);
        float p = 2.0f * l - q;
        r = _CalcColor(p, q, h + 1.0f / 3.0f);
        g = _CalcColor(p, q, h);
        b = _CalcColor(p, q, h - 1.0f / 3.0f);
    }

    //Serial.print(r);
    //Serial.print(", ");
    //Serial.print(g);
    //Serial.print(", ");
    //Serial.print(b);
    //Serial.print(" = ");

    R = (uint8_t)(r * 255.0f);
    G = (uint8_t)(g * 255.0f);
    B = (uint8_t)(b * 255.0f);

    //Serial.print(R);
    //Serial.print(", ");
    //Serial.print(G);
    //Serial.print(", ");
    //Serial.print(B);
    //Serial.println();
}

RgbColor::RgbColor(HsbColor color)
{
    //Serial.print("HSB to RGB : ");
    //Serial.print(color.H);
    //Serial.print(", ");
    //Serial.print(color.S);
    //Serial.print(", ");
    //Serial.print(color.B);
    //Serial.print(" => ");

    float r;
    float g;
    float b;
#ifdef HSB_FLOAT
    float h = color.H;
    float s = color.S;
    float v = color.B;
#else
    float h = color.H / 255.0f;
    float s = color.S / 255.0f;
    float v = color.B / 255.0f;
#endif

    if (color.S == 0.0f)
    {
        r = g = b = v; // achromatic or black
    }
    else
    {
        if (h < 0.0f)
            h += 1.0f;
        if (h > 1.0f)
            h -= 1.0f;
        h *= 6.0f;
        int i = (int)h;
        float f = h - i;
        float q = v * (1.0f - s * f);
        float p = v * (1.0f - s);
        float t = v * (1.0f - s * (1.0f - f));
        switch (i)
        {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        default:
            r = v;
            g = p;
            b = q;
            break;
        }
    }

    //Serial.print(r);
    //Serial.print(", ");
    //Serial.print(g);
    //Serial.print(", ");
    //Serial.print(b);
    //Serial.print(" = ");

    R = (uint8_t)(r * 255.0f);
    G = (uint8_t)(g * 255.0f);
    B = (uint8_t)(b * 255.0f);

    //Serial.print(R);
    //Serial.print(", ");
    //Serial.print(G);
    //Serial.print(", ");
    //Serial.print(B);
    //Serial.println();
}

uint8_t RgbColor::CalculateBrightness() const
{
	return (uint8_t)(((uint16_t)R + (uint16_t)G + (uint16_t)B) / 3);
}

void RgbColor::Darken(uint8_t delta)
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
}

void RgbColor::Lighten(uint8_t delta)
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

RgbColor RgbColor::LinearBlend(RgbColor left, RgbColor right, float progress)
{
	return RgbColor( left.R + ((right.R - left.R) * progress),
		left.G + ((right.G - left.G) * progress),
		left.B + ((right.B - left.B) * progress));
}