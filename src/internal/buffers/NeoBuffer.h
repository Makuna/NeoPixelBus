/*-------------------------------------------------------------------------
NeoBuffer

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

// T_BUFFER_METHOD - one of
//      NeoBufferMethod
//      NeoBufferProgmemMethod
//
template <typename T_BUFFER_METHOD> 
class NeoBuffer
{
public:
    NeoBuffer(uint16_t width,
        uint16_t height = 1,
        PGM_VOID_P pixels = nullptr) :
        _method(width, height, pixels)
    {
    }

    ~NeoBuffer()
    {
    }

    operator NeoBufferContext<typename T_BUFFER_METHOD::ColorObject>()
    {
        return _method;
    }

    uint16_t PixelCount() const
    {
        return _method.PixelCount();
    };

    uint16_t Width() const
    {
        return _method.Width();
    };

    uint16_t Height() const
    {
        return _method.Height();
    };

    void SetPixelColor(
        int16_t x,
        int16_t y,
        typename T_BUFFER_METHOD::ColorObject color)
    {
        _method.SetPixelColor(x, y, color);
    };

    void SetPixelColor(
        uint16_t index,
        typename T_BUFFER_METHOD::ColorObject color)
    {
        _method.SetPixelColor(index, color);
    };

    typename T_BUFFER_METHOD::ColorObject GetPixelColor(
        int16_t x,
        int16_t y) const
    {
        return _method.GetPixelColor(x, y);
    };

    typename T_BUFFER_METHOD::ColorObject GetPixelColor(
        uint16_t index) const
    {
        return _method.GetPixelColor(index);
    };

    void ClearTo(typename T_BUFFER_METHOD::ColorObject color)
    {
        _method.ClearTo(color);
    };

    void Blt(NeoBufferContext<typename T_BUFFER_METHOD::ColorObject> destBuffer,
        uint16_t destPixelIndex)
    {
        uint16_t destPixelCount = destBuffer.PixelCount();
        // validate destPixelIndex
        if (destPixelIndex >= destPixelCount)
        {
            return;
        }

        // calc how many we can copy
        uint16_t copyCount = destPixelCount - destPixelIndex;
        uint16_t srcPixelCount = PixelCount();
        if (copyCount > srcPixelCount)
        {
            copyCount = srcPixelCount;
        }

        for (uint16_t index = 0; index < copyCount; index++)
        {
            typename T_BUFFER_METHOD::ColorObject color = _method.GetPixelColor(index);
            destBuffer.Pixels[destPixelIndex + index] = color;
        }
    }

    void Blt(NeoBufferContext<typename T_BUFFER_METHOD::ColorObject> destBuffer,
        int16_t xDest,
        int16_t yDest,
        int16_t xSrc,
        int16_t ySrc,
        int16_t wSrc,
        int16_t hSrc,
        LayoutMapCallback layoutMap)
    {
        uint16_t destPixelCount = destBuffer.PixelCount();

        for (int16_t y = 0; y < hSrc; y++)
        {
            for (int16_t x = 0; x < wSrc; x++)
            {
                uint16_t destPixelIndex = layoutMap(xDest + x, yDest + y);
 
                if (destPixelIndex < destPixelCount)
                {
                    typename T_BUFFER_METHOD::ColorObject color = _method.GetPixelColor(xSrc + x, ySrc + y);
                    destBuffer.Pixels[destPixelIndex] = color;
                }
            }
        }
    }

    void Blt(NeoBufferContext<typename T_BUFFER_METHOD::ColorObject> destBuffer,
        int16_t xDest,
        int16_t yDest,
        LayoutMapCallback layoutMap)
    {
        Blt(destBuffer, xDest, yDest, 0, 0, Width(), Height(), layoutMap);
    }

    template <typename T_SHADER> 
    void Render(NeoBufferContext<typename T_BUFFER_METHOD::ColorObject> destBuffer, 
        T_SHADER& shader, 
        uint16_t startIndex = 0)
    {
        uint16_t countPixels = destBuffer.PixelCount;

        if (countPixels > _method.PixelCount())
        {
            countPixels = _method.PixelCount();
        }

        for (uint16_t indexPixel = 0; indexPixel < countPixels; indexPixel++)
        {
            typename T_BUFFER_METHOD::ColorObject color = _method.GetPixelColor(indexPixel);
            destBuffer.Pixels[indexPixel + startIndex] = shader.Apply(color, destBuffer.Pixels[indexPixel + startIndex]);
        }
    }

    template <typename T_SHADER>
    void Render(NeoBufferContext<typename T_BUFFER_METHOD::ColorObject> destBuffer, 
        const NeoBuffer<T_BUFFER_METHOD>& srcBufferB,
        T_SHADER& shader, 
        uint16_t startIndex = 0)
    {
        uint16_t countPixels = destBuffer.PixelCount;

        if (countPixels > _method.PixelCount())
        {
            countPixels = _method.PixelCount();
        }

        for (uint16_t indexPixel = 0; indexPixel < countPixels; indexPixel++)
        {
            typename T_BUFFER_METHOD::ColorObject color = _method.GetPixelColor(indexPixel);
            typename T_BUFFER_METHOD::ColorObject colorB = srcBufferB.GetPixelColor(indexPixel);
            destBuffer.Pixels[indexPixel + startIndex] = shader.Apply(color, colorB);
        }
    }

    uint16_t PixelIndex(
        int16_t x,
        int16_t y) const
    {
        uint16_t result = PixelIndex_OutOfBounds;

        if (x >= 0 &&
            static_cast<uint16_t>(x) < Width() &&
            y >= 0 &&
            static_cast<uint16_t>(y) < Height())
        {
            result = x + y * Width();
        }
        return result;
    }

protected:
    T_BUFFER_METHOD _method;
};

