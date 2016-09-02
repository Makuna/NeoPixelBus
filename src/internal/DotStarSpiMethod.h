/*-------------------------------------------------------------------------
NeoPixel library helper functions for DotStars using SPI hardware (APA102).

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

class DotStarSpiMethod
{
public:
    DotStarSpiMethod(uint16_t pixelCount, size_t elementSize) :
        _sizePixels(pixelCount * elementSize)
    {
        _pixels = (uint8_t*)malloc(_sizePixels);
        memset(_pixels, 0, _sizePixels);
    }

    ~DotStarSpiMethod()
    {
        SPI.end();
        free(_pixels);
    }

    bool IsReadyToUpdate() const
    {
        return true; // dot stars don't have a required delay
    }

    void Initialize()
    {
        SPI.begin();

#if defined(ARDUINO_ARCH_ESP8266)
        SPI.setFrequency(20000000L);
#elif defined(ARDUINO_ARCH_AVR) 
        SPI.setClockDivider(SPI_CLOCK_DIV2); // 8 MHz (6 MHz on Pro Trinket 3V)
#else
        SPI.setClockDivider((F_CPU + 4000000L) / 8000000L); // 8-ish MHz on Due
#endif
        SPI.setBitOrder(MSBFIRST);
        SPI.setDataMode(SPI_MODE0);
    }

    void Update()
    {
        // start frame
        for (int startFrameByte = 0; startFrameByte < 4; startFrameByte++)
        {
            SPI.transfer(0x00);
        }
        
        // data
        uint8_t* data = _pixels;
        const uint8_t* endData = _pixels + _sizePixels;
        while (data < endData)
        {
            SPI.transfer(*data++);
        }
        
        // end frame 
        // one bit for every two pixels with no less than 1 byte
        const uint16_t countEndFrameBytes = ((_sizePixels / 4) + 15) / 16;
        for (uint16_t endFrameByte = 0; endFrameByte < countEndFrameBytes; endFrameByte++)
        {
            SPI.transfer(0xff);
        }
    }

    uint8_t* getPixels() const
    {
        return _pixels;
    };

    size_t getPixelsSize() const
    {
        return _sizePixels;
    };

private:
    const size_t   _sizePixels;   // Size of '_pixels' buffer below

    uint8_t* _pixels;       // Holds LED color values
};




