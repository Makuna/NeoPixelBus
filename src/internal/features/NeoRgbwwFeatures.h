/*-------------------------------------------------------------------------
NeoRgbwwFeature provides feature classes to describe color order and
color depth for NeoPixelBus template class

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by donating (see https://github.com/Makuna/NeoPixelBus)

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

class NeoGrbwwFeature : 
    public Neo5ByteFeature<ColorIndexG, ColorIndexR, ColorIndexB, ColorIndexW1, ColorIndexW2>,
    public NeoElementsNoSettings
{
};

class NeoGbrwwFeature :
    public Neo5ByteFeature<ColorIndexG, ColorIndexB, ColorIndexR, ColorIndexW1, ColorIndexW2>,
    public NeoElementsNoSettings
{
};

class NeoRgbwwFeature :
    public Neo5ByteFeature<ColorIndexR, ColorIndexG, ColorIndexB, ColorIndexW1, ColorIndexW2>,
    public NeoElementsNoSettings
{
};

class NeoRbgwwFeature :
    public Neo5ByteFeature<ColorIndexR, ColorIndexB, ColorIndexG, ColorIndexW1, ColorIndexW2>,
    public NeoElementsNoSettings
{
};

class NeoGrbwcFeature :
    public Neo5ByteFeature<ColorIndexG, ColorIndexR, ColorIndexB, ColorIndexWW, ColorIndexCW>,
    public NeoElementsNoSettings
{
};

class NeoRgbwcFeature :
    public Neo5ByteFeature<ColorIndexR, ColorIndexG, ColorIndexB, ColorIndexWW, ColorIndexCW>,
    public NeoElementsNoSettings
{
};
