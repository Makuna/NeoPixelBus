/*-------------------------------------------------------------------------
NeoPixel library helper functions for SM16716 using general Pins 

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


template<typename T_TWOWIRE> class SM16716BitBangMethodBase
{
public:
	SM16716BitBangMethodBase(uint8_t pinClock, uint8_t pinData, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
		_wire(pinClock, pinData)
    {
        _data = static_cast<uint8_t*>(malloc(_sizeData));
        memset(_data, 0, _sizeData);
    }

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
	SM16716BitBangMethodBase(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
		SM16716BitBangMethodBase(SCK, MOSI, pixelCount, elementSize, settingsSize)
	{
	}
#endif

    ~SM16716BitBangMethodBase()
    {
        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        return true; // dot stars don't have a required delay
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
    }

    void Update(bool)
    {
		const uint8_t startFrame[6] = { 0x00,0x00,0x00,0x00,0x00,0x00};
		
		_wire.beginTransaction();
        
        // start frame
		_wire.transmitBytes(startFrame, sizeof(startFrame)); //48 0s
		_wire.transmitBit(0);
        _wire.transmitBit(0);//two extra 0s to make the 50 0 header
        _wire.transmitBit(1); //one to start the led frame

        for (uint16_t i = 0; i < (_sizeData/3); i++){ 
            _wire.transmitByte(_data[i]);
            _wire.transmitByte(_data[i+1]);
            _wire.transmitByte(_data[i+2]);
            _wire.transmitBit(1); //show the color and start the next frame
        }

		_wire.endTransaction();
    }

    uint8_t* getData() const
    {
        return _data;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    };

private:
	const size_t   _sizeData;   // Size of '_data' buffer below

	T_TWOWIRE _wire;
    uint8_t* _data;       // Holds LED color values
};

typedef SM16716BitBangMethodBase<TwoWireBitBangImple> SM16716BitBangMethod;