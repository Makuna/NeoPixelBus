/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp8266 and Esp32

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

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

#if defined(ARDUINO_ARCH_ESP8266)
#include <eagle_soc.h>
#endif

#define CYCLES_LOOPTEST   (4) // adjustment due to loop exit test instruction cycles

class NeoEspSpeedWs2811
{
public:
    const static uint32_t T0H = (F_CPU / 3333333 - CYCLES_LOOPTEST); // 0.3us
    const static uint32_t T1H = (F_CPU / 1052632 - CYCLES_LOOPTEST); // 0.95us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit
};

class NeoEspSpeedTm1814
{
public:
    const static uint32_t T0H = (F_CPU / 2916666 - CYCLES_LOOPTEST); // 0.35us
    const static uint32_t T1H = (F_CPU / 1666666 - CYCLES_LOOPTEST); // 0.75us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit
};

class NeoEspSpeedTm1829
{
public:
    const static uint32_t T0H = (F_CPU / 3333333 - CYCLES_LOOPTEST); // 0.3us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit
};

class NeoEspSpeed800Mhz
{
public:
    const static uint32_t T0H = (F_CPU / 2500000 - CYCLES_LOOPTEST); // 0.4us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit
};

class NeoEspSpeed400Mhz
{
public:
    const static uint32_t T0H = (F_CPU / 2000000 - CYCLES_LOOPTEST); 
    const static uint32_t T1H = (F_CPU /  833333 - CYCLES_LOOPTEST); 
    const static uint32_t Period = (F_CPU / 400000 - CYCLES_LOOPTEST);
};

class NeoEspSpeedApa106
{
public:
    const static uint32_t T0H = (F_CPU / 2857143 - CYCLES_LOOPTEST); // 0.35us
    const static uint32_t T1H = (F_CPU / 740741 - CYCLES_LOOPTEST); // 1.35
    const static uint32_t Period = (F_CPU / 606061 - CYCLES_LOOPTEST); // 1.65us
};

extern void neoEspBitBangWritePixels(uint8_t* pixels, uint8_t* end, uint8_t pin, uint32_t t0h, uint32_t t1h, uint32_t period, bool invert);

class NeoEspNoninverted
{
public:
    const static uint8_t IdleLevel = LOW;
};

class NeoEspInverted
{
public:
    const static uint8_t IdleLevel = HIGH;
};

class NeoEspBitBangSpeedWs2811 : public NeoEspSpeedWs2811, public NeoEspNoninverted
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoEspBitBangSpeedWs2812x : public NeoEspSpeed800Mhz, public NeoEspNoninverted
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoEspBitBangSpeedSk6812 : public NeoEspSpeed800Mhz, public NeoEspNoninverted
{
public:
    static const uint32_t ResetTimeUs = 80;
};

// normal is inverted signal
class NeoEspBitBangSpeedTm1814 : public NeoEspSpeedTm1814, public NeoEspInverted
{
public:
    static const uint32_t ResetTimeUs = 200;
};

// normal is inverted signal
class NeoEspBitBangSpeedTm1829 : public NeoEspSpeedTm1829, public NeoEspInverted
{
public:
    static const uint32_t ResetTimeUs = 200;
};

class NeoEspBitBangSpeed800Kbps : public NeoEspSpeed800Mhz, public NeoEspNoninverted
{
public:
    static const uint32_t ResetTimeUs = 50; 
};

class NeoEspBitBangSpeed400Kbps : public NeoEspSpeed400Mhz, public NeoEspNoninverted
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoEspBitBangSpeedApa106 : public NeoEspSpeedApa106, public NeoEspNoninverted
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoEspBitBangInvertedSpeedWs2811 : public NeoEspSpeedWs2811, public NeoEspInverted
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoEspBitBangInvertedSpeedWs2812x : public NeoEspSpeed800Mhz, public NeoEspInverted
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoEspBitBangInvertedSpeedSk6812 : public NeoEspSpeed800Mhz, public NeoEspInverted
{
public:
    static const uint32_t ResetTimeUs = 80;
};

// normal is inverted signal, so inverted is normal
class NeoEspBitBangInvertedSpeedTm1814 : public NeoEspSpeedTm1814, public NeoEspNoninverted
{
public:
    static const uint32_t ResetTimeUs = 200;
};

// normal is inverted signal, so inverted is normal
class NeoEspBitBangInvertedSpeedTm1829 : public NeoEspSpeedTm1829, public NeoEspNoninverted
{
public:
    static const uint32_t ResetTimeUs = 200;
};

class NeoEspBitBangInvertedSpeed800Kbps : public NeoEspSpeed800Mhz, public NeoEspInverted
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoEspBitBangInvertedSpeed400Kbps : public NeoEspSpeed400Mhz, public NeoEspInverted
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoEspBitBangInvertedSpeedApa106 : public NeoEspSpeedApa106, public NeoEspInverted
{
public:
    static const uint32_t ResetTimeUs = 50;
};


template<typename T_SPEED> class NeoEspBitBangMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEspBitBangMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin)
    {
        pinMode(pin, OUTPUT);

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()
    }

    ~NeoEspBitBangMethodBase()
    {
        pinMode(_pin, INPUT);

        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        uint32_t delta = micros() - _endTime;

        return (delta >= T_SPEED::ResetTimeUs);
    }

    void Initialize()
    {
        digitalWrite(_pin, T_SPEED::IdleLevel);

        _endTime = micros();
    }

    void Update(bool)
    {
        // Data latch = 50+ microsecond pause in the output stream.  Rather than
        // put a delay at the end of the function, the ending time is noted and
        // the function will simply hold off (if needed) on issuing the
        // subsequent round of data until the latch time has elapsed.  This
        // allows the mainline code to start generating the next frame of data
        // rather than stalling for the latch.
        while (!IsReadyToUpdate())
        {
            yield(); // allows for system yield if needed
        }

        // Need 100% focus on instruction timing
#if defined(ARDUINO_ARCH_ESP32)
        delay(1); // required
        portMUX_TYPE updateMux = portMUX_INITIALIZER_UNLOCKED;

        portENTER_CRITICAL(&updateMux);
#else
        noInterrupts(); 
#endif

        neoEspBitBangWritePixels(_data,
            _data + _sizeData,
            _pin,
            T_SPEED::T0H,
            T_SPEED::T1H,
            T_SPEED::Period,
            T_SPEED::IdleLevel);
        
#if defined(ARDUINO_ARCH_ESP32)
        portEXIT_CRITICAL(&updateMux);
#else
        interrupts();
#endif

        // save EOD time for latch on next call
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
    }

private:
    const size_t  _sizeData;   // Size of '_data' buffer below
    const uint8_t _pin;            // output pin number

    uint32_t _endTime;       // Latch timing reference
    uint8_t* _data;        // Holds LED color values
};


#if defined(ARDUINO_ARCH_ESP32)

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811> NeoEsp32BitBangWs2811Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x> NeoEsp32BitBangWs2812xMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812> NeoEsp32BitBangSk6812Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814Inverted> NeoEsp32BitBangTm1814Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829Inverted> NeoEsp32BitBangTm1829Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps> NeoEsp32BitBang800KbpsMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps> NeoEsp32BitBang400KbpsMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106> NeoEsp32BitBangApa106Method;

typedef NeoEsp32BitBangWs2812xMethod NeoEsp32BitBangWs2813Method;
typedef NeoEsp32BitBang800KbpsMethod NeoEsp32BitBangWs2812Method;
typedef NeoEsp32BitBangWs2812xMethod NeoEsp32BitBangWs2816Method;
typedef NeoEsp32BitBangTm1814Method NeoEsp32BitBangTm1914Method;
typedef NeoEsp32BitBangSk6812Method NeoEsp32BitBangLc8812Method;

typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedWs2811> NeoEsp32BitBangWs2811InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedWs2812x> NeoEsp32BitBangWs2812xInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedSk6812> NeoEsp32BitBangSk6812InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedTm1814> NeoEsp32BitBangTm1814InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedTm1829> NeoEsp32BitBangTm1829InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeed800Kbps> NeoEsp32BitBang800KbpsInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeed400Kbps> NeoEsp32BitBang400KbpsInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedApa106> NeoEsp32BitBangApa106InvertedMethod;

typedef NeoEsp32BitBangWs2812xInvertedMethod NeoEsp32BitBangWs2813InvertedMethod;
typedef NeoEsp32BitBang800KbpsInvertedMethod NeoEsp32BitBangWs2812InvertedMethod;
typedef NeoEsp32BitBangWs2812xInvertedMethod NeoEsp32BitBangWs2816InvertedMethod;
typedef NeoEsp32BitBangTm1814InvertedMethod NeoEsp32BitBangTm1914InvertedMethod;
typedef NeoEsp32BitBangSk6812InvertedMethod NeoEsp32BitBangLc8812InvertedMethod;

#else

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811> NeoEsp8266BitBangWs2811Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x> NeoEsp8266BitBangWs2812xMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812> NeoEsp8266BitBangSk6812Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814> NeoEsp8266BitBangTm1814Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829> NeoEsp8266BitBangTm1829Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps> NeoEsp8266BitBang800KbpsMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps> NeoEsp8266BitBang400KbpsMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106> NeoEsp8266BitBangApa106Method;

typedef NeoEsp8266BitBangWs2812xMethod NeoEsp8266BitBangWs2813Method;
typedef NeoEsp8266BitBang800KbpsMethod NeoEsp8266BitBangWs2812Method;
typedef NeoEsp8266BitBangWs2812xMethod NeoEsp8266BitBangWs2816Method;
typedef NeoEsp8266BitBangTm1814Method NeoEsp8266BitBangTm1914Method;
typedef NeoEsp8266BitBangSk6812Method NeoEsp8266BitBangLc8812Method;

typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedWs2811> NeoEsp8266BitBangWs2811InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedWs2812x> NeoEsp8266BitBangWs2812xInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedSk6812> NeoEsp8266BitBangSk6812InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedTm1814> NeoEsp8266BitBangTm1814InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedTm1829> NeoEsp8266BitBangTm1829InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeed800Kbps> NeoEsp8266BitBang800KbpsInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeed400Kbps> NeoEsp8266BitBang400KbpsInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangInvertedSpeedApa106> NeoEsp8266BitBangApa106InvertedMethod;

typedef NeoEsp8266BitBangWs2812xInvertedMethod NeoEsp8266BitBangWs2813InvertedMethod;
typedef NeoEsp8266BitBang800KbpsInvertedMethod NeoEsp8266BitBangWs2812InvertedMethod;
typedef NeoEsp8266BitBangWs2812xInvertedMethod NeoEsp8266BitBangWs2816InvertedMethod;
typedef NeoEsp8266BitBangTm1814InvertedMethod NeoEsp8266BitBangTm1914InvertedMethod;
typedef NeoEsp8266BitBangSk6812InvertedMethod NeoEsp8266BitBangLc8812InvertedMethod;

#endif

// ESP bitbang doesn't have defaults and should avoided except for testing

#endif // defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
