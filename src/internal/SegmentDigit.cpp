/*-------------------------------------------------------------------------
SegmentDigit provides a color object that can be directly consumed by NeoPixelBus

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

#include "SegmentDigit.h"


void SevenSegDigit::init(uint8_t bitmask, uint8_t brightness, uint8_t defaultBrightness)
{
    for (uint8_t iSegment = 0; iSegment < SegmentCount; iSegment++)
    {
        Segment[iSegment] = (bitmask & 0x01) ? brightness : defaultBrightness;
        bitmask >>= 1;
    }
}

SevenSegDigit::SevenSegDigit(uint8_t bitmask, uint8_t brightness, uint8_t defaultBrightness)
{
    init(bitmask, brightness, defaultBrightness);
};

SevenSegDigit::SevenSegDigit(char letter, uint8_t brightness, uint8_t defaultBrightness)
{
    if (letter >= '0' && letter <= '9')
    {
        init(DecodeNumbers[letter - '0'], brightness, defaultBrightness);
    }
    else if (letter >= 'a' && letter <= 'z')
    {
        init(DecodeAlpha[letter - 'a'], brightness, defaultBrightness);
    }
    else if (letter >= 'A' && letter <= 'Z')
    {
        init(DecodeAlphaCaps[letter - 'A'], brightness, defaultBrightness);
    }
    else if (letter >= ',' && letter <= '/')
    {
        init(DecodeSpecial[letter - ','], brightness, defaultBrightness);
    }
    else
    {
        memset(Segment, defaultBrightness, sizeof(Segment));
    }
};

uint8_t SevenSegDigit::CalculateBrightness() const
{
    uint16_t sum = 0;

    for (uint8_t iSegment = 0; iSegment < SegmentCount; iSegment++)
    {
        sum += Segment[iSegment];
    }

	return (uint8_t)(sum / SegmentCount);
}

void SevenSegDigit::Darken(uint8_t delta)
{
    for (uint8_t iSegment = 0; iSegment < SegmentCount; iSegment++)
    {
        uint8_t element = Segment[iSegment];
        if (element > delta)
        {
            element -= delta;
        }
        else
        {
            element = 0;
        }
        Segment[iSegment] = element;
    }
}

void SevenSegDigit::Lighten(uint8_t delta)
{
    for (uint8_t iSegment = 0; iSegment < SegmentCount; iSegment++)
    {
        uint8_t element = Segment[iSegment];
        if (element < 255 - delta)
        {
            element += delta;
        }
        else
        {
            element = 255;
        }
        Segment[iSegment] = element;
    }
}

SevenSegDigit SevenSegDigit::LinearBlend(const SevenSegDigit& left, const SevenSegDigit& right, float progress)
{
    SevenSegDigit result;

    for (uint8_t iSegment = 0; iSegment < SegmentCount; iSegment++)
    {
        result.Segment[iSegment] = left.Segment[iSegment] + ((right.Segment[iSegment] - left.Segment[iSegment]) * progress);
    }
    return result;
}
