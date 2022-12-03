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
#pragma once

#include <Arduino.h>
#include "NeoSettings.h"

struct RgbColor;
struct RgbwColor;
struct HslColor;
struct HsbColor;
struct HtmlColor;

// ------------------------------------------------------------------------
// MonoColor represents a color object that is represented by only White
// component. It contains helpful color routines to manipulate the color.
// ------------------------------------------------------------------------
struct MonoColor
{
    typedef NeoMonoCurrentSettings SettingsObject;

    // ------------------------------------------------------------------------
    // Construct a MonoColor using R, G, B, W values (0-255)
    // ------------------------------------------------------------------------
    MonoColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0);

    // ------------------------------------------------------------------------
    // Construct a MonoColor using a single brightness value (0-255)
    // This works well for creating gray tone colors
    // (0) = black, (255) = white, (128) = gray
    // ------------------------------------------------------------------------
    MonoColor(uint8_t brightness);

    // ------------------------------------------------------------------------
    // Construct a MonoColor using RgbColor
    // ------------------------------------------------------------------------
    MonoColor(const RgbColor& color);

    // ------------------------------------------------------------------------
    // Construct a MonoColor using HtmlColor
    // ------------------------------------------------------------------------
    MonoColor(const HtmlColor& color);

    // ------------------------------------------------------------------------
    // Construct a MonoColor using HslColor
    // ------------------------------------------------------------------------
    MonoColor(const HslColor& color);

    // ------------------------------------------------------------------------
    // Construct a MonoColor using HsbColor
    // ------------------------------------------------------------------------
    MonoColor(const HsbColor& color);

    // ------------------------------------------------------------------------
    // Construct a MonoColor that will have its values set in latter operations
    // CAUTION:  The R,G,B, W members are not initialized and may not be consistent
    // ------------------------------------------------------------------------
    MonoColor()
    {
    };

    // ------------------------------------------------------------------------
    // Comparison operators
    // ------------------------------------------------------------------------
    bool operator==(const MonoColor& other) const
    {
        return (W == other.W);
    };

    bool operator!=(const MonoColor& other) const
    {
        return !(*this == other);
    };

    // ------------------------------------------------------------------------
    // Returns if the color is grey, all values are equal other than white
    // ------------------------------------------------------------------------
    bool IsMonotone() const
    {
        return true;
    };

    // ------------------------------------------------------------------------
    // Returns if the color components are all zero, the white component maybe 
    // anything
    // ------------------------------------------------------------------------
    bool IsColorLess() const
    {
        return true;
    };

    // ------------------------------------------------------------------------
    // CalculateBrightness will calculate the overall brightness
    // NOTE: This is a simple linear brightness
    // ------------------------------------------------------------------------
    uint8_t CalculateBrightness() const;

    // ------------------------------------------------------------------------
    // Dim will return a new color that is blended to black with the given ratio
    // ratio - (0-255) where 255 will return the original color and 0 will return black
    // 
    // NOTE: This is a simple linear blend
    // ------------------------------------------------------------------------
    MonoColor Dim(uint8_t ratio) const;

    // ------------------------------------------------------------------------
    // Brighten will return a new color that is blended to white with the given ratio
    // ratio - (0-255) where 255 will return the original color and 0 will return white
    // 
    // NOTE: This is a simple linear blend
    // ------------------------------------------------------------------------
    MonoColor Brighten(uint8_t ratio) const;

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
    // progress - (0.0 - 1.0) value where 0 will return left and 1.0 will return right
    //     and a value between will blend the color weighted linearly between them
    // ------------------------------------------------------------------------
    static MonoColor LinearBlend(const MonoColor& left, const MonoColor& right, float progress);

    // ------------------------------------------------------------------------
    // BilinearBlend between four colors by the amount defined by 2d variable
    // c00 - upper left quadrant color
    // c01 - upper right quadrant color
    // c10 - lower left quadrant color
    // c11 - lower right quadrant color
    // x - unit value (0.0 - 1.0) that defines the blend progress in horizontal space
    // y - unit value (0.0 - 1.0) that defines the blend progress in vertical space
    // ------------------------------------------------------------------------
    static MonoColor BilinearBlend(const MonoColor& c00, 
        const MonoColor& c01,
        const MonoColor& c10,
        const MonoColor& c11,
        float x,
        float y);

    uint16_t CalcTotalTenthMilliAmpere(const SettingsObject& settings)
    {
        return W * settings.WhiteTenthMilliAmpere / Max;
    }

    // ------------------------------------------------------------------------
    // White color members (0-255) where 
    // (0) is black and (255) is white
    // ------------------------------------------------------------------------
    uint8_t W;

    const static uint8_t Max = 255;

private:
    inline static uint8_t _elementDim(uint8_t value, uint8_t ratio)
    {
        return (static_cast<uint16_t>(value) * (static_cast<uint16_t>(ratio) + 1)) >> 8;
    }

    inline static uint8_t _elementBrighten(uint8_t value, uint8_t ratio)
    {
        uint16_t element = ((static_cast<uint16_t>(value) + 1) << 8) / (static_cast<uint16_t>(ratio) + 1);

        if (element > Max)
        {
            element = Max;
        }
        else
        {
            element -= 1;
        }
        return element;
    }
};
