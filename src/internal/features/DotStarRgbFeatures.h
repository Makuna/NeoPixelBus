/*-------------------------------------------------------------------------
DotStarRbgFeature provides feature classes to describe color order and
color depth for NeoPixelBus template class when used with DotStars

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

// Byte features

class DotStarRgbFeature :
    public DotStarX4ByteFeature<ColorIndexR, ColorIndexG, ColorIndexB>,
    public NeoElementsNoSettings
{
};

class DotStarRbgFeature :
    public DotStarX4ByteFeature<ColorIndexR, ColorIndexB, ColorIndexG>,
    public NeoElementsNoSettings
{
};


class DotStarGbrFeature :
    public DotStarX4ByteFeature<ColorIndexG, ColorIndexB, ColorIndexR>,
    public NeoElementsNoSettings
{
};

class DotStarGrbFeature :
    public DotStarX4ByteFeature<ColorIndexG, ColorIndexR, ColorIndexB>,
    public NeoElementsNoSettings
{
};


class DotStarBrgFeature :
    public DotStarX4ByteFeature<ColorIndexB, ColorIndexR, ColorIndexG>,
    public NeoElementsNoSettings
{
};

class DotStarBgrFeature :
    public DotStarX4ByteFeature<ColorIndexB, ColorIndexG, ColorIndexR>,
    public NeoElementsNoSettings
{
};

// Word features

class DotStarBgr48Feature :
    public DotStarX4WordFeature<ColorIndexB, ColorIndexG, ColorIndexR>,
    public NeoElementsNoSettings
{
};

typedef DotStarBgr48Feature Hd108BgrFeature;
