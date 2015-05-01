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
#pragma once

#include <Arduino.h>

// ------------------------------------------------------------------------
// RgbColor represents a color object that is represented by Red, Green, Blue
// component values.  It contains helpful color routines to manipulate the 
// color.
// ------------------------------------------------------------------------
struct RgbColor
{
    // ------------------------------------------------------------------------
    // Construct a RgbColor using R, G, B values (0-255)
    // ------------------------------------------------------------------------
	RgbColor(uint8_t r, uint8_t g, uint8_t b) :
		R(r), G(g), B(b)
	{
	};

    // ------------------------------------------------------------------------
    // Construct a RgbColor using a single brightness value (0-255)
    // This works well for creating gray tone colors
    // (0) = blakc, (255) = white, (128) = gray
    // ------------------------------------------------------------------------
	RgbColor(uint8_t brightness) :
		R(brightness), G(brightness), B(brightness)
	{
	};

    // ------------------------------------------------------------------------
    // Construct a RgbColor that will have its values set in latter operations
    // CAUTION:  The R,G,B members are not initialized and may not be consistent
    // ------------------------------------------------------------------------
	RgbColor()
	{
	};

    // ------------------------------------------------------------------------
    // CalculateBrightness will calculate the overall brightness
    // NOTE: This is a simple linear brightness
    // ------------------------------------------------------------------------
	uint8_t CalculateBrightness();

    // ------------------------------------------------------------------------
    // Darken will adjust the color by the given delta toward black
    // NOTE: This is a simple linear change
    // delta - (0-255) the amount to dim the color
    // ------------------------------------------------------------------------
	void Darken(uint8_t delta);

    // ------------------------------------------------------------------------
    // Lighten will adjust the color by the given delta toward white
    // NOTE: This is a simple linear change
    // delta - (0-255) the amount to lighten the color
    // ------------------------------------------------------------------------
	void Lighten(uint8_t delta);

    // ------------------------------------------------------------------------
    // LinearBlend between two colors by the amount defined by progress variable
    // left - the color to start the blend at
    // right - the color to end the blend at
    // progress - (0-255) value where 0 will return left and 255 will return right
    //     and a value between will blend the color weighted linearly between them
    // ------------------------------------------------------------------------
	static RgbColor LinearBlend(RgbColor left, RgbColor right, uint8_t progress);
    
    // ------------------------------------------------------------------------
    // Red, Green, Blue color members (0-255) where 
    // (0,0,0) is black and (255,255,255) is white
    // ------------------------------------------------------------------------
	uint8_t R;
	uint8_t G;
	uint8_t B;
};

