/*-------------------------------------------------------------------------
NeoPixel library 

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

#include "internal/NeoHueBlend.h"

#include "internal/RgbColor.h"
#include "internal/HslColor.h"
#include "internal/HsbColor.h"
#include "internal/HtmlColor.h"
#include "internal/RgbwColor.h"

#include "internal/Layouts.h"
#include "internal/NeoTopology.h"
#include "internal/NeoTiles.h"
#include "internal/NeoMosaic.h"

#include "internal/NeoEase.h"
#include "internal/NeoGamma.h"

#include "internal/NeoColorFeatures.h"

#if defined(ARDUINO_ARCH_ESP8266)
#include "internal/NeoEsp8266DmaMethod.h"
#include "internal/NeoEsp8266UartMethod.h"
#include "internal/NeoEsp8266BitBangMethod.h"
#elif defined(__arm__) // must be before ARDUINO_ARCH_AVR due to Teensy incorrectly having it set
#include "internal/NeoArmMethod.h"
#elif defined(ARDUINO_ARCH_AVR)
#include "internal/NeoAvrMethod.h"
#else
#error "Platform Currently Not Supported, please add an Issue at Github/Makuna/NeoPixelBus"
#endif

// '_state' flags for internal state
#define NEO_DIRTY   0x80 // a change was made to pixel data that requires a show

template<typename T_COLOR_FEATURE, typename T_METHOD> class NeoPixelBus
{
public:
    // Constructor: number of LEDs, pin number
    // NOTE:  Pin Number maybe ignored due to hardware limitations of the method.
   
    NeoPixelBus(uint16_t countPixels, uint8_t pin) :
        _countPixels(countPixels),
        _method(pin, countPixels, T_COLOR_FEATURE::PixelSize)
    {
    }

    ~NeoPixelBus()
    {

    }

    void Begin()
    {
        _method.Initialize();
        Dirty();
    }

    void Show()
    {
        if (!IsDirty())
        {
            return;
        }

        _method.Update();

        ResetDirty();
    }

    inline bool CanShow() const
    { 
        return _method.IsReadyToUpdate();
    };

    bool IsDirty() const
    {
        return  (_state & NEO_DIRTY);
    };

    void Dirty()
    {
        _state |= NEO_DIRTY;
    };

    void ResetDirty()
    {
        _state &= ~NEO_DIRTY;
    };

    uint8_t* Pixels() const
    {
        return _method.getPixels();
    };

    size_t PixelsSize() const
    {
        return _method.getPixelsSize();
    };

    size_t PixelSize() const
    {
        return T_COLOR_FEATURE::PixelSize;
    };

    uint16_t PixelCount() const
    {
        return _countPixels;
    };

    void SetPixelColor(uint16_t indexPixel, typename T_COLOR_FEATURE::ColorObject color)
    {
        if (indexPixel < _countPixels)
        {
            T_COLOR_FEATURE::applyPixelColor(_method.getPixels(), indexPixel, color);
            Dirty();
        }
    };

    typename T_COLOR_FEATURE::ColorObject GetPixelColor(uint16_t indexPixel) const
    {
        if (indexPixel < _countPixels)
        {
            return T_COLOR_FEATURE::retrievePixelColor(_method.getPixels(), indexPixel);
        }
        else
        {
            // Pixel # is out of bounds, this will get converted to a 
            // color object type initialized to 0 (black)
            return 0;
        }
    };

    void ClearTo(typename T_COLOR_FEATURE::ColorObject color)
    {
        uint8_t temp[T_COLOR_FEATURE::PixelSize]; 

        T_COLOR_FEATURE::applyPixelColor(temp, 0, color);

        uint8_t* pixels = _method.getPixels();
        uint8_t* pFirst = T_COLOR_FEATURE::getPixelAddress(pixels, 0);
        uint8_t* pLast = T_COLOR_FEATURE::getPixelAddress(pixels, _countPixels);
        uint8_t* pFront = temp;
        while (pFirst < pLast)
        {
            T_COLOR_FEATURE::copyIncPixel(pFirst, pFront);
        }

        Dirty();
    };

    void RotateLeft(uint16_t rotationCount, uint16_t first = 0, uint16_t last = 0xffff)
    {
        if (last >= _countPixels)
        {
            last = _countPixels - 1;
        }

        if (first < _countPixels &&
            last < _countPixels &&
            first < last &&
            (last - first) >= rotationCount)
        {

            // store in temp
            uint8_t temp[rotationCount * T_COLOR_FEATURE::PixelSize];
            uint8_t* pixels = _method.getPixels();
            uint8_t* pFirst = T_COLOR_FEATURE::getPixelAddress(temp, 0);
            uint8_t* pLast = T_COLOR_FEATURE::getPixelAddress(temp, rotationCount - 1);
            uint8_t* pFront = T_COLOR_FEATURE::getPixelAddress(pixels, first);
            while (pFirst <= pLast)
            {
                T_COLOR_FEATURE::moveIncPixel(pFirst, pFront);
            }

            // shift data
            ShiftLeft(rotationCount, first, last);

            // move temp back
            pFirst = T_COLOR_FEATURE::getPixelAddress(temp, 0);
            pFront = T_COLOR_FEATURE::getPixelAddress(pixels, last - (rotationCount - 1));
            while (pFirst <= pLast)
            {
                T_COLOR_FEATURE::moveIncPixel(pFront, pFirst);
            }

            Dirty();
        }
    }

    void ShiftLeft(uint16_t shiftCount, uint16_t first = 0, uint16_t last = 0xffff)
    {
        if (last >= _countPixels)
        {
            last = _countPixels - 1;
        }

        if (first < _countPixels && 
            last < _countPixels && 
            first < last &&
            (last - first) >= shiftCount)
        {
            uint8_t* pixels = _method.getPixels();
            uint8_t* pFirst = T_COLOR_FEATURE::getPixelAddress(pixels, first);
            uint8_t* pLast = T_COLOR_FEATURE::getPixelAddress(pixels, last);
            uint8_t* pFront = T_COLOR_FEATURE::getPixelAddress(pixels, first + shiftCount);
            while (pFront <= pLast)
            {
                T_COLOR_FEATURE::moveIncPixel(pFirst, pFront);
            }

            Dirty();
        }
    }

    void RotateRight(uint16_t rotationCount, uint16_t first = 0, uint16_t last = 0xffff)
    {
        if (last >= _countPixels)
        {
            last = _countPixels - 1;
        }

        if (first < _countPixels &&
            last < _countPixels &&
            first < last &&
            (last - first) >= rotationCount)
        {

            // store in temp
            uint8_t temp[rotationCount * T_COLOR_FEATURE::PixelSize];
            uint8_t* pixels = _method.getPixels();
            uint8_t* pFirst = T_COLOR_FEATURE::getPixelAddress(temp, 0);
            uint8_t* pLast = T_COLOR_FEATURE::getPixelAddress(temp, rotationCount - 1);
            uint8_t* pBack = T_COLOR_FEATURE::getPixelAddress(pixels, last);
            while (pLast >= pFirst)
            {
                T_COLOR_FEATURE::moveDecPixel(pLast, pBack);
            }

            // shift data
            ShiftRight(rotationCount, first, last);

            // move temp back
            pLast = T_COLOR_FEATURE::getPixelAddress(temp, rotationCount - 1);
            pBack = T_COLOR_FEATURE::getPixelAddress(pixels, first + rotationCount - 1);
            while (pLast >= pFirst)
            {
                T_COLOR_FEATURE::moveDecPixel(pBack, pLast);
            }

            Dirty();
        }
    }

    void ShiftRight(uint16_t shiftCount, uint16_t first = 0, uint16_t last = 0xffff)
    {
        if (last >= _countPixels)
        {
            last = _countPixels - 1;
        }

        if (first < _countPixels &&
            last < _countPixels &&
            first < last &&
            (last - first) >= shiftCount)
        {
            uint8_t* pixels = _method.getPixels();
            uint8_t* pFirst = T_COLOR_FEATURE::getPixelAddress(pixels, first);
            uint8_t* pLast = T_COLOR_FEATURE::getPixelAddress(pixels, last);
            uint8_t* pBack = T_COLOR_FEATURE::getPixelAddress(pixels, last - shiftCount);
            while (pBack >= pFirst)
            {
                T_COLOR_FEATURE::moveDecPixel(pLast, pBack);
            }

            Dirty();
        }
    }

private:
    const uint16_t _countPixels; // Number of RGB LEDs in strip

    uint8_t _state;     // internal state
    T_METHOD _method;
};

