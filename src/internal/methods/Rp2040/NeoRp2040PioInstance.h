/*-------------------------------------------------------------------------
NeoPixel library helper functions for RP2040.

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

#ifdef ARDUINO_ARCH_RP2040

// PIO Instances
// --------------------------------------------------------
class NeoRp2040PioInstance0
{
public:
    NeoRp2040PioInstance0() :
        Instance(pio0)
    {};

    const PIO Instance;
};

class NeoRp2040PioInstance1
{
public:
    NeoRp2040PioInstance1() :
        Instance(pio1)
    {};

    const PIO Instance;
};

// dynamic channel support
class NeoRp2040PioInstanceN
{
public:
    NeoRp2040PioInstanceN(NeoBusChannel channel) :
        Instance(channel == NeoBusChannel_0 ? pio0 : pio1)
    {
    }
    NeoRp2040PioInstanceN() = delete; // no default constructor

    const PIO Instance;
};

#endif
