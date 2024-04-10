/*-------------------------------------------------------------------------
Rgbww80Color provides a color object that can be directly consumed by NeoPixelBus

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

struct RgbColor;
struct HslColor;
struct HsbColor;

// ------------------------------------------------------------------------
// Rgbww80Color represents a color object that is represented by Red, Green, Blue
// component values and two extra White components.  
// While the white components are labeled as WW (warm) and CW (cool), they can be
// considered as the "warmer" and "cooler" whites of your LEDs; so that if yours
// have Nuetral and Cool, you could consider the WW as your nuetral. 
// It contains helpful color routines to manipulate the color.
// ------------------------------------------------------------------------
struct Rgbww80Color : RgbColorBase
{
    typedef uint16_t ElementType;
    typedef NeoRgbwwCurrentSettings SettingsObject;

    // ------------------------------------------------------------------------
    // Construct a Rgbww80Color using R, G, B, WW, CW values (0-65535)
    // ------------------------------------------------------------------------
    Rgbww80Color(ElementType r, ElementType g, ElementType b, ElementType warmW = 0, ElementType coolW = 0) :
        R(r), G(g), B(b), WW(warmW), CW(coolW)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a Rgbww80Color using a single brightness value (0-65535)
    // This works well for creating gray tone colors
    // (0) = black, (65535) = white, (128) = gray
    // ------------------------------------------------------------------------
    Rgbww80Color(ElementType brightness) :
        R(0), G(0), B(0), WW(brightness), CW(brightness)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a Rgbww80Color using RgbColor
    // ------------------------------------------------------------------------
    Rgbww80Color(const RgbColor& color) 
    {
        *this = Rgb48Color(color);
    };

    // ------------------------------------------------------------------------
    // Construct a Rgbw80Color using Rgb48Color
    // ------------------------------------------------------------------------
    Rgbww80Color(const Rgb48Color& color) :
        R(color.R),
        G(color.G),
        B(color.B),
        WW(0),
        CW(0)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a Rgbww80Color using RgbwColor
    // ------------------------------------------------------------------------
    Rgbww80Color(const RgbwColor& color);

    // ------------------------------------------------------------------------
    // Construct a Rgbw64Color using Rgb48Color
    // ------------------------------------------------------------------------
    Rgbww80Color(const Rgbw64Color& color);

    // ------------------------------------------------------------------------
    // Construct a Rgbww80Color using HtmlColor
    // ------------------------------------------------------------------------
    Rgbww80Color(const HtmlColor& color)
    {
        *this = RgbwColor(color);
    }

    // ------------------------------------------------------------------------
    // Construct a Rgbww80Color using HslColor
    // ------------------------------------------------------------------------
    Rgbww80Color(const HslColor& color)
    {
        *this = Rgb48Color(color);
    }

    // ------------------------------------------------------------------------
    // Construct a Rgbww80Color using HsbColor
    // ------------------------------------------------------------------------
    Rgbww80Color(const HsbColor& color)
    {
        *this = Rgb48Color(color);
    }

    // ------------------------------------------------------------------------
    // Construct a Rgbww80Color that will have its values set in latter operations
    // CAUTION:  The R,G,B, WW, CW members are not initialized and may not be consistent
    // ------------------------------------------------------------------------
    Rgbww80Color()
    {
    };

    // ------------------------------------------------------------------------
    // Comparison operators
    // ------------------------------------------------------------------------
    bool operator==(const Rgbww80Color& other) const
    {
        return (R == other.R && G == other.G && B == other.B && WW == other.WW && CW == other.CW);
    };

    bool operator!=(const Rgbww80Color& other) const
    {
        return !(*this == other);
    };

    // ------------------------------------------------------------------------
    // CompareTo method
    // compares against another color with the given epsilon (delta allowed)
    // returns the greatest difference of a set of elements, 
    //   0 = equal within epsilon delta
    //   negative - this is less than other
    //   positive - this is greater than other
    // ------------------------------------------------------------------------
    int32_t CompareTo(const Rgbww80Color& other, ElementType epsilon = 256)
    {
        return _Compare<Rgbww80Color, int32_t>(*this, other, epsilon);
    }

    // ------------------------------------------------------------------------
    // Compare method
    // compares two colors with the given epsilon (delta allowed)
    // returns the greatest difference of a set of elements, 
    //   0 = equal within epsilon delta
    //   negative - left is less than right
    //   positive - left is greater than right
    // ------------------------------------------------------------------------
    static int32_t Compare(const Rgbww80Color& left, const Rgbww80Color& right, ElementType epsilon = 256)
    {
        return _Compare<Rgbww80Color, int32_t>(left, right, epsilon);
    }

    // ------------------------------------------------------------------------
    // operator [] - readonly
    // access elements in order by index rather than R,G,B,WW,CW
    // see static Count for the number of elements
    // ------------------------------------------------------------------------
    ElementType operator[](size_t idx) const
    {
        switch (idx)
        {
        case 0:
            return R;
        case 1:
            return G;
        case 2:
            return B;
        case 3:
            return WW;
        default:
            return CW;
        }
    }

    // ------------------------------------------------------------------------
    // operator [] - read write
    // access elements in order by index rather than R,G,B,WW,CW
    // see static Count for the number of elements
    // ------------------------------------------------------------------------
    ElementType& operator[](size_t idx)
    {
        switch (idx)
        {
        case 0:
            return R;
        case 1:
            return G;
        case 2:
            return B;
        case 3:
            return WW;
        default:
            return CW;
        }
    }

    // ------------------------------------------------------------------------
    // Returns if the color is grey, all values are equal other than whites
    // ------------------------------------------------------------------------
    bool IsMonotone() const
    {
        return (R == B && R == G);
    };

    // ------------------------------------------------------------------------
    // Returns if the color components are all zero, the white components maybe 
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
    ElementType CalculateBrightness() const;

    // ------------------------------------------------------------------------
    // Dim will return a new color that is blended to black with the given ratio
    // ratio - (0-65535) where 65535 will return the original color and 0 will return black
    // 
    // NOTE: This is a simple linear blend
    // ------------------------------------------------------------------------
    Rgbww80Color Dim(ElementType ratio) const;

    // ------------------------------------------------------------------------
    // Dim will return a new color that is blended to black with the given ratio
    // ratio - (0-255) where 255 will return the original color and 0 will return black
    // 
    // NOTE: This is a simple linear blend
    // ------------------------------------------------------------------------
    Rgbww80Color Dim(uint8_t ratio) const
    {
        ElementType expanded = ratio << 8;
        return Dim(expanded);
    }

    // ------------------------------------------------------------------------
    // Brighten will return a new color that is blended to white with the given ratio
    // ratio - (0-65535) where 65535 will return the original color and 0 will return white
    // 
    // NOTE: This is a simple linear blend
    // ------------------------------------------------------------------------
    Rgbww80Color Brighten(ElementType ratio) const;

    // ------------------------------------------------------------------------
    // Brighten will return a new color that is blended to white with the given ratio
    // ratio - (0-255) where 255 will return the original color and 0 will return white
    // 
    // NOTE: This is a simple linear blend
    // ------------------------------------------------------------------------
    Rgbww80Color Brighten(uint8_t ratio) const
    {
        ElementType expanded = ratio << 8;
        return Brighten(expanded);
    }

    // ------------------------------------------------------------------------
    // Darken will adjust the color by the given delta toward black
    // NOTE: This is a simple linear change
    // delta - (0-65535) the amount to dim the color
    // ------------------------------------------------------------------------
    void Darken(ElementType delta);

    // ------------------------------------------------------------------------
    // Lighten will adjust the color by the given delta toward white
    // NOTE: This is a simple linear change
    // delta - (0-65535) the amount to lighten the color
    // ------------------------------------------------------------------------
    void Lighten(ElementType delta);

    // ------------------------------------------------------------------------
    // LinearBlend between two colors by the amount defined by progress variable
    // left - the color to start the blend at
    // right - the color to end the blend at
    // progress - (0.0 - 1.0) value where 0 will return left and 1.0 will return right
    //     and a value between will blend the color weighted linearly between them
    // ------------------------------------------------------------------------
    static Rgbww80Color LinearBlend(const Rgbww80Color& left, const Rgbww80Color& right, float progress);
    // progress - (0 - 255) value where 0 will return left and 255 will return right
    //     and a value between will blend the color weighted linearly between them
    // ------------------------------------------------------------------------
    static Rgbww80Color LinearBlend(const Rgbww80Color& left, const Rgbww80Color& right, uint8_t progress);

    // ------------------------------------------------------------------------
    // BilinearBlend between four colors by the amount defined by 2d variable
    // c00 - upper left quadrant color
    // c01 - upper right quadrant color
    // c10 - lower left quadrant color
    // c11 - lower right quadrant color
    // x - unit value (0.0 - 1.0) that defines the blend progress in horizontal space
    // y - unit value (0.0 - 1.0) that defines the blend progress in vertical space
    // ------------------------------------------------------------------------
    static Rgbww80Color BilinearBlend(const Rgbww80Color& c00, 
        const Rgbww80Color& c01, 
        const Rgbww80Color& c10, 
        const Rgbww80Color& c11, 
        float x, 
        float y);

    uint16_t CalcTotalTenthMilliAmpere(const SettingsObject& settings)
    {
        auto total = 0;

        total += R * settings.RedTenthMilliAmpere / Max;
        total += G * settings.GreenTenthMilliAmpere / Max;
        total += B * settings.BlueTenthMilliAmpere / Max;
        total += WW * settings.WarmWhiteTenthMilliAmpere / Max;
        total += CW * settings.CoolWhiteTenthMilliAmpere / Max;

        return total;
    }

    // ------------------------------------------------------------------------
    // Red, Green, Blue, Warm White, Cool White color members (0-65535) where 
    // (0,0,0,0,0) is black and 
    // (65535,65535,65535, 0, 0) is a white 
    // (0,0,0,65535,0) is warm white and
    // (0,0,0,0,65535) is cool white and
    // Note (65535,65535,65535,65535,65535) is extreme bright white
    // ------------------------------------------------------------------------
    ElementType R;
    ElementType G;
    ElementType B;
    ElementType WW;
    ElementType CW;

    const static ElementType Max = 65535;
    const static size_t Count = 5; // five elements in []
    const static size_t Size = Count * sizeof(ElementType);

private:
    inline static ElementType _elementDim(ElementType value, ElementType ratio)
    {
        return (static_cast<uint32_t>(value) * (static_cast<uint32_t>(ratio) + 1)) >> 16;
    }

    inline static ElementType _elementBrighten(ElementType value, ElementType ratio)
    {
        uint32_t element = ((static_cast<uint32_t>(value) + 1) << 16) / (static_cast<uint32_t>(ratio) + 1);

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

