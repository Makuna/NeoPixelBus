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

template <typename T_EXPOSED_COLOR_OBJECT, 
    typename T_FEATURE_COLOR_OBJECT,
    typename T_GAMMA>
class LuminanceShader
{
public:
    LuminanceShader(typename T_EXPOSED_COLOR_OBJECT::ElementType luminance = T_EXPOSED_COLOR_OBJECT::Max) :
        _luminance(luminance)
    {
    }

    T_FEATURE_COLOR_OBJECT Apply(const T_EXPOSED_COLOR_OBJECT& original) const
    {
        // dim and then return gamma adjusted
        T_FEATURE_COLOR_OBJECT color(original.Dim(_luminance));
        return NeoGamma<T_GAMMA>::Correct(color);
    }

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

private:
    typename T_EXPOSED_COLOR_OBJECT::ElementType _luminance;
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
class NeoPixelBusLg : 
    public NeoPixelBus<T_COLOR_FEATURE, 
        T_METHOD, 
        T_EXPOSED_COLOR_OBJECT, 
        LuminanceShader<T_EXPOSED_COLOR_OBJECT, typename T_COLOR_FEATURE::ColorObject, T_GAMMA>>
{
public:
    NeoPixelBusLg(const BusView& busView, uint8_t pin) :
        NeoPixelBus<T_COLOR_FEATURE,
            T_METHOD,
            T_EXPOSED_COLOR_OBJECT,
            LuminanceShader<T_EXPOSED_COLOR_OBJECT, typename T_COLOR_FEATURE::ColorObject, T_GAMMA>>(busView, pin)
    {
    }

    NeoPixelBusLg(const BusView& busView, uint8_t pin, NeoBusChannel channel) :
        NeoPixelBus<T_COLOR_FEATURE,
            T_METHOD,
            T_EXPOSED_COLOR_OBJECT,
            LuminanceShader<T_EXPOSED_COLOR_OBJECT, typename T_COLOR_FEATURE::ColorObject, T_GAMMA>>(busView, pin, channel)
    {
    }

    NeoPixelBusLg(const BusView& busView, uint8_t pinClock, uint8_t pinData) :
        NeoPixelBus<T_COLOR_FEATURE,
            T_METHOD,
            T_EXPOSED_COLOR_OBJECT,
            LuminanceShader<T_EXPOSED_COLOR_OBJECT, typename T_COLOR_FEATURE::ColorObject, T_GAMMA>>(busView, pinClock, pinData)
    {
    }

    NeoPixelBusLg(const BusView& busView, uint8_t pinClock, uint8_t pinData, uint8_t pinLatch, uint8_t pinOutputEnable = NOT_A_PIN) :
        NeoPixelBus<T_COLOR_FEATURE,
            T_METHOD,
            T_EXPOSED_COLOR_OBJECT,
            LuminanceShader<T_EXPOSED_COLOR_OBJECT, typename T_COLOR_FEATURE::ColorObject, T_GAMMA>>(busView, pinClock, pinData, pinLatch, pinOutputEnable)
    {
    }

    NeoPixelBusLg(const BusView& busView) :
        NeoPixelBus<T_COLOR_FEATURE,
            T_METHOD,
            T_EXPOSED_COLOR_OBJECT,
            LuminanceShader<T_EXPOSED_COLOR_OBJECT, typename T_COLOR_FEATURE::ColorObject, T_GAMMA>>(busView)
    {
    }

    NeoPixelBusLg(const BusView& busView, Stream* pixieStream) :
        NeoPixelBus<T_COLOR_FEATURE,
            T_METHOD,
            T_EXPOSED_COLOR_OBJECT,
            LuminanceShader<T_EXPOSED_COLOR_OBJECT, typename T_COLOR_FEATURE::ColorObject, T_GAMMA>>(busView, pixieStream)
    {
    }

    void SetLuminance(typename T_EXPOSED_COLOR_OBJECT::ElementType luminance)
    {
        this->_shader.setLuminance(luminance);
    }

    typename T_EXPOSED_COLOR_OBJECT::ElementType GetLuminance() const
    {
        return this->_shader.getLuminance();
    }
};


