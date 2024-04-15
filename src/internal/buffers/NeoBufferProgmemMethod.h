/*-------------------------------------------------------------------------
NeoBufferProgmemMethod

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

template <typename T_COLOR_OBJECT> 
class NeoBufferProgmemMethod
{
public:
    NeoBufferProgmemMethod(uint16_t width, uint16_t height, PGM_VOID_P pixels) :
        _width(width),
        _height(height),
        _pixels(pixels)
    {
    }

    operator NeoBufferContext<T_COLOR_OBJECT>()
    {
        return NeoBufferContext<T_COLOR_OBJECT>(Pixels(), PixelCount());
    }

    const uint8_t* Pixels() const
    {
        return reinterpret_cast<const uint8_t*>(_pixels);
    };

    uint16_t PixelCount() const
    {
        return _width * _height;
    };

    uint16_t Width() const
    {
        return _width;
    };

    uint16_t Height() const
    {
        return _height;
    };

    void SetPixelColor(uint16_t indexPixel, T_COLOR_OBJECT color)
    {
        // PROGMEM is read only, this will do nothing
    };

    void SetPixelColor(uint16_t x, uint16_t y, T_COLOR_OBJECT color)
    {
        // PROGMEM is read only, this will do nothing
    };

    T_COLOR_OBJECT GetPixelColor(uint16_t indexPixel) const
    {
        if (indexPixel >= PixelCount())
        {
            // Pixel # is out of bounds, this will get converted to a 
            // color object type initialized to 0 (black)
            return 0;
        }

        const uint8_t* pixelsSrc = static_cast<const uint8_t*>(_pixels);

        return T_COLOR_OBJECT::PgmRead(pixelsSrc + T_COLOR_OBJECT::Size * indexPixel);
    };


    T_COLOR_OBJECT GetPixelColor(int16_t x, int16_t y) const
    {
        if (x < 0 || x >= _width || y < 0 || y >= _height)
        {
            // Pixel # is out of bounds, this will get converted to a 
            // color object type initialized to 0 (black)
            return 0;
        }

        uint16_t indexPixel = x + y * _width;
        return GetPixelColor(indexPixel);
    };

    void ClearTo(T_COLOR_OBJECT color)
    {
        // PROGMEM is read only, this will do nothing
    };

    typedef T_COLOR_OBJECT ColorObject;

private:
    const uint16_t _width;
    const uint16_t _height;
    PGM_VOID_P _pixels;
};

template <typename T_COLOR_OBJECT>
class NeoDib_P : public NeoBuffer<NeoBufferProgmemMethod<T_COLOR_OBJECT>>
{
public:
    NeoDib_P(uint16_t width,
        uint16_t height,
        PGM_VOID_P pixels) :
        NeoBuffer<NeoBufferProgmemMethod<T_COLOR_OBJECT>>(width, height, pixels)
    {

    }
};