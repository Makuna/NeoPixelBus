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

class Esp32VspiBus
{
public:
    static const uint8_t spi_bus = VSPI;
};

class Esp32HspiBus
{
public:
    static const uint8_t spi_bus = HSPI;
};

template<typename T_SPISPEED, typename T_SPIBUS> class TwoWireEsp32DmaSpiImple
{
public:
    // "pin" is used to send "spi_bus", this must be specified in the constructor
    TwoWireEsp32DmaSpiImple(uint8_t, uint8_t)
    {
        spiClass = new SPIClass(T_SPIBUS::spi_bus);
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
