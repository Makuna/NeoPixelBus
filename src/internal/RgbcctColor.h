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
#pragma once

#include <Arduino.h>

#include "RgbwColor.h"
#include "lib8tion/math8.h"

struct RgbColor;
struct RgbwColor;
struct HslColor;
struct HsbColor;
/**
 * 
 * Important note!
 * warm white is always byte 4, not 5, that is cold white
 * 
 * */

#define ENABLE_DEVFEATURE_RGBCCT_MANIPULATION
// ------------------------------------------------------------------------
// RgbcctColor represents a color object that is represented by Red, Green, Blue
// component values and an extra White component.  It contains helpful color 
// routines to manipulate the color.
// ------------------------------------------------------------------------
struct RgbcctColor
{
    typedef NeoRgbcctCurrentSettings SettingsObject;

    // ------------------------------------------------------------------------
    // Construct a RgbcctColor using R, G, B, W values (0-255)
    // ------------------------------------------------------------------------
    RgbcctColor(uint8_t r, uint8_t g, uint8_t b, uint8_t ww = 0, uint8_t wc = 0) :
        R(r), G(g), B(b), WW(ww), WC(wc)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a RgbColor using a single brightness value (0-255)
    // This works well for creating gray tone colors
    // (0) = black, (255) = white, (128) = gray
    // ------------------------------------------------------------------------
    RgbcctColor(uint8_t brightness) :
        R(0), G(0), B(0), WW(brightness), WC(brightness)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a RgbcctColor using RgbColor
    // ------------------------------------------------------------------------
    RgbcctColor(const RgbColor& color) :
        R(color.R),
        G(color.G),
        B(color.B),
        WW(0),
        WC(0)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a RgbcctColor using RgbWColor
    // ------------------------------------------------------------------------
    RgbcctColor(const RgbwColor& color) :
        R(color.R),
        G(color.G),
        B(color.B),
        WW(color.W),    //to be renamed W1/W2, since it could be warm white or cold white
        WC(color.W)
        // R(0),G(0),B(0),WC(0)    
    {
    };

    // ------------------------------------------------------------------------
    // Construct a RgbcctColor using HtmlColor
    // ------------------------------------------------------------------------
    RgbcctColor(const HtmlColor& color);

    // ------------------------------------------------------------------------
    // Construct a RgbcctColor using HslColor
    // ------------------------------------------------------------------------
    RgbcctColor(const HslColor& color);

    // ------------------------------------------------------------------------
    // Construct a RgbcctColor using HsbColor
    // ------------------------------------------------------------------------
    RgbcctColor(const HsbColor& color);

    // ------------------------------------------------------------------------
    // Construct a RgbcctColor that will have its values set in latter operations
    // CAUTION:  The R,G,B, W members are not initialized and may not be consistent
    // ------------------------------------------------------------------------
    RgbcctColor()
    {
    };

    // RgbcctColor RgbcctColor(uint32_t color32bit)
    // {
    //     return RgbcctColor(color32bit & 0x000F);
    // }

    // ------------------------------------------------------------------------
    // Comparison operators
    // ------------------------------------------------------------------------
    bool operator==(const RgbcctColor& other) const
    {
        return (R == other.R && G == other.G && B == other.B && WW == other.WW && WC == other.WC);
    };

    bool operator!=(const RgbcctColor& other) const
    {
        return !(*this == other);
    };

    // ------------------------------------------------------------------------
    // Returns if the color is grey, all values are equal other than white
    // ------------------------------------------------------------------------
    bool IsMonotone() const
    {
        return (R == B && R == G);
    };


   /// add one RgbcctColor to another, saturating at 0xFF for each channel
    inline RgbcctColor& operator+= (const RgbcctColor& rhs )
    {
        R = qadd8( R, rhs.R);
        G = qadd8( G, rhs.G);
        B = qadd8( B, rhs.B);
        WW = qadd8( WW, rhs.WW);
        WC = qadd8( WC, rhs.WC);
        return *this;
    }

    /// scale down a RGB to N 256ths of it's current brightness, using
    /// 'plain math' dimming rules, which means that if the low light levels
    /// may dim all the way to 100% black.
    inline RgbcctColor& nscale8 (const RgbcctColor & scaledown )
    {
        R = ::scale8(R, scaledown.R);
        G = ::scale8(G, scaledown.G);
        B = ::scale8(B, scaledown.B);
        return *this;
    }





   /// add one RgbcctColor to another, saturating at 0xFF for each channel
    inline RgbcctColor& operator-= (const RgbcctColor& rhs )
    {
        R = qsub8( R, rhs.R);
        G = qsub8( G, rhs.G);
        B = qsub8( B, rhs.B);
        WW = qsub8( WW, rhs.WW);
        WC = qsub8( WC, rhs.WC);
        return *this;
    }



    // ------------------------------------------------------------------------
    // Returns if the color components are all zero, the white component maybe 
    // anything
    // ------------------------------------------------------------------------
    bool IsColorLess() const
    {
        return (R == 0 && B == 0 && G == 0);
    };

    // ------------------------------------------------------------------------
    // CalculateBrightness will calculate the overall brightness
    // NOTE: This is a simple linear brightness
    // ------------------------------------------------------------------------
    uint8_t CalculateBrightness() const;

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
    static RgbcctColor LinearBlend(const RgbcctColor& left, const RgbcctColor& right, float progress);
    
    // ------------------------------------------------------------------------
    // BilinearBlend between four colors by the amount defined by 2d variable
    // c00 - upper left quadrant color
    // c01 - upper right quadrant color
    // c10 - lower left quadrant color
    // c11 - lower right quadrant color
    // x - unit value (0.0 - 1.0) that defines the blend progress in horizontal space
    // y - unit value (0.0 - 1.0) that defines the blend progress in vertical space
    // ------------------------------------------------------------------------
    static RgbcctColor BilinearBlend(const RgbcctColor& c00, 
        const RgbcctColor& c01, 
        const RgbcctColor& c10, 
        const RgbcctColor& c11, 
        float x, 
        float y);

    // ------------------------------------------------------------------------
    // Red, Green, Blue, White color members (0-255) where 
    // (0,0,0,0) is black and (255,255,255, 0) and (0,0,0,255) is white
    // Note (255,255,255,255) is extreme bright white
    // ------------------------------------------------------------------------
    // enum{
    // uint8_t R;
    // uint8_t G;
    // uint8_t B;
    // uint8_t WW;
    // uint8_t WC;

    /**
     * @brief 
     * Conversion from Rgbcct into uint32_t (dropping the second white colour)
     * 
     */
    static uint32_t GetU32Colour(RgbcctColor c)
    {
        uint32_t c_out =
            ((uint32_t)c.warm_white << 24) |
            ((uint32_t)c.red        << 16) |
            ((uint32_t)c.green      <<  8) |
            ((uint32_t)c.blue            ) ;
        return c_out;
    }

    static RgbcctColor GetRgbcctFromU32Colour(uint32_t c)
    {
        
        uint8_t white = (uint8_t)(c >> 24);
        uint8_t red   = (uint8_t)(c >> 16);
        uint8_t green = (uint8_t)(c >> 8);
        uint8_t blue  = (uint8_t)(c);
        return RgbcctColor(red,green,blue,white);
    }

    // Unions allow overlapping of parameters, size of parameters below is only 5 bytes, but can be accessed by multiple ways
    union {
		struct {
            union {
                uint8_t R;
                uint8_t red;
            };
            union {
                uint8_t G;
                uint8_t green;
            };
            union {
                uint8_t B;
                uint8_t blue;
            };
            union {
                uint8_t WW;
                uint8_t W1; // for when the white channel does not matter e.g. rgbw
                uint8_t warm_white;
            };
            union {
                uint8_t WC;
                uint8_t W2;
                uint8_t white_cold;
            };
        };
		uint8_t raw[5];
	};

    inline uint8_t& operator[] (uint8_t x) __attribute__((always_inline))
    {
        return raw[x];
    }


#ifdef ENABLE_DEVFEATURE_RGBCCT_MANIPULATION








#endif // ENABLE_DEVFEATURE_RGBCCT_MANIPULATION

};

