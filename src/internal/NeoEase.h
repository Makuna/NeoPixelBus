/*-------------------------------------------------------------------------
NeoEase provides animation curve equations for animation support.

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

#ifdef ARDUINO_ARCH_AVR

typedef float(*AnimEaseFunction)(float linear);

#else

#undef max
#undef min
#include <functional>
typedef std::function<float(float linear)> AnimEaseFunction;

#endif

class NeoEase
{
public:
    static float QuadraticIn(float linear)
    {
        return linear * linear;
    }

    static float QuadraticOut(float linear)
    {
        return (-linear * (linear - 2.0f));
    }

    static float QuadraticInOut(float linear)
    {
        linear *= 2.0f;
        if (linear < 1.0f)
        {
            return (0.5f * linear * linear);
        }
        else
        {
            linear -= 1.0f;
            return (-0.5f * (linear * (linear - 2.0f) - 1.0f));
        }
    }

    static float CubicIn(float linear)
    {
        return (linear * linear * linear);
    }

    static float CubicOut(float linear)
    {
        linear -= 1.0f;
        return (linear * linear * linear + 1);
    }

    static float CubicInOut(float linear)
    {
        linear *= 2.0f;
        if (linear < 1.0f)
        {
            return (0.5f * linear * linear * linear);
        }
        else
        {
            linear -= 2.0f;
            return (0.5f * (linear * linear * linear + 2.0f));
        }
    }

    static float QuarticIn(float linear)
    {
        return (linear * linear * linear * linear);
    }

    static float QuarticOut(float linear)
    {
        linear -= 1.0f;
        return -(linear * linear * linear * linear - 1);
    }

    static float QuarticInOut(float linear)
    {
        linear *= 2.0f;
        if (linear < 1.0f)
        {
            return (0.5f * linear * linear * linear * linear);
        }
        else
        {
            linear -= 2.0f;
            return (-0.5f * (linear * linear * linear * linear - 2.0f));
        }
    }

    static float QuinticIn(float linear)
    {
        return (linear * linear * linear * linear * linear);
    }

    static float QuinticOut(float linear)
    {
        linear -= 1.0f;
        return (linear * linear * linear * linear * linear + 1.0f);
    }

    static float QuinticInOut(float linear)
    {
        linear *= 2.0f;
        if (linear < 1.0f)
        {
            return (0.5f * linear * linear * linear * linear * linear);
        }
        else
        {
            linear -= 2.0f;
            return (0.5f * (linear * linear * linear * linear * linear + 2.0f));
        }
    }

    static float SinusoidalIn(float linear)
    {
        return (-cos(linear * HALF_PI) + 1.0f);
    }

    static float SinusoidalOut(float linear)
    {
        return (sin(linear * HALF_PI));
    }

    static float SinusoidalInOut(float linear)
    {
        return -0.5 * (cos(PI * linear) - 1.0f);
    }

    static float ExponentialIn(float linear)
    {
        return (pow(2, 10.0f * (linear - 1.0f)));
    }

    static float ExponentialOut(float linear)
    {
        return (-pow(2, -10.0f * linear) + 1.0f);
    }

    static float ExponentialInOut(float linear)
    {
        linear *= 2.0f;
        if (linear < 1.0f)
        {
            return (0.5f * pow(2, 10.0f * (linear - 1.0f)));
        }
        else
        {
            linear -= 1.0f;
            return (0.5f * (-pow(2, -10.0f * linear) + 2.0f));
        }
    }

    static float CircularIn(float linear)
    {
        if (linear == 1.0f)
        {
            return 1.0f;
        }
        else
        {
            return (-(sqrt(1.0f - linear * linear) - 1.0f));
        }
    }

    static float CircularOut(float linear)
    {
        linear -= 1.0f;
        return (sqrt(1.0f - linear * linear));
    }

    static float CircularInOut(float linear)
    {
        linear *= 2.0f;
        if (linear < 1.0f)
        {
            return (-0.5f * (sqrt(1.0f - linear * linear) - 1));
        }
        else
        {
            linear -= 2.0f;
            return (0.5f * (sqrt(1.0f - linear * linear) + 1.0f));
        }
    }
};