/*-------------------------------------------------------------------------
DotStarLrgbFeature provides feature classes to describe color order and
color depth for NeoPixelBus template class when used with DotStars

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

// Byte features
class DotStarLrgbFeature : 
    public DotStarL4ByteFeature<ColorIndexR, ColorIndexG, ColorIndexB>,
    public NeoElementsNoSettings
{
};

class DotStarLrbgFeature :
    public DotStarL4ByteFeature<ColorIndexR, ColorIndexB, ColorIndexG>,
    public NeoElementsNoSettings
{
};


class DotStarLgrbFeature :
    public DotStarL4ByteFeature<ColorIndexG, ColorIndexR, ColorIndexB>,
    public NeoElementsNoSettings
{
};

class DotStarLgbrFeature :
    public DotStarL4ByteFeature<ColorIndexG, ColorIndexB, ColorIndexR>,
    public NeoElementsNoSettings
{
};


class DotStarLbrgFeature :
    public DotStarL4ByteFeature<ColorIndexB, ColorIndexR, ColorIndexG>,
    public NeoElementsNoSettings
{
};

class DotStarLbgrFeature :
    public DotStarL4ByteFeature<ColorIndexB, ColorIndexG, ColorIndexR>,
    public NeoElementsNoSettings
{
};

// Word features

class DotStarLbgr64Feature :
    public DotStarL4WordFeature<ColorIndexB, ColorIndexG, ColorIndexR>,
    public NeoElementsNoSettings
{
};

class DotStarLrgb64Feature :
    public DotStarL4WordFeature<ColorIndexR, ColorIndexG, ColorIndexB>,
    public NeoElementsNoSettings
{
};

typedef DotStarLbgr64Feature Hd108LbgrFeature;
typedef DotStarLrgb64Feature Hd108LrgbFeature;