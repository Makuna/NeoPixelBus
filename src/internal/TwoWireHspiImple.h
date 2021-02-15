/*-------------------------------------------------------------------------
NeoPixel library helper functions for DotStars using ESP32's alternate SPI (HSPI) (APA102/LPD8806).

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

#include <SPI.h>

template<typename T_SPISPEED> class TwoWireHspiImple
{
public:
    TwoWireHspiImple(uint8_t, uint8_t) // clock and data pins ignored for hardware SPI
    {
        SPI_H = new SPIClass(HSPI);
    }

    ~TwoWireHspiImple()
    {
        SPI_H->end();
        delete SPI_H;
    }

#if defined(ARDUINO_ARCH_ESP32)
    // for cases where hardware SPI can have pins changed
    void begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
    {
        SPI_H->begin(sck, miso, mosi, ss);
    }
#endif

    void begin()
    {
        SPI_H->begin();
    }

    void beginTransaction()
    {
        SPI_H->beginTransaction(SPISettings(T_SPISPEED::Clock, MSBFIRST, SPI_MODE0));
    }

    void endTransaction()
    {
        SPI_H->endTransaction();
    }

    void transmitByte(uint8_t data)
    {
        SPI_H->transfer(data);
    }

    void transmitBytes(const uint8_t* data, size_t dataSize)
    {
        // ESPs have a method to write without inplace overwriting the send buffer
        // since we don't care what gets received, use it for performance
        // FIX: but for what ever reason on Esp32, its not const
        SPI_H->writeBytes(const_cast<uint8_t*>(data), dataSize);
    }

private:
    SPIClass * SPI_H = NULL;
};