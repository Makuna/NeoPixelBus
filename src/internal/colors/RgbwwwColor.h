/*-------------------------------------------------------------------------
RgbwwwColor provides a color object that can be directly consumed by NeoPixelBus

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
// RgbwwwColor represents a color object that is represented by Red, Green, Blue
// component values and three extra White components.  
// It contains helpful color routines to manipulate the color.
// ------------------------------------------------------------------------
struct RgbwwwColor : RgbColorBase
{
    typedef uint8_t ElementType;
    typedef NeoRgbwwwCurrentSettings SettingsObject;

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor using R, G, B, W1, W2, W3 values (0-255)
    // ------------------------------------------------------------------------
    RgbwwwColor(ElementType r, ElementType g, ElementType b, ElementType w1, ElementType w2, ElementType w3) :
        R(r), G(g), B(b), W1(w1), W2(w2), W3(w3)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor using R, G, B, (0-255) and a single W (0-765)
    // White is set incrementally across the three whites, 
    // where 1 = (0,0,1), 2 = (0,1,1), 3 = (1,1,1), 4 = (1,1,2)
    // ------------------------------------------------------------------------
    RgbwwwColor(ElementType r, ElementType g, ElementType b, uint16_t w) :
        R(r), G(g), B(b)
    {
        ApplyAsIncrementalWhite(w);
    };

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor using a single brightness value (0-255)
    // This works well for creating gray tone colors
    // (0) = black, (255) = white, (128) = gray
    // ------------------------------------------------------------------------
    RgbwwwColor(ElementType brightness) :
        R(0), G(0), B(0), W1(brightness), W2(brightness), W3(brightness)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor using RgbColor
    // ------------------------------------------------------------------------
    RgbwwwColor(const RgbColor& color) :
        R(color.R),
        G(color.G),
        B(color.B),
        W1(0),
        W2(0),
        W3(0)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor using RgbwColor
    // ------------------------------------------------------------------------
    RgbwwwColor(const RgbwColor& color) :
        R(color.R),
        G(color.G),
        B(color.B),
        W1(color.W),
        W2(color.W),
        W3(color.W)
    {
    };

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor using HtmlColor
    // ------------------------------------------------------------------------
    RgbwwwColor(const HtmlColor& color);

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor using HslColor
    // ------------------------------------------------------------------------
    RgbwwwColor(const HslColor& color);

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor using HsbColor
    // ------------------------------------------------------------------------
    RgbwwwColor(const HsbColor& color);

    // ------------------------------------------------------------------------
    // Construct a RgbwwwColor that will have its values set in latter operations
    // CAUTION:  The R,G,B, W1, W2, W3 members are not initialized and may not be consistent
    // ------------------------------------------------------------------------
    RgbwwwColor()
    {
    };

    // ------------------------------------------------------------------------
    // Comparison operators
    // ------------------------------------------------------------------------
    bool operator==(const RgbwwwColor& other) const
    {
        return (R == other.R && G == other.G && B == other.B && W1 == other.W1 && W2 == other.W2 && W3 == other.W3);
    };

    bool operator!=(const RgbwwwColor& other) const
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
    int16_t CompareTo(const RgbwwwColor& other, ElementType epsilon = 1)
    {
        return _Compare<RgbwwwColor, int16_t>(*this, other, epsilon);
    }

    // ------------------------------------------------------------------------
    // Compare method
    // compares two colors with the given epsilon (delta allowed)
    // returns the greatest difference of a set of elements, 
    //   0 = equal within epsilon delta
    //   negative - left is less than right
    //   positive - left is greater than right
    // ------------------------------------------------------------------------
    static int16_t Compare(const RgbwwwColor& left, const RgbwwwColor& right, ElementType epsilon = 1)
    {
        return _Compare<RgbwwwColor, int16_t>(left, right, epsilon);
    }

    // ------------------------------------------------------------------------
    // operator [] - readonly
    // access elements in order by index rather than R,G,B,W1,W2,W3
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
            return W1;
        case 4:
            return W2;
        default:
            return W3;
        }
    }

    // ------------------------------------------------------------------------
    // operator [] - read write
    // access elements in order by index rather than R,G,B,W1,W2
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
            return W1;
        case 4:
            return W2;
        default:
            return W3;
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
    // ratio - (0-255) where 255 will return the original color and 0 will return black
    // 
    // NOTE: This is a simple linear blend
    // ------------------------------------------------------------------------
    RgbwwwColor Dim(ElementType ratio) const;

    // ------------------------------------------------------------------------
    // Brighten will return a new color that is blended to white with the given ratio
    // ratio - (0-255) where 255 will return the original color and 0 will return white
    // 
    // NOTE: This is a simple linear blend
    // ------------------------------------------------------------------------
    RgbwwwColor Brighten(ElementType ratio) const;

    // ------------------------------------------------------------------------
    // Darken will adjust the color by the given delta toward black
    // NOTE: This is a simple linear change
    // delta - (0-255) the amount to dim the color
    // ------------------------------------------------------------------------
    void Darken(ElementType delta);

    // ------------------------------------------------------------------------
    // Lighten will adjust the color by the given delta toward white
    // NOTE: This is a simple linear change
    // delta - (0-255) the amount to lighten the color
    // ------------------------------------------------------------------------
    void Lighten(ElementType delta);

    // ------------------------------------------------------------------------
    // LinearBlend between two colors by the amount defined by progress variable
    // left - the color to start the blend at
    // right - the color to end the blend at
    // progress - (0.0 - 1.0) value where 0 will return left and 1.0 will return right
    //     and a value between will blend the color weighted linearly between them
    // ------------------------------------------------------------------------
    static RgbwwwColor LinearBlend(const RgbwwwColor& left, const RgbwwwColor& right, float progress);
    // progress - (0 - 255) value where 0 will return left and 255 will return right
    //     and a value between will blend the color weighted linearly between them
    // ------------------------------------------------------------------------
    static RgbwwwColor LinearBlend(const RgbwwwColor& left, const RgbwwwColor& right, uint8_t progress);

    // ------------------------------------------------------------------------
    // BilinearBlend between four colors by the amount defined by 2d variable
    // c00 - upper left quadrant color
    // c01 - upper right quadrant color
    // c10 - lower left quadrant color
    // c11 - lower right quadrant color
    // x - unit value (0.0 - 1.0) that defines the blend progress in horizontal space
    // y - unit value (0.0 - 1.0) that defines the blend progress in vertical space
    // ------------------------------------------------------------------------
    static RgbwwwColor BilinearBlend(const RgbwwwColor& c00, 
        const RgbwwwColor& c01, 
        const RgbwwwColor& c10, 
        const RgbwwwColor& c11, 
        float x, 
        float y);

    uint16_t CalcTotalTenthMilliAmpere(const SettingsObject& settings)
    {
        auto total = 0;

        total += R * settings.RedTenthMilliAmpere / Max;
        total += G * settings.GreenTenthMilliAmpere / Max;
        total += B * settings.BlueTenthMilliAmpere / Max;
        total += W1 * settings.W1TenthMilliAmpere / Max;
        total += W2 * settings.W2TenthMilliAmpere / Max;
        total += W3 * settings.W3TenthMilliAmpere / Max;

        return total;
    }

    // ------------------------------------------------------------------------
    // White is set incrementally across the three whites, 
    // white (0-765) 
    // where 1 = (0,0,1), 2 = (0,1,1), 3 = (1,1,1), 4 = (1,1,2)
    // ------------------------------------------------------------------------
    void ApplyAsIncrementalWhite(uint16_t white)
    {
        ElementType value = white / 3;
        ElementType remainder = white % 3;

        W1 = value;
        W2 = value;
        W3 = value;

        if (remainder)
        {
            W3++;
            if (remainder == 2)
            {
                W2++;
            }
        }
    }

    // ------------------------------------------------------------------------
    // Red, Green, Blue, Warm White, Cool White color members (0-255) where 
    // (0,0,0, 0,0,0) is black and 
    // (255,255,255, 0,0,0) is a white 
    // (0,0,0, 255,0,0) is white1 and
    // (0,0,0, 0,255,0) is white2 and
    // Note (255,255,255, 255,255,255) is extreme bright white
    // ------------------------------------------------------------------------
    ElementType R;
    ElementType G;
    ElementType B;
    ElementType W1;
    ElementType W2;
    ElementType W3;

    const static ElementType Max = 255;
    const static size_t Count = 6; // six elements in []
    const static size_t Size = Count * sizeof(ElementType);
    const static uint16_t MaxIncrementalWhite = 765;

private:
    inline static ElementType _elementDim(ElementType value, ElementType ratio)
    {
        return (static_cast<uint16_t>(value) * (static_cast<uint16_t>(ratio) + 1)) >> 8;
    }

    inline static ElementType _elementBrighten(ElementType value, ElementType ratio)
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

