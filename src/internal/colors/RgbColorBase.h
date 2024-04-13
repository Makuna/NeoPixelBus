/*-------------------------------------------------------------------------
RgbColorBase provides a RGB color object common support

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

struct HslColor;
struct HsbColor;
struct HtmlColor;
struct Rgb16Color;

struct RgbColorBase
{

protected:
    static float _CalcColor(float p, float q, float t);

    static void _HslToRgb(const HslColor& color, float* r, float* g, float* b);

    static void _HsbToRgb(const HsbColor& color, float* r, float* g, float* b);

    template <typename T_COLOR, typename T_RESULT> static T_RESULT _Compare(
        const T_COLOR& left,
        const T_COLOR& right,
        T_RESULT epsilon)
    {
        T_RESULT result = 0;
        T_RESULT resultAbs = 0;

        for (size_t elem = 0; elem < T_COLOR::Count; elem++)
        {
            T_RESULT delta = static_cast<T_RESULT>(left[elem]) - right[elem];
            T_RESULT deltaAbs = abs(delta);

            if (deltaAbs > resultAbs)
            {
                resultAbs = deltaAbs;
                result = delta;
            }
        }

        if (resultAbs > epsilon)
        {
            return result;
        }
        return 0;
    }

    template <typename T_COLOR> static T_COLOR 
        _PgmReadByBytes(PGM_VOID_P pPixelSrc)
    {
        T_COLOR result;
        const uint8_t* pSrc = reinterpret_cast<const uint8_t*>(pPixelSrc);
        const uint8_t* pEnd = pSrc + T_COLOR::Count;
        uint8_t index = 0;

        while (pSrc < pEnd)
        {
            result[index++] = pgm_read_byte(pSrc++);
        }

        return result;
    }

    template <typename T_COLOR> static T_COLOR
        _PgmReadByWords(PGM_VOID_P pPixelSrc)
    {
        T_COLOR result;
        const uint16_t* pSrc = reinterpret_cast<const uint16_t*>(pPixelSrc);
        const uint16_t* pEnd = pSrc + T_COLOR::Count;
        uint8_t index = 0;

        while (pSrc < pEnd)
        {
            result[index++] = pgm_read_word(pSrc++);
        }
        return result;
    }
};