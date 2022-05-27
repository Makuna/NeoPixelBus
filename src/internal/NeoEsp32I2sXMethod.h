#pragma once

/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp32.

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

// ESP32C3 I2S is not supported yet due to significant changes to interface
#if defined(ARDUINO_ARCH_ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C3)

enum Esp32i2sXState
{
	Esp32i2sXState_Uninitialized,
	Esp32i2sXState_Initialized,
};

static Esp32i2sXState s_state;
static uint8_t s_UpdateCount; // count of busses that have called update
const static uint8_t s_BusCount; // number of actual busses
const statuc uint8_t s_BusMaxCount; // 8, 16, 24

void Update(bool)
{
    // wait for not actively sending data
    while (!IsReadyToUpdate())
    {
        yield();
    }
    
    FillBuffers();

    s_UpdateCount++;
    if (s_UpdateCount == s_BusCount)
    {
        s_UpdateCount = 0;
        i2sWrite(_bus.I2sBusNumber, _i2sBuffer, _i2sBufferSize, false, false);
    }
}

#endif