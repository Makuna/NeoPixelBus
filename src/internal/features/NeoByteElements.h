/*-------------------------------------------------------------------------
NeoByteElements provides feature base classes to describe color elements
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

// NeoElementsBase contains common methods used by features to map and
// copy pixel memory data in native stream format
// 
// V_PIXEL_SIZE - the size in bytes of a pixel in the data stream
// T_COLOR_OBJECT - the primary color object used to represent a pixel
template<size_t V_PIXEL_SIZE, typename T_COLOR_OBJECT>
class NeoElementsBase
{
public:
    static const size_t PixelSize = V_PIXEL_SIZE;
    typedef T_COLOR_OBJECT ColorObject;

};



