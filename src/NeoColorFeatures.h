/*-------------------------------------------------------------------------
NeoPixelFeatures provides feature classes to describe color order and
color depth for NeoPixelBus template class

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

class Neo3Elements
{
public:
    static const size_t PixelSize = 3;
    static const RgbColor Empty();

    static uint8_t* getPixelAddress(uint8_t* pPixels, uint16_t indexPixel) 
    {
        return &pPixels[indexPixel * PixelSize];
    }

    typedef RgbColor ColorObject;

};

class Neo4Elements
{
public:
    static const size_t PixelSize = 4;
    static const RgbColor Empty();

    static uint8_t* getPixelAddress(uint8_t* pPixels, uint16_t indexPixel) 
    {
        return &pPixels[indexPixel * PixelSize];
    }

    typedef RgbwColor ColorObject;
};

class NeoGrbFeature : public Neo3Elements
{
public:
    static void applyPixelColor(uint8_t* pPixels, uint16_t indexPixel, Neo3Elements::ColorObject color)
    {
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        *p++ = color.G;
        *p++ = color.R;
        *p = color.B;
    }

    static Neo3Elements::ColorObject retrievePixelColor(uint8_t* pPixels, uint16_t indexPixel)
    {
        Neo3Elements::ColorObject color;
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        color.G = *p++;
        color.R = *p++;
        color.B = *p;

        return color;
    }
};

class NeoRgbwFeature : public Neo4Elements
{
public:
    static void applyPixelColor(uint8_t* pPixels, uint16_t indexPixel, Neo4Elements::ColorObject color)
    {
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        *p++ = color.R;
        *p++ = color.G;
        *p++ = color.B;
        *p = color.W;
    }

    static Neo4Elements::ColorObject retrievePixelColor(uint8_t* pPixels, uint16_t indexPixel)
    {
        Neo4Elements::ColorObject color;
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        color.R = *p++;
        color.G = *p++;
        color.B = *p++;
        color.W = *p;

        return color;
    }
};

class NeoRgbFeature : public Neo3Elements
{
public:
    static void applyPixelColor(uint8_t* pPixels, uint16_t indexPixel, Neo3Elements::ColorObject color)
    {
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        *p++ = color.R;
        *p++ = color.G;
        *p = color.B;
    }

    static Neo3Elements::ColorObject retrievePixelColor(uint8_t* pPixels, uint16_t indexPixel)
    {
        Neo3Elements::ColorObject color;
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        color.R = *p++;
        color.G = *p++;
        color.B = *p;

        return color;
    }
};

class NeoBrgFeature : public Neo3Elements
{
public:
    static void applyPixelColor(uint8_t* pPixels, uint16_t indexPixel, Neo3Elements::ColorObject color)
    {
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        *p++ = color.B;
        *p++ = color.R;
        *p = color.G;
    }

    static Neo3Elements::ColorObject retrievePixelColor(uint8_t* pPixels, uint16_t indexPixel)
    {
        Neo3Elements::ColorObject color;
        uint8_t* p = getPixelAddress(pPixels, indexPixel);

        color.B = *p++;
        color.R = *p++;
        color.G = *p;

        return color;
    }
};
