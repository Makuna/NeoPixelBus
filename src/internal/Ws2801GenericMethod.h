/*-------------------------------------------------------------------------
NeoPixel library helper functions for WS2801

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

// must also check for arm due to Teensy incorrectly having ARDUINO_ARCH_AVR set
#if defined(ARDUINO_ARCH_AVR) && !defined(__arm__)
#include "TwoWireBitBangImpleAvr.h"
#else
#include "TwoWireBitBangImple.h"
#endif


template<typename T_TWOWIRE> class Ws2801MethodBase
{
public:
	Ws2801MethodBase(uint8_t pinClock, uint8_t pinData, uint16_t pixelCount, size_t elementSize) :
        _sizePixels(pixelCount * elementSize),
		_wire(pinClock, pinData)
    {
        _pixels = (uint8_t*)malloc(_sizePixels);
        memset(_pixels, 0, _sizePixels);
    }

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
	Ws2801MethodBase(uint16_t pixelCount, size_t elementSize) :
		Ws2801MethodBase(SCK, MOSI, pixelCount, elementSize)
	{
	}
#endif

    ~Ws2801MethodBase()
    {
        free(_pixels);
    }

    bool IsReadyToUpdate() const
    {
        uint32_t delta = micros() - _endTime;

        return (delta >= 500);
    }

#if defined(ARDUINO_ARCH_ESP32)
	void Initialize(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
	{
		_wire.begin(sck, miso, mosi, ss);
	}
#endif

    void Initialize()
    {
		_wire.begin();

        _endTime = micros();
    }

    void Update(bool)
    {
        while (!IsReadyToUpdate())
        {
#if !defined(ARDUINO_TEEONARDU_LEO) && !defined(ARDUINO_TEEONARDU_FLORA)
            yield(); // allows for system yield if needed
#endif
        }

		_wire.beginTransaction();
        
        // data
		_wire.transmitBytes(_pixels, _sizePixels);
        
		_wire.endTransaction();

        // save EOD time for latch on next call
        _endTime = micros();
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
    uint32_t _endTime;       // Latch timing reference
	const size_t  _sizePixels;   // Size of '_pixels' buffer below

	T_TWOWIRE _wire;
    uint8_t* _pixels;       // Holds LED color values
};

typedef Ws2801MethodBase<TwoWireBitBangImple> NeoWs2801Method;

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
#include "TwoWireSpiImple.h"
typedef Ws2801MethodBase<TwoWireSpiImple<SpiSpeed20Mhz>> NeoWs2801Spi20MhzMethod;
typedef Ws2801MethodBase<TwoWireSpiImple<SpiSpeed10Mhz>> NeoWs2801Spi10MhzMethod;
typedef Ws2801MethodBase<TwoWireSpiImple<SpiSpeed2Mhz>> NeoWs2801Spi2MhzMethod;
typedef NeoWs2801Spi10MhzMethod NeoWs2801SpiMethod;
#endif



