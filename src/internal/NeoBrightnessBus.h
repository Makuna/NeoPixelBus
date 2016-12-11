/*-------------------------------------------------------------------------
NeoPixel library helper template class that provides overall brightness control

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

template<typename T_COLOR_FEATURE, typename T_METHOD> class NeoBrightnessBus : 
    public NeoPixelBus<T_COLOR_FEATURE, T_METHOD>
{
public:
    void SetBrightness(uint8_t brightness)
    {
        // Stored brightness value is different than what's passed.
        // This simplifies the actual scaling math later, allowing a fast
        // 8x8-bit multiply and taking the MSB.  'brightness' is a uint8_t,
        // adding 1 here may (intentionally) roll over...so 0 = max brightness
        // (color values are interpreted literally; no scaling), 1 = min
        // brightness (off), 255 = just below max brightness.
        uint8_t newBrightness = brightness + 1;
        // Only update if there is a change
        if (newBrightness != _brightness) 
        { 
            // calculate a scale to modify from old brightness to new brightness
            //
            uint8_t oldBrightness = _brightness - 1; // De-wrap old brightness value
            uint16_t scale;

            if (oldBrightness == 0)
            {
                scale = 0; // Avoid divide by 0
            }
            else if (brightness == 255)
            {
                scale = 65535 / oldBrightness;
            }
            else
            {
                scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
            }

            // re-scale existing data in RAM
            //
            uint8_t* ptr = pixels;
            uint8_t* ptrEnd = pixels + PixelsSize();
            while (ptr != ptrEnd)
            {
                uint16_t value = *ptr;
                *ptr++ = (value * scale) >> 8;
            }

            _brightness = newBrightness;
            Dirty();
        }
    }

    uint8_t GetBrightness() const
    {
        return _brightness;
    }

    void SetPixelColor(uint16_t indexPixel, typename T_COLOR_FEATURE::ColorObject color)
    {
        if (_brightness)
        {
            uint8_t* ptr = (uint8_t*)&color;
            uint8_t* ptrEnd = ptr + T_COLOR_FEATURE::PixelSize;

            while (ptr != ptrEnd)
            {
                uint16_t value = *ptr;
                *ptr++ = (value * _brightness) >> 8;
            }
        }
        NeoPixelBus<T_COLOR_FEATURE, T_METHOD>::SetPixelColor(indexPixel, color);
    }

    typename T_COLOR_FEATURE::ColorObject GetPixelColor(uint16_t indexPixel) const
    {
        T_COLOR_FEATURE::ColorObject color = NeoPixelBus<T_COLOR_FEATURE, T_METHOD>::GetPixelColor(indexPixel);

        if (_brightness)
        {
            uint8_t* ptr = (uint8_t*)&color;
            uint8_t* ptrEnd = ptr + T_COLOR_FEATURE::PixelSize;

            while (ptr != ptrEnd)
            {
                uint16_t value = *ptr;
                *ptr++ = (value << 8) / _brightness);
            }
        }
        return color;
    }

protected:
    uint8_t _brightness;
};


