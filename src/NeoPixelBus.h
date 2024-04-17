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

// standard neo definitions
// 
const uint8_t NEO_DIRTY = 0x80; // a change was made to pixel data that requires a show
const uint16_t PixelIndex_OutOfBounds = 0xffff;

#include "internal/NeoUtil.h"
#include "internal/animations/NeoEase.h"
#include "internal/NeoSettings.h"
#include "internal/NeoColors.h"
#include "internal/NeoColorFeatures.h"
#include "internal/NeoTopologies.h"
#include "internal/NeoBuffers.h"
#include "internal/NeoBusChannel.h"
#include "internal/NeoMethods.h"


struct BusView
{
    BusView(uint16_t stripCount) :
        PixelCount(stripCount),
        StripCount(stripCount)
    {
    }

    BusView(uint16_t repeatedPixelsCount, 
            uint16_t stripCount ) :
        PixelCount(repeatedPixelsCount),
        StripCount(stripCount)
    {
    }

    const uint16_t PixelCount; // exposed count of pixels that will be repeated to fill strip
    const uint16_t StripCount; // actual data stream count of pixels
};

// T_COLOR_FEATURE - 
//    The color feature object that defines bit depth, order, and any settings related
//    to them
// 
// T_METHOD -
//    The led feature objec that defines specific timing and hardware used to send the data
//    stream on the pin
// 
// T_EXPOSED_COLOR_OBJECT- 
//    The color object to use for the front buffer, does not need to match the
//    T_COLOR_FEATURE::ColorObject but must be auto-converted, so no loss of data
// 
template<typename T_COLOR_FEATURE, 
    typename T_METHOD,
    typename T_EXPOSED_COLOR_OBJECT = typename T_COLOR_FEATURE::ColorObject,
    typename T_SHADER = NeoShaderNop<T_EXPOSED_COLOR_OBJECT, typename T_COLOR_FEATURE::ColorObject>>
class NeoPixelBus
{
public:
    NeoPixelBus(const BusView& busView, uint8_t pin) :
        _countPixels(busView.PixelCount),
        _pixels(nullptr),
        _method(pin, busView.StripCount, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize),
        _shader()
    {
    }

    NeoPixelBus(const BusView& busView, uint8_t pin, NeoBusChannel channel) :
        _countPixels(busView.PixelCount),
        _pixels(nullptr),
        _method(pin, busView.StripCount, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize, channel),
        _shader()
    {
    }

    NeoPixelBus(const BusView& busView, uint8_t pinClock, uint8_t pinData) :
        _countPixels(busView.PixelCount),
        _pixels(nullptr),
        _method(pinClock, pinData, busView.StripCount, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize),
        _shader()
    {
    }

    NeoPixelBus(const BusView& busView, uint8_t pinClock, uint8_t pinData, uint8_t pinLatch, uint8_t pinOutputEnable = NOT_A_PIN) :
        _countPixels(busView.PixelCount),
        _pixels(nullptr),
        _method(pinClock, pinData, pinLatch, pinOutputEnable, busView.StripCount, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize),
        _shader()
    {
    }

    NeoPixelBus(const BusView& busView) :
        _countPixels(busView.PixelCount),
        _pixels(nullptr),
        _method(busView.StripCount, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize),
        _shader()
    {
    }

    NeoPixelBus(const BusView& busView, Stream* pixieStream) :
        _countPixels(busView.PixelCount),
        _pixels(nullptr),
        _method(busView.StripCount, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize, pixieStream),
        _shader()
    {
    }

    ~NeoPixelBus()
    {
        delete[] _pixels;
    }


    operator NeoBufferContext<T_EXPOSED_COLOR_OBJECT>()
    {
        return NeoBufferContext<T_EXPOSED_COLOR_OBJECT>(_pixels, _countPixels);
    }


    void Begin()
    {
        _method.Initialize();
        _initialize();
    }

    // used by DotStarSpiMethod/DotStarEsp32DmaSpiMethod if pins can be configured
    void Begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
    {
        _method.Initialize(sck, miso, mosi, ss);
        _initialize();
    }

    // used by DotStarEsp32DmaSpiMethod if pins can be configured - reordered and extended version supporting quad SPI
    void Begin(int8_t sck, int8_t dat0, int8_t dat1, int8_t dat2, int8_t dat3, int8_t ss)
    {
        _method.Initialize(sck, dat0, dat1, dat2, dat3, ss);
        _initialize();
    }

    // used by DotStarEsp32DmaSpiMethod if pins can be configured - reordered and extended version supporting oct SPI
    void Begin(int8_t sck, int8_t dat0, int8_t dat1, int8_t dat2, int8_t dat3, int8_t dat4, int8_t dat5, int8_t dat6, int8_t dat7, int8_t ss)
    {
        _method.Initialize(sck, dat0, dat1, dat2, dat3, dat4, dat5, dat6, dat7, ss);
        _initialize();
    }

    void SetFeatureSettings(const typename T_COLOR_FEATURE::SettingsObject& settings)
    {
        _featureSettings = settings;
    }

    void SetMethodSettings(const typename T_METHOD::SettingsObject& settings)
    {
        _method.applySettings(settings);
    }

    void Show()
    {
        _method.template Update<T_EXPOSED_COLOR_OBJECT, T_COLOR_FEATURE, T_SHADER>(_pixels,
            _countPixels,
            _featureSettings,
            _shader);
    }

    inline bool CanShow() const
    { 
        return _method.IsReadyToUpdate();
    };

    T_EXPOSED_COLOR_OBJECT* Pixels()
    {
        return _pixels;
    };

    uint16_t PixelCount() const
    {
        return _countPixels;
    };

    void SetPixelColor(uint16_t indexPixel, typename T_COLOR_FEATURE::ColorObject color)
    {
        if (indexPixel < _countPixels)
        {
            _pixels[indexPixel] = color;
        }
    };

    T_EXPOSED_COLOR_OBJECT GetPixelColor(uint16_t indexPixel) const
    {
        if (indexPixel >= _countPixels)
        {
            return 0;
        }
        return _pixels[indexPixel];
    };

    template <typename T_COLOROBJECT> T_COLOROBJECT GetPixelColor(uint16_t indexPixel) const
    {
        return T_COLOROBJECT(GetPixelColor(indexPixel));
    }

    void ClearTo(const T_EXPOSED_COLOR_OBJECT& color)
    {
        ClearTo(color, 0, _countPixels - 1);
    }

    void ClearTo(const T_EXPOSED_COLOR_OBJECT& color, uint16_t first, uint16_t last)
    {
        if (first < _countPixels &&
            last < _countPixels &&
            first <= last)
        {
            T_EXPOSED_COLOR_OBJECT* pixels = _pixels + last + 1;
            T_EXPOSED_COLOR_OBJECT* pixelsFirst = _pixels + first;

            while (pixelsFirst <= --pixels)
            {
                *pixels = color;
            }
        }
    }

    void RotateLeft(uint16_t rotationCount)
    {
        if ((_countPixels - 1) >= rotationCount)
        {
            _rotateLeft(rotationCount, 0, _countPixels - 1);
        }
    }

    void RotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last)
    {
        if (first < _countPixels &&
            last < _countPixels &&
            first < last &&
            (last - first) >= rotationCount)
        {
            _rotateLeft(rotationCount, first, last);
        }
    }

    void ShiftLeft(uint16_t shiftCount)
    {
        if ((_countPixels - 1) >= shiftCount)
        {
            _shiftLeft(shiftCount, 0, _countPixels - 1);
        }
    }

    void ShiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last)
    {
        if (first < _countPixels && 
            last < _countPixels && 
            first < last &&
            (last - first) >= shiftCount)
        {
            _shiftLeft(shiftCount, first, last);
        }
    }

    void RotateRight(uint16_t rotationCount)
    {
        if ((_countPixels - 1) >= rotationCount)
        {
            _rotateRight(rotationCount, 0, _countPixels - 1);
        }
    }

    void RotateRight(uint16_t rotationCount, uint16_t first, uint16_t last)
    {
        if (first < _countPixels &&
            last < _countPixels &&
            first < last &&
            (last - first) >= rotationCount)
        {
            _rotateRight(rotationCount, first, last);
        }
    }

    void ShiftRight(uint16_t shiftCount)
    {
        if ((_countPixels - 1) >= shiftCount)
        {
            _shiftRight(shiftCount, 0, _countPixels - 1);
        }
    }

    void ShiftRight(uint16_t shiftCount, uint16_t first, uint16_t last)
    {
        if (first < _countPixels &&
            last < _countPixels &&
            first < last &&
            (last - first) >= shiftCount)
        {
            _shiftRight(shiftCount, first, last);
        }
    }
    
    void SwapPixelColor(uint16_t indexPixelOne, uint16_t indexPixelTwo)
    {
        auto colorOne = GetPixelColor(indexPixelOne);

        SetPixelColor(indexPixelOne, GetPixelColor(indexPixelTwo));
        SetPixelColor(indexPixelTwo, colorOne);
    };


    uint32_t CalcTotalMilliAmpere(const typename T_EXPOSED_COLOR_OBJECT::SettingsObject& settings)
    {
        uint32_t total = 0; // in 1/10th milliamps

        for (uint16_t index = 0; index < _countPixels; index++)
        {
            auto color = GetPixelColor(index);
            total += color.CalcTotalTenthMilliAmpere(settings);
        }

        return total / 10; // return millamps
    }

protected:
    const uint16_t _countPixels; // Number of RGB LEDs in strip

    T_EXPOSED_COLOR_OBJECT* _pixels;
    T_METHOD _method;
    T_SHADER _shader;
    typename T_COLOR_FEATURE::SettingsObject _featureSettings;

    void _initialize()
    {
        _pixels = new T_EXPOSED_COLOR_OBJECT[_countPixels];
        ClearTo(0);
    }

    void _rotateLeft(uint16_t rotationCount, uint16_t first, uint16_t last)
    {
        // store in temp
        T_EXPOSED_COLOR_OBJECT temp[rotationCount];

        for (uint16_t index = 0; index < rotationCount; index++)
        {
            temp[index] = _pixels[first + index];
        }

        // shift data
        _shiftLeft(rotationCount, first, last);

        // move temp back
        for (uint16_t index = 0; index < rotationCount; index++)
        {
            _pixels[last - (rotationCount - 1) + index] = temp[index];
        }
    }

    void _shiftLeft(uint16_t shiftCount, uint16_t first, uint16_t last)
    {
        uint16_t front = first + shiftCount;

        while (first <= last)
        {
            _pixels[first++] = _pixels[front++];
        }
    }

    void _rotateRight(uint16_t rotationCount, uint16_t first, uint16_t last)
    {
        // store in temp
        T_EXPOSED_COLOR_OBJECT temp[rotationCount];

        for (uint16_t index = 0; index < rotationCount; index++)
        {
            temp[index] = _pixels[last - (rotationCount - 1) + index];
        }

        // shift data
        _shiftRight(rotationCount, first, last);

        // move temp back
        for (uint16_t index = 0; index < rotationCount; index++)
        {
            _pixels[first + index] = temp[index];
        }
    }

    void _shiftRight(uint16_t shiftCount, uint16_t first, uint16_t last)
    {
        uint16_t back = last - shiftCount;

        while (first < last)
        {
            _pixels[last--] = _pixels[back--];
        }
    }
};


