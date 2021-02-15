/*-------------------------------------------------------------------------
NeoPixel library helper functions for DotStars using ESP32's DMA-capable SPI (APA102/LPD8806).

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

template<typename T_SPISPEED> class TwoWireEsp32DmaSpiImple
{
public:
    // "pin" is used to send "spi_bus", this must be specified in the constructor
    TwoWireEsp32DmaSpiImple(uint8_t spi_bus, uint8_t unused)
    {
        // default to VSPI if an invalid bus number is provided
        if(spi_bus != VSPI && spi_bus != HSPI)
            spi_bus = VSPI;

        busnum = spi_bus;
        spiClass = new SPIClass(spi_bus);
    }

#if defined(ARDUINO_ARCH_ESP32)
    // for cases where hardware SPI can have pins changed
    void begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
    {
        spiClass->begin(sck, miso, mosi, ss);
    }
#endif

    void begin()
    {
        spiClass->begin();
    }

    void beginTransaction()
    {
        spiClass->beginTransaction(SPISettings(T_SPISPEED::Clock, MSBFIRST, SPI_MODE0));
    }

    void endTransaction()
    {
        spiClass->endTransaction();
    }

    void transmitByte(uint8_t data)
    {
        spiClass->transfer(data);
    }

    void transmitBytes(const uint8_t* data, size_t dataSize)
    {
        // ESPs have a method to write without inplace overwriting the send buffer
        // since we don't care what gets received, use it for performance
        // FIX: but for what ever reason on Esp32, its not const
        spiClass->writeBytes(const_cast<uint8_t*>(data), dataSize);
    }

private:
    SPIClass * spiClass = NULL;
    uint8_t busnum;
};