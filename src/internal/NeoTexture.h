/*-------------------------------------------------------------------------
Teture provides a texture object that can contain a matrix of Color
values 

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

NOTES:
Where does the data come from?
    * Loaded by external routine

  

template<typename T_COLOR> class Texture
{
public:
    Texture(uint16_t width, uint16_t height, T_COLOR* pixels) :
        _width(width),
        _height(height),
        _pixels(pixels)
    {
    };

    const T_COLOR* Blend(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, float progress)
    {
        x1 = constrain(x1, 0, _width - 1);
        y1 = constrain(y1, 0, _height - 1);
        x2 = constrain(x2, 0, _width - 1);
        y2 = constrain(y2, 0, _height - 1);

        // calc cordinate of the four nearby pixels
        // first lerp our cordinates
        float xf = (static_cast<float>(x2) - x1) * progress + x1;
        float yf = (static_cast<float>(y2) - y1) * progress + y1;

        uint16_t x = static_cast<uint16_t>(xf); // truncates to lower uint
        uint16_t y = static_cast<uint16_t>(yf); // truncates to lower uint
        uint16_t xn = x + 1;
        uint16_t yn = y + 1;
        
        T_COLOR c00 = PixelAt(x, y);
        T_COLOR c01 = PixelAt(x, yn);
        T_COLOR c10 = PixelAt(xn, y);
        T_COLOR c11 = PixelAt(xn, yn);

        // unitize progress cordinate to these pixels
        xf -= x;
        yf -= y;

        return T_COLOR::BilinearBlend(c00, c01, c10, c11, xf, yf);
    };

    const T_COLOR& PixelAt(uint16_t x, uint16_t y)
    {
        uint16_t xSafe = constrain(x, 0, _width - 1);
        uint16_t ySafe = constrain(y, 0, _height - 1);
        return pixelAt(xSafe, ySafe);
    }

private:
    const uint16_t _width;
    const uint16_t _height;
    const T_COLOR* _pixels;

    const T_COLOR& pixelAt(uint16_t x, uint16_t y)
    {
        return *(_pixels + x + y * _width);
    }
};

typedef Texture<RgbColor> RgbTexture;
typedef Texture<RgbwColor> RgbwTexture;
typedef Texture<HtmlColor> HtmlTexture;

