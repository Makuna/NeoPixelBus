/*-------------------------------------------------------------------------
NeoPixelBus library wrapper template class that provides luminance and gamma control

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

#include "NeoPixelBus.h"

// T_EXPOSED_COLOR_OBJECT- 
//    The color object to use for the front buffer, does not need to match the
//    T_COLOR_FEATURE::ColorObject but must be auto-converted, so no loss of data
//    
// T_GAMMA - 
//    NeoGammaEquationMethod 
//    NeoGammaCieLabEquationMethod
//    NeoGammaTableMethod
//    NeoGammaNullMethod
//    NeoGammaInvert<one of the above>

template<typename T_COLOR_FEATURE, 
    typename T_METHOD, 
    typename T_EXPOSED_COLOR_OBJECT = typename T_COLOR_FEATURE::ColorObject,
    typename T_GAMMA = NeoGammaTableMethod>
class NeoPixelBusLg
{
public:
    class LuminanceShader
    {
    public:
        LuminanceShader(typename T_EXPOSED_COLOR_OBJECT::ElementType luminance = T_EXPOSED_COLOR_OBJECT::Max) :
            _luminance(luminance)
        {
        }

        typename T_COLOR_FEATURE::ColorObject Apply(const T_EXPOSED_COLOR_OBJECT& original) const
        {
            // dim and then return gamma adjusted
            typename T_COLOR_FEATURE::ColorObject color(original.Dim(_luminance));
            return NeoGamma<T_GAMMA>::Correct(color);
        }

    protected:
        bool setLuminance(typename T_EXPOSED_COLOR_OBJECT::ElementType luminance)
        {
            bool different = (_luminance != luminance);

            if (different)
            {
                _luminance = luminance;
            }
            
            return different;
        }

        typename T_EXPOSED_COLOR_OBJECT::ElementType getLuminance() const
        {
            return _luminance;
        }

        friend class NeoPixelBusLg;

    private:
        typename T_EXPOSED_COLOR_OBJECT::ElementType _luminance;
    };

public:
    NeoPixelBusLg(uint16_t countPixels, uint8_t pin) :
        _countPixels(countPixels),
        _pixels(nullptr),
        _method(pin, countPixels, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize),
        _shader()
    {
    }

    NeoPixelBusLg(uint16_t countPixels, uint8_t pin, NeoBusChannel channel) :
        _countPixels(countPixels),
        _pixels(nullptr),
        _method(pin, countPixels, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize, channel),
        _shader()
    {
    }

    NeoPixelBusLg(uint16_t countPixels, uint8_t pinClock, uint8_t pinData) :
        _countPixels(countPixels),
        _pixels(nullptr),
        _method(pinClock, pinData, countPixels, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize),
        _shader()
    {
    }

    NeoPixelBusLg(uint16_t countPixels, uint8_t pinClock, uint8_t pinData, uint8_t pinLatch, uint8_t pinOutputEnable = NOT_A_PIN) :
        _countPixels(countPixels),
        _pixels(nullptr),
        _method(pinClock, pinData, pinLatch, pinOutputEnable, countPixels, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize),
        _shader()
    {
    }

    NeoPixelBusLg(uint16_t countPixels) :
        _countPixels(countPixels),
        _pixels(nullptr),
        _method(countPixels, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize),
        _shader()
    {
    }

    NeoPixelBusLg(uint16_t countPixels, Stream* pixieStream) :
         _countPixels(countPixels),
         _pixels(nullptr),
         _method(countPixels, T_COLOR_FEATURE::PixelSize, T_COLOR_FEATURE::SettingsSize, pixieStream),
         _shader()
    {
    }

    ~NeoPixelBusLg()
    {
        delete [] _pixels;
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
        _method.template Update<T_EXPOSED_COLOR_OBJECT, T_COLOR_FEATURE, LuminanceShader>(_pixels,
                _countPixels,
                _featureSettings,
                _shader);
    }

    inline bool CanShow() const
    {
        return _method.IsReadyToUpdate();
    }

    uint16_t PixelCount() const
    {
        return _countPixels;
    }

    void SetLuminance(typename T_EXPOSED_COLOR_OBJECT::ElementType luminance)
    {
        _shader.setLuminance(luminance);
    }

    typename T_EXPOSED_COLOR_OBJECT::ElementType GetLuminance() const
    {
        return _shader.getLuminance();
    }

    void SetPixelColor(uint16_t indexPixel, const T_EXPOSED_COLOR_OBJECT& color)
    {
        if (indexPixel < _countPixels)
        {
            _pixels[indexPixel] = color;
        }
    }

    T_EXPOSED_COLOR_OBJECT GetPixelColor(uint16_t indexPixel) const
    {
        if (indexPixel >= _countPixels)
        {
            return 0;
        }
        return _pixels[indexPixel];
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

    // TODO:  Move other modification methods over
    void RotateLeft(uint16_t rotationCount)
    {
        if ((_countPixels - 1) >= rotationCount)
        {
            _rotateLeft(rotationCount, 0, _countPixels - 1);
        }
    }

protected:
    const uint16_t _countPixels; 

    T_EXPOSED_COLOR_OBJECT* _pixels;
    T_METHOD _method;
    LuminanceShader _shader;
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
        T_EXPOSED_COLOR_OBJECT* pixel = _pixels;

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
};


