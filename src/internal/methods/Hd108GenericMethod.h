/*-------------------------------------------------------------------------
NeoPixel library helper functions for HD108 using general Pins.

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

// must also check for arm due to Teensy incorrectly having ARDUINO_ARCH_AVR set
#if defined(ARDUINO_ARCH_AVR) && !defined(__arm__)
#include "TwoWireBitBangImpleAvr.h"
#else
#include "TwoWireBitBangImple.h"
#endif


template<typename T_TWOWIRE> class Hd108MethodBase
{
public:
    typedef typename T_TWOWIRE::SettingsObject SettingsObject;

    Hd108MethodBase(uint8_t pinClock, uint8_t pinData, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _wire(pinClock, pinData)
    {
        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()
    }

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
    Hd108MethodBase(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        Hd108MethodBase(SCK, MOSI, pixelCount, elementSize, settingsSize)
    {
    }
#endif

    ~Hd108MethodBase()
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
        const uint8_t startFrame[16] = { 0x00 };
        const uint8_t endFrame[4] = { 0xff };
        
        _wire.beginTransaction();

        // start frame
        _wire.transmitBytes(startFrame, sizeof(startFrame));
        
        // data
        _wire.transmitBytes(_data, _sizeData);

        // end frame
        _wire.transmitBytes(endFrame, sizeof(endFrame));
        
        _wire.endTransaction();
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called only if changes to buffer
        return false;
    }

    bool SwapBuffers()
    {
        return false;
    }

    uint8_t* getData() const
    {
        return _data;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    };

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
        _wire.applySettings(settings);
    }

private:
    const size_t   _sizeData;   // Size of '_data' buffer below

    T_TWOWIRE _wire;
    uint8_t* _data;       // Holds LED color values
};

typedef Hd108MethodBase<TwoWireBitBangImple> Hd108Method;

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
#include "TwoWireSpiImple.h"
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed40Mhz>> Hd108Spi40MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed20Mhz>> Hd108Spi20MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed10Mhz>> Hd108Spi10MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed5Mhz>> Hd108Spi5MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed2Mhz>> Hd108Spi2MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed1Mhz>> Hd108Spi1MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed500Khz>> Hd108Spi500KhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeedHz>> Hd108SpiHzMethod;

typedef Hd108Spi10MhzMethod Hd108SpiMethod;
#endif

#if defined(ARDUINO_ARCH_ESP32)
// Give option to use Vspi alias of Spi class if wanting to specify which SPI peripheral is used on the ESP32
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed40Mhz>> Hd108Esp32Vspi40MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed20Mhz>> Hd108Esp32Vspi20MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed10Mhz>> Hd108Esp32Vspi10MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed5Mhz>> Hd108Esp32Vspi5MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed2Mhz>> Hd108Esp32Vspi2MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed1Mhz>> Hd108Esp32Vspi1MhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeed500Khz>> Hd108Esp32Vspi500KhzMethod;
typedef Hd108MethodBase<TwoWireSpiImple<SpiSpeedHz>> Hd108Esp32VspiHzMethod;

typedef Hd108Spi10MhzMethod Hd108Esp32VspiMethod;

#include "TwoWireHspiImple.h"
typedef Hd108MethodBase<TwoWireHspiImple<SpiSpeed40Mhz>> Hd108Esp32Hspi40MhzMethod;
typedef Hd108MethodBase<TwoWireHspiImple<SpiSpeed20Mhz>> Hd108Esp32Hspi20MhzMethod;
typedef Hd108MethodBase<TwoWireHspiImple<SpiSpeed10Mhz>> Hd108Esp32Hspi10MhzMethod;
typedef Hd108MethodBase<TwoWireHspiImple<SpiSpeed5Mhz>> Hd108Esp32Hspi5MhzMethod;
typedef Hd108MethodBase<TwoWireHspiImple<SpiSpeed2Mhz>> Hd108Esp32Hspi2MhzMethod;
typedef Hd108MethodBase<TwoWireHspiImple<SpiSpeed1Mhz>> Hd108Esp32Hspi1MhzMethod;
typedef Hd108MethodBase<TwoWireHspiImple<SpiSpeed500Khz>> Hd108Esp32Hspi500KhzMethod;
typedef Hd108MethodBase<TwoWireHspiImple<SpiSpeedHz>> Hd108Esp32HspiHzMethod;

typedef Hd108Esp32Hspi10MhzMethod Hd108Esp32HspiMethod;
#endif
