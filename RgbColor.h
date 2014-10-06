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

#ifndef RGBCOLOR_H
#define RGBCOLOR_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif

struct RgbColor
{
	RgbColor(uint8_t r, uint8_t g, uint8_t b) :
		R(r), G(g), B(b)
	{
	};

	RgbColor(uint8_t brightness) :
		R(brightness), G(brightness), B(brightness)
	{
	};

	RgbColor()
	{
	};

	uint8_t CalculateBrightness();

	void Darken(uint8_t delta);
	void Lighten(uint8_t delta);

	static RgbColor LinearBlend(RgbColor left, RgbColor right, uint8_t progress);

	uint8_t R;
	uint8_t G;
	uint8_t B;
};


#endif // RGBCOLOR_H