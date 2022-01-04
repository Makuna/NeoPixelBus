/*-------------------------------------------------------------------------
NeoPixel library helper functions for TLC5947 24 channel PWM controller using general Pins.

Written by Michael C. Miller.
Written by Dennis Kasprzyk.

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

class TLC5947ConverterBase
{
public:
};

class TLC5947Converter8Bit : TLC5947ConverterBase
{
public:
    static const size_t ChannelSize = 1;
    static void ConvertFrame(uint8_t* dst, uint8_t* src, uint8_t numChannel)
    {
        size_t gap = (12 - (numChannel >> 1)) * 3;
        memset(dst, 0, gap);
        dst += gap;

        if (numChannel & 1)
        {
            *dst++ = 0;
            *dst++ = *src >> 4;
            *dst++ = *src--; 
        }
        for (int i = 0; i < (numChannel >> 1); ++i)
        {
            *dst++ = *src;
            *dst++ = (*src-- & 0xf0) | (*src >> 4);
            *dst++ = ((*src << 4) & 0xf0) | (*src-- >> 4); 
        }
    }
};

class TLC5947Converter16Bit : TLC5947ConverterBase
{
public:
    static const size_t ChannelSize = 2;
    static void ConvertFrame(uint8_t* dst, uint8_t* src, uint8_t numChannel)
    {
        size_t gap = (12 - (numChannel >> 1)) * 3;
        memset(dst, 0, gap);
        dst += gap;

        uint16_t* sPtr = (uint16_t*)src;
        if (numChannel & 1)
        {
            *dst++ = 0;
            *dst++ = *sPtr >> 12;
            *dst++ = *sPtr-- >> 4;
        }
        for (int i = 0; i < (numChannel >> 1); ++i)
        {
            *dst++ = *sPtr >> 8; 
            *dst++ = (*sPtr-- & 0xf0) | (*sPtr >> 12);
            *dst++ = *sPtr-- >> 4;
        }
    }
};


template<typename T_BITCONVERT, typename T_TWOWIRE> class TLC5947MethodBase
{
public:
    typedef typename T_TWOWIRE::SettingsObject SettingsObject;

    TLC5947MethodBase(uint8_t pinClock, uint8_t pinData, uint8_t pinLatch, int8_t pinOutputEnable, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _channelCount(pixelCount * elementSize / T_BITCONVERT::ChannelSize),
        _wire(pinClock, pinData),
        _pinLatch(pinLatch),
        _pinOutputEnable(pinOutputEnable)
    {
        _data = static_cast<uint8_t*>(malloc(_sizeData));
        pinMode(pinLatch, OUTPUT);
        if (pinOutputEnable >= 0)
        {
            pinMode(pinOutputEnable, OUTPUT);
            digitalWrite(pinOutputEnable, HIGH);
        }
    }

    TLC5947MethodBase(uint8_t pinClock, uint8_t pinData, uint8_t pinLatch, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        TLC5947MethodBase(pinClock, pinData, pinLatch, -1, pixelCount, elementSize, settingsSize)
    {
    }

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
    TLC5947MethodBase(uint8_t pinLatch, uint8_t pinOutputEnable, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        TLC5947MethodBase(SCK, MOSI, pinLatch, pinOutputEnable, pixelCount, elementSize, settingsSize)
    {
    }

    TLC5947MethodBase(uint8_t pinLatch, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        TLC5947MethodBase(SCK, MOSI, pinLatch, -1, pixelCount, elementSize, settingsSize)
    {
    }
#endif

    ~TLC5947MethodBase()
    {
        free(_data);
        pinMode(_pinLatch, INPUT);
        if (_pinOutputEnable >= 0)
            pinMode(_pinOutputEnable, INPUT);
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

    void WriteBuffer(uint8_t* buffer)
    {
        if (_pinOutputEnable >= 0)
            digitalWrite(_pinOutputEnable, HIGH);
        
        digitalWrite(_pinLatch, LOW);
        _wire.beginTransaction();

       
        _wire.transmitBytes(buffer, 36);   
      
        _wire.endTransaction();
        digitalWrite(_pinLatch, HIGH);
        digitalWrite(_pinLatch, LOW);
        if (_pinOutputEnable >= 0)
            digitalWrite(_pinOutputEnable, LOW);
    }

    void Update(bool)
    { 
        
        if (_pinOutputEnable >= 0)
            digitalWrite(_pinOutputEnable, HIGH);
        
        digitalWrite(_pinLatch, LOW);
        _wire.beginTransaction();

        uint8_t* ptr = _data + ((_channelCount - 1) * T_BITCONVERT::ChannelSize);
        uint16_t remCh = _channelCount;
        uint16_t numCh = _channelCount % 24;
        if (numCh == 0)
            numCh = 24;

        while (remCh > 0)
        {
            T_BITCONVERT::ConvertFrame(_frame, ptr, numCh);
            _wire.transmitBytes(_frame, sizeof(_frame));
            ptr -= numCh * T_BITCONVERT::ChannelSize;
            remCh -= numCh;
            numCh = 24;
        }           
      
        _wire.endTransaction();
        digitalWrite(_pinLatch, HIGH);
        digitalWrite(_pinLatch, LOW);
        if (_pinOutputEnable >= 0)
            digitalWrite(_pinOutputEnable, LOW);
    }

    uint8_t* getData() const
    {
        return _data;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    };

    void applySettings(const SettingsObject& settings)
    {
        _wire.applySettings(settings);
    }

private:
    const size_t   _sizeData;   // Size of '_data' buffer below
    const uint16_t _channelCount;

    T_TWOWIRE _wire;
    uint8_t*  _data;       // Holds LED color values
    uint8_t   _frame[36];  // Holds channel values for one module
    uint8_t   _pinLatch;
    int8_t    _pinOutputEnable;
};

typedef TLC5947MethodBase<TLC5947Converter8Bit, TwoWireBitBangImple> TLC5947Method;
typedef TLC5947MethodBase<TLC5947Converter16Bit, TwoWireBitBangImple> TLC5947Method16Bit;

#if !defined(__AVR_ATtiny85__) && !defined(ARDUINO_attiny)
#include "TwoWireSpiImple.h"

// for standalone
typedef TLC5947MethodBase<TLC5947Converter8Bit, TwoWireSpiImple<SpiSpeed30Mhz>> TLC5947Spi30MhzMethod;
typedef TLC5947MethodBase<TLC5947Converter16Bit, TwoWireSpiImple<SpiSpeed30Mhz>> TLC5947Spi30MhzMethod16Bit;

// for cascaded devices
typedef TLC5947MethodBase<TLC5947Converter8Bit, TwoWireSpiImple<SpiSpeed15Mhz>> TLC5947Spi15MhzMethod;
typedef TLC5947MethodBase<TLC5947Converter16Bit, TwoWireSpiImple<SpiSpeed15Mhz>> TLC5947Spi15MhzMethod16Bit;

typedef TLC5947MethodBase<TLC5947Converter8Bit, TwoWireSpiImple<SpiSpeed15Mhz>> TLC5947SpiMethod;
typedef TLC5947MethodBase<TLC5947Converter16Bit, TwoWireSpiImple<SpiSpeed15Mhz>> TLC5947SpiMethod16Bit;


#endif



