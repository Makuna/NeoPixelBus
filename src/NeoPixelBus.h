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

#include "RgbColor.h"
#include "HslColor.h"
#include "HsbColor.h"
#include "RgbwColor.h"
#include "NeoColorFeatures.h"

#ifdef ARDUINO_ARCH_ESP8266
#include "NeoEsp8266DmaMethod.h"
#include "NeoEsp8266UartMethod.h"
#include "NeoEsp8266BitBangMethod.h"
#elif ARDUINO_ARCH_AVR
#include "NeoAvrMethod.h"
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
        _method.IsReadyToUpdate();
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
    }

    uint16_t PixelCount() const
    {
        return _countPixels;
    };

    void SetPixelColor(uint16_t n, typename T_COLOR_FEATURE::ColorObject c)
    {
        if (n < _countPixels)
        {
            T_COLOR_FEATURE::applyPixelColor(_method.getPixels(), n, c);
            Dirty();
        }
    };

    typename T_COLOR_FEATURE::ColorObject GetPixelColor(uint16_t n) const
    {
        if (n < _countPixels)
        {
            return T_COLOR_FEATURE::retrievePixelColor(_method.getPixels(), n);
        }
        else
        {
            // Pixel # is out of bounds, this will get converted to a 
            // color object type initialized to 0 (black)
            return 0;
        }
    };

    void ClearTo(typename T_COLOR_FEATURE::ColorObject c)
    {
        uint8_t* pixels = _method.getPixels();
        for (uint16_t n = 0; n < _countPixels; n++)
        {
            T_COLOR_FEATURE::applyPixelColor(pixels, n, c);
        }
        Dirty();
    };

private:
    const uint16_t _countPixels; // Number of RGB LEDs in strip

    uint8_t _state;     // internal state
    T_METHOD _method;
};

