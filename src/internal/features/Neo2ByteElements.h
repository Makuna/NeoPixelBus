/*-------------------------------------------------------------------------
Neo2ByteElements provides feature base classes to describe color elements
for NeoPixelBus Color Feature template classes

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
 
class Neo2ByteElementsNoSettings : public NeoByteElements<2, RgbColor>
{
public:
    typedef NeoNoSettings SettingsObject;
    static const size_t SettingsSize = 0;

    static void applySettings([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData, [[maybe_unused]] const SettingsObject& settings)
    {
    }

    static uint8_t* pixels([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        return pData;
    }

    static const uint8_t* pixels([[maybe_unused]] const uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        return pData;
    }

protected:
    static void encodePixel(uint8_t c1, uint8_t c2, uint8_t c3, uint16_t* color555)
    {
        *color555 = (0x8000 |
            ((c1 & 0xf8) << 7) |
            ((c2 & 0xf8) << 2) |
            ((c3 & 0xf8) >> 3));
    }

    static void decodePixel(uint16_t color555, uint8_t* c1, uint8_t* c2, uint8_t* c3)
    {
        *c1 = (color555 >> 7) & 0xf8;
        *c2 = (color555 >> 2) & 0xf8;
        *c3 = (color555 << 3) & 0xf8;
    }
};