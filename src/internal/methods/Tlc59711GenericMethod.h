/*-------------------------------------------------------------------------
NeoPixel library helper functions for Tlc59711

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


template<typename T_TWOWIRE> class Tlc59711MethodBase
{
public:
    typedef typename T_TWOWIRE::SettingsObject SettingsObject;

    Tlc59711MethodBase(uint8_t pinClock, 
            uint8_t pinData, 
            uint16_t pixelCount, 
            size_t elementSize, 
            size_t settingsSize) :
        _sizeData(NeoUtil::RoundUp(pixelCount * elementSize, c_dataPerChipSize) + settingsSize),
        _sizeSettings(settingsSize),
        _wire(pinClock, pinData)
    {
        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()
    }

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
    Tlc59711MethodBase(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        Tlc59711MethodBase(SCK, MOSI, pixelCount, elementSize, settingsSize)
    {
    }
#endif

    ~Tlc59711MethodBase()
    {
        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        // need 8 clock periods plus 1.34us for the chips to all latch
        // slowest clock is 2us clock period, so 8x2+1.24 = ~17.24
        // since we don't have access to actual clock value at this level
        // the slowest will be used until a refactor can be made to allow
        // the actual wire clock (or bitbang estimate of it) at this level

        uint32_t delta = micros() - _endTime;

        return (delta >= 18);
    }

#if defined(ARDUINO_ARCH_ESP32)
    void Initialize(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
    {
        _wire.begin(sck, miso, mosi, ss);

        _endTime = micros();
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

        const size_t sizeSettings = (_sizeSettings / 2); // we have two variants
        const uint8_t* pSettingsA = _data;
        const uint8_t* pSettingsB = _data + sizeSettings;
        // expects chips in reverse order
        uint8_t* pData = _data + _sizeData - c_dataPerChipSize;
        uint8_t* pDataEnd = _data + _sizeSettings;
        const uint8_t* pSettings = pSettingsA;

        _wire.beginTransaction();

        while (pData >= pDataEnd)
        {
            // settings
            _wire.transmitBytes(pSettings, sizeSettings);

            // toggle settings varient for next chip
            if (pSettings == pSettingsA)
            {
                pSettings = pSettingsB;
            }
            else
            {
                pSettings = pSettingsA;
            }

            // data send and advance to next chips data
            _wire.transmitBytes(pData, c_dataPerChipSize);
            pData -= c_dataPerChipSize;
        }
        
        _wire.endTransaction();

        _endTime = micros();
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called only if changes to buffer
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
    static constexpr size_t c_dataPerChipSize = 24;

    const size_t   _sizeData;   // Size of '_data' buffer below
    const size_t   _sizeSettings;

    T_TWOWIRE _wire;
    uint8_t* _data;       // Holds Settings and LED color values
    uint32_t _endTime;       // Latch timing reference
};

typedef Tlc59711MethodBase<TwoWireBitBangImple> Tlc59711Method;

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
#include "TwoWireSpiImple.h"
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed40Mhz>> Tlc59711Spi40MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed20Mhz>> Tlc59711Spi20MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed10Mhz>> Tlc59711Spi10MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed5Mhz>> Tlc59711Spi5MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed2Mhz>> Tlc59711Spi2MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed1Mhz>> Tlc59711Spi1MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed500Khz>> Tlc59711Spi500KhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeedHz>> Tlc59711SpiHzMethod;

typedef Tlc59711Spi10MhzMethod Tlc59711SpiMethod;
#endif

#if defined(ARDUINO_ARCH_ESP32)
// Give option to use Vspi alias of Spi class if wanting to specify which SPI peripheral is used on the ESP32
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed40Mhz>> Tlc59711Esp32Vspi40MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed20Mhz>> Tlc59711Esp32Vspi20MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed10Mhz>> Tlc59711Esp32Vspi10MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed5Mhz>> Tlc59711Esp32Vspi5MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed2Mhz>> Tlc59711Esp32Vspi2MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed1Mhz>> Tlc59711Esp32Vspi1MhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeed500Khz>> Tlc59711Esp32Vspi500KhzMethod;
typedef Tlc59711MethodBase<TwoWireSpiImple<SpiSpeedHz>> Tlc59711Esp32VspiHzMethod;

typedef Tlc59711Spi10MhzMethod Tlc59711Esp32VspiMethod;

#include "TwoWireHspiImple.h"
typedef Tlc59711MethodBase<TwoWireHspiImple<SpiSpeed40Mhz>> Tlc59711Esp32Hspi40MhzMethod;
typedef Tlc59711MethodBase<TwoWireHspiImple<SpiSpeed20Mhz>> Tlc59711Esp32Hspi20MhzMethod;
typedef Tlc59711MethodBase<TwoWireHspiImple<SpiSpeed10Mhz>> Tlc59711Esp32Hspi10MhzMethod;
typedef Tlc59711MethodBase<TwoWireHspiImple<SpiSpeed5Mhz>> Tlc59711Esp32Hspi5MhzMethod;
typedef Tlc59711MethodBase<TwoWireHspiImple<SpiSpeed2Mhz>> Tlc59711Esp32Hspi2MhzMethod;
typedef Tlc59711MethodBase<TwoWireHspiImple<SpiSpeed1Mhz>> Tlc59711Esp32Hspi1MhzMethod;
typedef Tlc59711MethodBase<TwoWireHspiImple<SpiSpeed500Khz>> Tlc59711Esp32Hspi500KhzMethod;
typedef Tlc59711MethodBase<TwoWireHspiImple<SpiSpeedHz>> Tlc59711Esp32HspiHzMethod;

typedef Tlc59711Esp32Hspi10MhzMethod Tlc59711Esp32HspiMethod;
#endif
