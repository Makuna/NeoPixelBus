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

uint8_t RgbColor::CalculateBrightness()
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

RgbColor RgbColor::LinearBlend(RgbColor left, RgbColor right, uint8_t progress)
{
	return RgbColor( left.R + ((right.R - left.R) * progress / 255),
		left.G + ((right.G - left.G) * progress / 255),
		left.B + ((right.B - left.B) * progress / 255));
}