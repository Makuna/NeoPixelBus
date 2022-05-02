/*-------------------------------------------------------------------------
NeoSm168xxColorFeatures provides feature classes to describe color order and
color depth for NeoPixelBus template class specific to the SM1680 chip

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
/*
3 channel RGB
SM16803PB  1.8~60mA
SM16813P 1.75~19mA
SM16823E 60~350mA
4 channel RGBW
SM16804PB 1.8~19mA
SM16804EB 1.5~60mA
SM16824E 60~350mA
*/

class NeoSm168x3SettingsBase : public NeoRgbCurrentSettings
{
public:
    NeoSm168x3SettingsBase(uint16_t red, uint16_t green, uint16_t blue) :
        NeoRgbCurrentSettings(red, green, blue) {}

protected:
    static uint16_t limit(const uint16_t min, const uint16_t max, uint16_t value)
    {
        if (value < min)
        {
            value = min;
        }
        else if (value > max)
        {
            value = max;
        }
        return value;
    }

    static uint8_t encode(const uint16_t min, const uint16_t max, uint16_t value)
    {
        return (limit(value) - min) / ((max - min) / 16);
    }
};

class NeoSm168x4SettingsBase : public NeoRgbwCurrentSettings
{
public:
    NeoSm168x4SettingsBase(uint16_t red, uint16_t green, uint16_t blue, uint16_t white) :
        NeoRgbwCurrentSettings(red, green, blue, white) {}

protected:
    static uint16_t limit((const uint16_t min, const uint16_t max, uint16_t value))
    {
        if (value < min)
        {
            value = min;
        }
        else if (value > max)
        {
            value = max;
        }
        return value;
    }

    static uint8_t encode(const uint16_t min, const uint16_t max, uint16_t value)
    {
        return (limit(value) - min) / ((max - min) / 16);
    }
};

class NeoSm16803pbSettings : public NeoSm168x3SettingsBase
{
public:
    NeoSm16803pbSettings(uint16_t red, uint16_t green, uint16_t blue)  :
        NeoSm168x3SettingsBase(red, green, blue)
    {
    }

    const static uint16_t MinCurrent = 18;
    const static uint16_t MaxCurrent = 600;

    static uint16_t LimitCurrent(uint16_t value)
    {
        return limit(MinCurrent, MaxCurrent, value);
    }

    static uint8_t Encode(uint16_t value)
    {
        return encode(MinCurrent, MaxCurrent, value);
    }
};

class NeoSm16813pSettings : public NeoSm168x3SettingsBase
{
public:
    NeoSm16813pSettings(uint16_t red, uint16_t green, uint16_t blue) :
        NeoSm168x3SettingsBase(red, green, blue)
    {
    }

    const static uint16_t MinCurrent = 17;
    const static uint16_t MaxCurrent = 190;

    static uint16_t LimitCurrent(uint16_t value)
    {
        return limit(MinCurrent, MaxCurrent, value);
    }

    static uint8_t Encode(uint16_t value)
    {
        return encode(MinCurrent, MaxCurrent, value);
    }
};

class NeoSm16823eSettings : public NeoSm168x3SettingsBase
{
public:
    NeoSm16823eSettings(uint16_t red, uint16_t green, uint16_t blue) :
        NeoSm168x3SettingsBase(red, green, blue)
    {
    }

    const static uint16_t MinCurrent = 600;
    const static uint16_t MaxCurrent = 3500;
 
    static uint16_t LimitCurrent(uint16_t value)
    {
        return limit(MinCurrent, MaxCurrent, value);
    }

    static uint8_t Encode(uint16_t value)
    {
        return encode(MinCurrent, MaxCurrent, value);
    }
};

class NeoSm16804pbSettings : public NeoSm168x4SettingsBase
{
public:
    NeoSm16804pbSettings(uint16_t red, uint16_t green, uint16_t blue, uint16_t white) :
        NeoSm168x4SettingsBase(red, green, blue, white)
    {
    }

    const static uint16_t MinCurrent = 18;
    const static uint16_t MaxCurrent = 190;

    static uint16_t LimitCurrent(uint16_t value)
    {
        return limit(MinCurrent, MaxCurrent, value);
    }

    static uint8_t Encode(uint16_t value)
    {
        return encode(MinCurrent, MaxCurrent, value);
    }
};

class NeoSm16804ebSettings : public NeoSm168x4SettingsBase
{
public:
    NeoSm16804ebSettings(uint16_t red, uint16_t green, uint16_t blue, uint16_t white) :
        NeoSm168x4SettingsBase(red, green, blue, white)
    {
    }

    const static uint16_t MinCurrent = 15;
    const static uint16_t MaxCurrent = 600;

    static uint16_t LimitCurrent(uint16_t value)
    {
        return limit(MinCurrent, MaxCurrent, value);
    }

    static uint8_t Encode(uint16_t value)
    {
        return encode(MinCurrent, MaxCurrent, value);
    }
};

class NeoSm16824eSettings : public NeoSm168x4SettingsBase
{
public:
    NeoSm168xxSettings(uint16_t red, uint16_t green, uint16_t blue, uint16_t white) :
        NeoSm168x4SettingsBase(red, green, blue, white)
    {
    }

    const static uint16_t MinCurrent = 600;
    const static uint16_t MaxCurrent = 3500;

    static uint16_t LimitCurrent(uint16_t value)
    {
        return limit(MinCurrent, MaxCurrent, value);
    }

    static uint8_t Encode(uint16_t value)
    {
        return encode(MinCurrent, MaxCurrent, value);
    }
};

template<typename T_SETTINGS> class NeoElementsSm168x4Settings : public Neo4ByteElements
{
public:
    typedef T_SETTINGS SettingsObject;
    static const size_t SettingsSize = 2;

    static void applySettings([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData, [[maybe_unused]] const SettingsObject& settings)
    {
        // settings are at the end of the data stream
        uint8_t* pSet = pData + sizeData - SettingsSize;

        uint8_t red = T_SETTINGS::Encode(settings.RedTenthMilliAmpere);
        uint8_t green = T_SETTINGS::Encode(settings.GreenTenthMilliAmpere);
        uint8_t blue = T_SETTINGS::Encode(settings.BlueTenthMilliAmpere);
        uint8_t white = T_SETTINGS::Encode(settings.WhiteTenthMilliAmpere);

        // four bits per element in RGBW order
        *pSet++ = (red << 4) | green;
        *pSet++ = (blue << 4) | white;
    }

    static uint8_t* pixels([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        return pData;
    }

    static const uint8_t* pixels([[maybe_unused]] const uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        return pData;
    }
};

template<typename T_SETTINGS> class NeoElementsSm168x3Settings : public Neo3ByteElements
{
public:
    typedef T_SETTINGS SettingsObject;
    static const size_t SettingsSize = 2;

    static void applySettings([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData, [[maybe_unused]] const SettingsObject& settings)
    {
        // settings are at the end of the data stream
        uint8_t* pSet = pData + sizeData - SettingsSize;

        uint8_t red = T_SETTINGS::Encode(settings.RedTenthMilliAmpere);
        uint8_t green = T_SETTINGS::Encode(settings.GreenTenthMilliAmpere);
        uint8_t blue = T_SETTINGS::Encode(settings.BlueTenthMilliAmpere);
        
        // four bits per element in xRGB order
        *pSet++ = (0xf0) | red;
        *pSet++ = (green << 4) | blue;
    }

    static uint8_t* pixels([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        return pData;
    }

    static const uint8_t* pixels([[maybe_unused]] const uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        return pData;
    }
};

class NeoRgbwSm168xxFeature : public Neo4ByteElementsSm168xxSettings
{
public:
    static void applyPixelColor(uint8_t* pPixels, uint16_t indexPixel, ColorObject color)
    {
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        *p++ = color.R;
        *p++ = color.G;
        *p++ = color.B;
        *p = color.W;
    }

    static ColorObject retrievePixelColor(const uint8_t* pPixels, uint16_t indexPixel)
    {
        ColorObject color;
        const uint8_t* p = getPixelAddress(pPixels, indexPixel);

        color.R = *p++;
        color.G = *p++;
        color.B = *p++;
        color.W = *p;

        return color;
    }
    
    static ColorObject retrievePixelColor_P(PGM_VOID_P pPixels, uint16_t indexPixel)
    {
        ColorObject color;
        const uint8_t* p = getPixelAddress((const uint8_t*)pPixels, indexPixel);

        color.R = pgm_read_byte(p++);
        color.G = pgm_read_byte(p++);
        color.B = pgm_read_byte(p++);
        color.W = pgm_read_byte(p);

        return color;
    }
    
};

