/*-------------------------------------------------------------------------
NeoBufferMethod

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
class NeoBufferMethod
{
public:
    NeoBufferMethod(uint16_t width, uint16_t height, PGM_VOID_P pixels = nullptr) :
        _width(width),
        _height(height)
    {
        _pixels = new T_COLOR_OBJECT[PixelCount()];

        if (pixels)
        {
            const uint8_t* pixelsSrc = static_cast<const uint8_t*>(pixels);

            // copy from progmem to initialize
            for (size_t index = 0; index < PixelCount(); index++)
            {
                _pixels[index] = T_COLOR_OBJECT::PgmRead(pixelsSrc + T_COLOR_OBJECT::Size * index);
            }
        }
    }

    ~NeoBufferMethod()
    {
        delete [] _pixels;
        _pixels = nullptr;
    }

    NeoBufferMethod& operator=(const NeoBufferMethod& other)
    {
        uint16_t pixelCount = PixelCount();
        uint16_t pixelCountOther = other.PixelCount();

        if (pixelCount > pixelCountOther)
        {
            pixelCount = pixelCountOther;
        }

        for (size_t index = 0; index < pixelCount; index++)
        {
            _pixels[index] = other._pixels[index];
        }

        return *this;
    }

    operator NeoBufferContext<T_COLOR_OBJECT>()
    {
        return NeoBufferContext<T_COLOR_OBJECT>(Pixels(), PixelCount());
    }

    uint8_t* Pixels() const
    {
        return _pixels;
    };

    size_t PixelsSize() const
    {
        return PixelSize() * PixelCount();
    };

    size_t PixelSize() const
    {
        return T_COLOR_OBJECT::Size;
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
        if (indexPixel < PixelCount())
        {
            _pixels[indexPixel] = color;
        }
    };

    void SetPixelColor(int16_t x, int16_t y, T_COLOR_OBJECT color)
    {
        if (x < 0 || static_cast<uint16_t>(x) >= _width || 
            y < 0 || static_cast<uint16_t>(y) >= _height)
        {
            return;
        }

        uint16_t indexPixel = x + y * _width;
        _pixels[indexPixel] = color;
    };

    T_COLOR_OBJECT GetPixelColor(uint16_t indexPixel) const
    {
        if (indexPixel >= PixelCount())
        {
            // Pixel # is out of bounds, this will get converted to a 
            // color object type initialized to 0 (black)
            return 0;
        }

        return _pixels[indexPixel];
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
        return _pixels[indexPixel];
    };

    void ClearTo(T_COLOR_OBJECT color)
    {
        T_COLOR_OBJECT* pixel = _pixels;
        T_COLOR_OBJECT* pixelEnd = pixel + PixelCount();

        while (pixel < pixelEnd)
        {
            *pixel++ = color;
        }
    };

    typedef T_COLOR_OBJECT ColorObject;

private:
    const uint16_t _width; 
    const uint16_t _height;
    T_COLOR_OBJECT* _pixels;
};

template <typename T_COLOR_OBJECT>
class NeoDib : public NeoBuffer<NeoBufferMethod<T_COLOR_OBJECT>>
{
public:
    NeoDib(uint16_t width,
        uint16_t height = 1,
        PGM_VOID_P pixels = nullptr) :
        NeoBuffer<NeoBufferMethod<T_COLOR_OBJECT>>(width, height, pixels)
    {

    }
};

