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

class NeoSm168xxSettings : public NeoRgbwCurrentSettings
{
public:
    NeoSm168xxSettings(uint16_t red, uint16_t green, uint16_t blue, uint16_t white)  :
        NeoRgbwCurrentSettings(red, green, blue, white)
    {
    }

    const static uint16_t MinCurrent = 60;
    const static uint16_t MaxCurrent = 350;

    static uint16_t LimitCurrent(uint16_t value)
    {
        if (value < MinCurrent)
        {
            value = MinCurrent;
        }
        else if (value > MaxCurrent)
        {
            value = MaxCurrent;
        }
        return value;
    }
};

class Neo4ByteElementsSm168xxSettings : public Neo4ByteElements
{
private:
    const static uint16_t EncodeDivisor = 19;

public:
    typedef NeoSm168xxSettings SettingsObject;
    static const size_t SettingsSize = 2;

    static void applySettings([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData, [[maybe_unused]] const SettingsObject& settings)
    {
        // settings are at the end of the data stream
        uint8_t* pSet = pData + sizeData - SettingsSize;

        // four bits per element in RGBW order
        *pSet++ = ((SettingsObject::LimitCurrent(settings.RedTenthMilliAmpere) - SettingsObject::MinCurrent) / EncodeDivisor) << 4 |
                ((SettingsObject::LimitCurrent(settings.GreenTenthMilliAmpere) - SettingsObject::MinCurrent) / EncodeDivisor);
        *pSet++ = ((SettingsObject::LimitCurrent(settings.BlueTenthMilliAmpere) - SettingsObject::MinCurrent) / EncodeDivisor) << 4 |
                ((SettingsObject::LimitCurrent(settings.WhiteCurrent) - SettingsObject::MinCurrent) / EncodeDivisor);

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

