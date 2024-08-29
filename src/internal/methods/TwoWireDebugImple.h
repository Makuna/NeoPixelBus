/*-------------------------------------------------------------------------
NeoPixel library helper functions for DotStars using general Pins (APA102/LPD8806).

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


class TwoWireDebugImple
{
public:
    typedef NeoNoSettings SettingsObject;

    TwoWireDebugImple(uint8_t pinClock, uint8_t pinData) :
        _pinClock(pinClock),
        _pinData(pinData)
    {
    }

    ~TwoWireDebugImple()
    {
    }
#if defined(ARDUINO_ARCH_ESP32)
    void begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
    {
        Serial.print("TwoWireDebugImple(sck = ");
        Serial.print(sck);
        Serial.print(", mosi = ");
        Serial.print(mosi);
        Serial.println(")");
    }
#endif

    void begin()
    {
        Serial.print("TwoWireDebugImple(pinClock = ");
        Serial.print(_pinClock);
        Serial.print(", pinData = ");
        Serial.print(_pinData);
        Serial.println(")");
    }

    void beginTransaction()
    {
        Serial.println("TwoWireDebugImple beginTransaction:");
    }

    void endTransaction()
    {
        Serial.println("TwoWireDebugImple EndTransaction:");
        Serial.println();
    }

    void transmitBit(uint8_t bit)
    {
        Serial.print(" b");
        Serial.println(bit);
    }

    void transmitByte(uint8_t data)
    {
        Serial.print(" 0x");

        Serial.print(data >> 4, HEX);
        Serial.print(data & 0x0f, HEX);
        Serial.print(" b");
        
        for (int bit = 7; bit >= 0; bit--)
        {
            Serial.print((data & 0x80) ? 1 : 0);
            data <<= 1;
        }
        Serial.println();
    }

    void transmitBytes(const uint8_t* data, size_t dataSize)
    {
        const uint8_t* endData = data + dataSize;
        while (data < endData)
        {
            transmitByte(*data++);
        }
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    const uint8_t  _pinClock;     // output pin number for clock line
    const uint8_t  _pinData;      // output pin number for data line
};


template <typename T_TWOWIRE>
class TwoWireDebugShimImple : public T_TWOWIRE
{
public:
    TwoWireDebugShimImple(uint8_t pinClock, uint8_t pinData) :
        T_TWOWIRE(pinClock, pinData),
        _pinClock(pinClock),
        _pinData(pinData)
    {
    }

#if defined(ARDUINO_ARCH_ESP32)
    void begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
    {
        Serial.print("TwoWireDebugImple(sck = ");
        Serial.print(sck);
        Serial.print(", mosi = ");
        Serial.print(mosi);
        Serial.println(")");

        T_TWOWIRE::begin(sck, miso, mosi, ss);
    }
#endif

    void begin()
    {
        Serial.print("TwoWireDebugImple(pinClock = ");
        Serial.print(_pinClock);
        Serial.print(", pinData = ");
        Serial.print(_pinData);
        Serial.println(")");

        T_TWOWIRE::begin();
    }

    void beginTransaction()
    {
        Serial.println("TwoWireDebugImple beginTransaction:");

        T_TWOWIRE::beginTransaction();
    }

    void endTransaction()
    {
        Serial.println("TwoWireDebugImple EndTransaction:");
        Serial.println();

        T_TWOWIRE::endTransaction();
    }

    void transmitBit(uint8_t bit)
    {
        Serial.print(" b");
        Serial.println(bit);

        T_TWOWIRE::transmitBit(bit);
    }

    void transmitByte(uint8_t data)
    {
        Serial.print(" 0x");

        Serial.print(data >> 4, HEX);
        Serial.print(data & 0x0f, HEX);
        Serial.print(" b");
        
        uint8_t shift = data;

        for (int bit = 7; bit >= 0; bit--)
        {
            Serial.print((shift & 0x80) ? 1 : 0);
            shift <<= 1;
        }
        Serial.println();

        T_TWOWIRE::transmitByte(data);
    }

    void transmitBytes(const uint8_t* data, size_t dataSize)
    {
        const uint8_t* endData = data + dataSize;
        const uint8_t* dataSrc = data;

        while (dataSrc < endData)
        {
            transmitByte(*dataSrc++);
        }
        T_TWOWIRE::transmitBytes(data, dataSize);
    }

private:
    const uint8_t  _pinClock;     // output pin number for clock line
    const uint8_t  _pinData;      // output pin number for data line
};