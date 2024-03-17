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
#if defined(CONFIG_IDF_TARGET_ESP32C3)
#define CYCLES_LOOPTEST   (1) // adjustment due to loop exit test instruction cycles
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#define CYCLES_LOOPTEST   (2) // adjustment due to loop exit test instruction cycles
#else
#define CYCLES_LOOPTEST   (4) // adjustment due to loop exit test instruction cycles
#endif

extern bool neoEspBitBangWriteSpacingPixels(const uint8_t* pixels, 
    const uint8_t* end, 
    uint8_t pin, 
    uint32_t t0h, 
    uint32_t t1h, 
    uint32_t period,
    size_t sizePixel,
    uint32_t tLatch, 
    bool invert);


class NeoEspNotInverted
{
public:
    const static uint8_t IdleLevel = LOW;
};

class NeoEspInverted
{
public:
    const static uint8_t IdleLevel = HIGH;
};

class NeoEspBitBangSpeedWs2811 
{
public:
    const static uint32_t T0H = (F_CPU / 3333333 - CYCLES_LOOPTEST); // 0.3us
    const static uint32_t T1H = (F_CPU / 1052632 - CYCLES_LOOPTEST); // 0.95us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit

    static const uint32_t ResetTimeUs = 300;
    const static uint32_t TLatch = (F_CPU / 22222 - CYCLES_LOOPTEST); // 45us, be generous
};

class NeoEspBitBangSpeedWs2812x 
{
public:
    const static uint32_t T0H = (F_CPU / 2500000 - CYCLES_LOOPTEST); // 0.4us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit

    static const uint32_t ResetTimeUs = 300;
    const static uint32_t TLatch = (F_CPU / 22222 - CYCLES_LOOPTEST); // 45us, be generous
};

class NeoEspBitBangSpeedWs2805
{
public:
    const static uint32_t T0H = (F_CPU / 2857143 - CYCLES_LOOPTEST); // 0.35us
    const static uint32_t T1H = (F_CPU / 740741 - CYCLES_LOOPTEST); // 1.35
    const static uint32_t Period = (F_CPU / 606061 - CYCLES_LOOPTEST); // 1.65us

    static const uint32_t ResetTimeUs = 300;
    const static uint32_t TLatch = (F_CPU / 22222 - CYCLES_LOOPTEST); // 45us, be generous
};

class NeoEspBitBangSpeedSk6812
{
public:
    const static uint32_t T0H = (F_CPU / 2500000 - CYCLES_LOOPTEST); // 0.4us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit

    static const uint32_t ResetTimeUs = 80;
    const static uint32_t TLatch = (F_CPU / 13333 - CYCLES_LOOPTEST); // 75us, be generous
};

// Tm1814 normal is inverted signal
class NeoEspBitBangSpeedTm1814
{
public:
    const static uint32_t T0H = (F_CPU / 2916666 - CYCLES_LOOPTEST); // 0.35us
    const static uint32_t T1H = (F_CPU / 1666666 - CYCLES_LOOPTEST); // 0.75us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit

    static const uint32_t ResetTimeUs = 200;
    const static uint32_t TLatch = (F_CPU / 10000 - CYCLES_LOOPTEST); // 100us, be generous
};

// Tm1829 normal is inverted signal
class NeoEspBitBangSpeedTm1829
{
public:
    const static uint32_t T0H = (F_CPU / 3333333 - CYCLES_LOOPTEST); // 0.3us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit

    static const uint32_t ResetTimeUs = 200;
    const static uint32_t TLatch = (F_CPU / 10000 - CYCLES_LOOPTEST); // 100us, be generous
};

class NeoEspBitBangSpeed800Kbps
{
public:
    const static uint32_t T0H = (F_CPU / 2500000 - CYCLES_LOOPTEST); // 0.4us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit

    static const uint32_t ResetTimeUs = 50; 
    const static uint32_t TLatch = (F_CPU / 22222 - CYCLES_LOOPTEST); // 45us, be generous
};

class NeoEspBitBangSpeed400Kbps
{
public:
    const static uint32_t T0H = (F_CPU / 2000000 - CYCLES_LOOPTEST);
    const static uint32_t T1H = (F_CPU / 833333 - CYCLES_LOOPTEST);
    const static uint32_t Period = (F_CPU / 400000 - CYCLES_LOOPTEST);

    static const uint32_t ResetTimeUs = 50;
    const static uint32_t TLatch = (F_CPU / 22222 - CYCLES_LOOPTEST); // 45us, be generous
};

class NeoEspBitBangSpeedApa106
{
public:
    const static uint32_t T0H = (F_CPU / 2857143 - CYCLES_LOOPTEST); // 0.35us
    const static uint32_t T1H = (F_CPU / 740741 - CYCLES_LOOPTEST); // 1.35
    const static uint32_t Period = (F_CPU / 606061 - CYCLES_LOOPTEST); // 1.65us

    static const uint32_t ResetTimeUs = 50;
    const static uint32_t TLatch = (F_CPU / 22222 - CYCLES_LOOPTEST); // 45us, be generous
};

class NeoEspBitBangSpeedIntertek
{
public:
    const static uint32_t T0H = (F_CPU / 2500000 - CYCLES_LOOPTEST); // 0.4us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit

    const static uint32_t ResetTimeUs = 12470;
    // const static uint32_t TInterPixel = (F_CPU / 50000); // 20us
    const static uint32_t TLatch = (F_CPU / 22222 - CYCLES_LOOPTEST); // 45us??? couldn't find datasheet
};

template <typename T_SPEED, bool V_INTER_PIXEL_ISR> class NeoEspTLatch
{ 
public:
    const static uint32_t TLatch = T_SPEED::TLatch;
};

// Partial specialization for no interrupt case resolution
template <typename T_SPEED> class NeoEspTLatch<T_SPEED, false>
{
public:    
    const static uint32_t TLatch = 0;
};

template<typename T_SPEED, typename T_INVERTED, bool V_INTER_PIXEL_ISR> class NeoEspBitBangMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEspBitBangMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizePixel(elementSize),
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
        digitalWrite(_pin, T_INVERTED::IdleLevel);

        _endTime = micros();
    }

    void Update(bool)
    {
        bool done = false;
        for (unsigned retries = 0; !done && retries < 4; ++retries)
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

            done = neoEspBitBangWriteSpacingPixels(_data,
                _data + _sizeData,
                _pin,
                T_SPEED::T0H,
                T_SPEED::T1H,
                T_SPEED::Period,
                _sizePixel,
                NeoEspTLatch<T_SPEED, V_INTER_PIXEL_ISR>::TLatch,
                T_INVERTED::IdleLevel);

            // save EOD time for latch on next call
            _endTime = micros();
        }
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
    const size_t _sizePixel; // size of a pixel in _data
    const size_t  _sizeData;   // Size of '_data' buffer below
    const uint8_t _pin;            // output pin number

    uint32_t _endTime;       // Latch timing reference
    uint8_t* _data;        // Holds LED color values
};


#if defined(ARDUINO_ARCH_ESP32)

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811, NeoEspNotInverted, true> NeoEsp32BitBangWs2811Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x, NeoEspNotInverted, true> NeoEsp32BitBangWs2812xMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2805, NeoEspNotInverted, true> NeoEsp32BitBangWs2805Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812, NeoEspNotInverted, true> NeoEsp32BitBangSk6812Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814, NeoEspInverted, true> NeoEsp32BitBangTm1814Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829, NeoEspInverted, true> NeoEsp32BitBangTm1829Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps, NeoEspNotInverted, true> NeoEsp32BitBang800KbpsMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps, NeoEspNotInverted, true> NeoEsp32BitBang400KbpsMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106, NeoEspNotInverted, true> NeoEsp32BitBangApa106Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedIntertek, NeoEspNotInverted, true> NeoEsp32BitBangIntertekMethod;

typedef NeoEsp32BitBangWs2812xMethod NeoEsp32BitBangWs2813Method;
typedef NeoEsp32BitBang800KbpsMethod NeoEsp32BitBangWs2812Method;
typedef NeoEsp32BitBangWs2812xMethod NeoEsp32BitBangWs2816Method;
typedef NeoEsp32BitBangTm1814Method NeoEsp32BitBangTm1914Method;
typedef NeoEsp32BitBangSk6812Method NeoEsp32BitBangLc8812Method;

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811, NeoEspInverted, true> NeoEsp32BitBangWs2811InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x, NeoEspInverted, true> NeoEsp32BitBangWs2812xInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2805, NeoEspInverted, true> NeoEsp32BitBangWs2805InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812, NeoEspInverted, true> NeoEsp32BitBangSk6812InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814, NeoEspNotInverted, true> NeoEsp32BitBangTm1814InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829, NeoEspNotInverted, true> NeoEsp32BitBangTm1829InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps, NeoEspInverted, true> NeoEsp32BitBang800KbpsInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps, NeoEspInverted, true> NeoEsp32BitBang400KbpsInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106, NeoEspInverted, true> NeoEsp32BitBangApa106InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedIntertek, NeoEspInverted, true> NeoEsp32BitBangIntertekInvertedMethod;

typedef NeoEsp32BitBangWs2812xInvertedMethod NeoEsp32BitBangWs2813InvertedMethod;
typedef NeoEsp32BitBang800KbpsInvertedMethod NeoEsp32BitBangWs2812InvertedMethod;
typedef NeoEsp32BitBangWs2812xInvertedMethod NeoEsp32BitBangWs2816InvertedMethod;
typedef NeoEsp32BitBangTm1814InvertedMethod NeoEsp32BitBangTm1914InvertedMethod;
typedef NeoEsp32BitBangSk6812InvertedMethod NeoEsp32BitBangLc8812InvertedMethod;

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811, NeoEspNotInverted, false> NeoEsp32BitBangWs2811NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x, NeoEspNotInverted, false> NeoEsp32BitBangWs2812xNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2805, NeoEspNotInverted, false> NeoEsp32BitBangWs2805NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812, NeoEspNotInverted, false> NeoEsp32BitBangSk6812NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814, NeoEspInverted, false> NeoEsp32BitBangTm1814NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829, NeoEspInverted, false> NeoEsp32BitBangTm1829NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps, NeoEspNotInverted, false> NeoEsp32BitBang800KbpsNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps, NeoEspNotInverted, false> NeoEsp32BitBang400KbpsNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106, NeoEspNotInverted, false> NeoEsp32BitBangApa106NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedIntertek, NeoEspNotInverted, false> NeoEsp32BitBangIntertekNoIntrMethod;

typedef NeoEsp32BitBangWs2812xMethod NeoEsp32BitBangWs2813NoIntrMethod;
typedef NeoEsp32BitBang800KbpsMethod NeoEsp32BitBangWs2812NoIntrMethod;
typedef NeoEsp32BitBangWs2812xMethod NeoEsp32BitBangWs2816NoIntrMethod;
typedef NeoEsp32BitBangTm1814Method NeoEsp32BitBangTm1914NoIntrMethod;
typedef NeoEsp32BitBangSk6812Method NeoEsp32BitBangLc8812NoIntrMethod;

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811, NeoEspInverted, false> NeoEsp32BitBangWs2811InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x, NeoEspInverted, false> NeoEsp32BitBangWs2812xInvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2805, NeoEspInverted, false> NeoEsp32BitBangWs2805InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812, NeoEspInverted, false> NeoEsp32BitBangSk6812InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814, NeoEspNotInverted, false> NeoEsp32BitBangTm1814InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829, NeoEspNotInverted, false> NeoEsp32BitBangTm1829InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps, NeoEspInverted, false> NeoEsp32BitBang800KbpsInvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps, NeoEspInverted, false> NeoEsp32BitBang400KbpsInvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106, NeoEspInverted, false> NeoEsp32BitBangApa106InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedIntertek, NeoEspInverted, false> NeoEsp32BitBangIntertekInvertedNoIntrMethod;

typedef NeoEsp32BitBangWs2812xInvertedMethod NeoEsp32BitBangWs2813InvertedNoIntrMethod;
typedef NeoEsp32BitBang800KbpsInvertedMethod NeoEsp32BitBangWs2812InvertedNoIntrMethod;
typedef NeoEsp32BitBangWs2812xInvertedMethod NeoEsp32BitBangWs2816InvertedNoIntrMethod;
typedef NeoEsp32BitBangTm1814InvertedMethod NeoEsp32BitBangTm1914InvertedNoIntrMethod;
typedef NeoEsp32BitBangSk6812InvertedMethod NeoEsp32BitBangLc8812InvertedNoIntrMethod;

#else // defined(ARDUINO_ARCH_ESP8266)

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811, NeoEspNotInverted, true> NeoEsp8266BitBangWs2811Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x, NeoEspNotInverted, true> NeoEsp8266BitBangWs2812xMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2805, NeoEspNotInverted, true> NeoEsp8266BitBangWs2805Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812, NeoEspNotInverted, true> NeoEsp8266BitBangSk6812Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814, NeoEspInverted, true> NeoEsp8266BitBangTm1814Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829, NeoEspInverted, true> NeoEsp8266BitBangTm1829Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps, NeoEspNotInverted, true> NeoEsp8266BitBang800KbpsMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps, NeoEspNotInverted, true> NeoEsp8266BitBang400KbpsMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106, NeoEspNotInverted, true> NeoEsp8266BitBangApa106Method;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedIntertek, NeoEspNotInverted, true> NeoEsp8266BitBangIntertekMethod;

typedef NeoEsp8266BitBangWs2812xMethod NeoEsp8266BitBangWs2813Method;
typedef NeoEsp8266BitBang800KbpsMethod NeoEsp8266BitBangWs2812Method;
typedef NeoEsp8266BitBangWs2812xMethod NeoEsp8266BitBangWs2816Method;
typedef NeoEsp8266BitBangTm1814Method NeoEsp8266BitBangTm1914Method;
typedef NeoEsp8266BitBangSk6812Method NeoEsp8266BitBangLc8812Method;

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811, NeoEspInverted, true> NeoEsp8266BitBangWs2811InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x, NeoEspInverted, true> NeoEsp8266BitBangWs2812xInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2805, NeoEspInverted, true> NeoEsp8266BitBangWs2805InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812, NeoEspInverted, true> NeoEsp8266BitBangSk6812InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814, NeoEspNotInverted, true> NeoEsp8266BitBangTm1814InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829, NeoEspNotInverted, true> NeoEsp8266BitBangTm1829InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps, NeoEspInverted, true> NeoEsp8266BitBang800KbpsInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps, NeoEspInverted, true> NeoEsp8266BitBang400KbpsInvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106, NeoEspInverted, true> NeoEsp8266BitBangApa106InvertedMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedIntertek, NeoEspInverted, true> NeoEsp8266BitBangIntertekInvertedMethod;

typedef NeoEsp8266BitBangWs2812xInvertedMethod NeoEsp8266BitBangWs2813InvertedMethod;
typedef NeoEsp8266BitBang800KbpsInvertedMethod NeoEsp8266BitBangWs2812InvertedMethod;
typedef NeoEsp8266BitBangWs2812xInvertedMethod NeoEsp8266BitBangWs2816InvertedMethod;
typedef NeoEsp8266BitBangTm1814InvertedMethod NeoEsp8266BitBangTm1914InvertedMethod;
typedef NeoEsp8266BitBangSk6812InvertedMethod NeoEsp8266BitBangLc8812InvertedMethod;

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811, NeoEspNotInverted, false> NeoEsp8266BitBangWs2811NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x, NeoEspNotInverted, false> NeoEsp8266BitBangWs2812xNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2805, NeoEspNotInverted, false> NeoEsp8266BitBangWs2805NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812, NeoEspNotInverted, false> NeoEsp8266BitBangSk6812NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814, NeoEspInverted, false> NeoEsp8266BitBangTm1814NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829, NeoEspInverted, false> NeoEsp8266BitBangTm1829NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps, NeoEspNotInverted, false> NeoEsp8266BitBang800KbpsNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps, NeoEspNotInverted, false> NeoEsp8266BitBang400KbpsNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106, NeoEspNotInverted, false> NeoEsp8266BitBangApa106NoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedIntertek, NeoEspNotInverted, false> NeoEsp8266BitBangIntertekNoIntrMethod;

typedef NeoEsp8266BitBangWs2812xMethod NeoEsp8266BitBangWs2813NoIntrMethod;
typedef NeoEsp8266BitBang800KbpsMethod NeoEsp8266BitBangWs2812NoIntrMethod;
typedef NeoEsp8266BitBangWs2812xMethod NeoEsp8266BitBangWs2816NoIntrMethod;
typedef NeoEsp8266BitBangTm1814Method NeoEsp8266BitBangTm1914NoIntrMethod;
typedef NeoEsp8266BitBangSk6812Method NeoEsp8266BitBangLc8812NoIntrMethod;

typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2811, NeoEspInverted, false> NeoEsp8266BitBangWs2811InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2812x, NeoEspInverted, false> NeoEsp8266BitBangWs2812xInvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedWs2805, NeoEspInverted, false> NeoEsp8266BitBangWs2805InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedSk6812, NeoEspInverted, false> NeoEsp8266BitBangSk6812InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1814, NeoEspNotInverted, false> NeoEsp8266BitBangTm1814InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedTm1829, NeoEspNotInverted, false> NeoEsp8266BitBangTm1829InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed800Kbps, NeoEspInverted, false> NeoEsp8266BitBang800KbpsInvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeed400Kbps, NeoEspInverted, false> NeoEsp8266BitBang400KbpsInvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedApa106, NeoEspInverted, false> NeoEsp8266BitBangApa106InvertedNoIntrMethod;
typedef NeoEspBitBangMethodBase<NeoEspBitBangSpeedIntertek, NeoEspInverted, false> NeoEsp8266BitBangIntertekInvertedNoIntrMethod;

typedef NeoEsp8266BitBangWs2812xInvertedMethod NeoEsp8266BitBangWs2813InvertedNoIntrMethod;
typedef NeoEsp8266BitBang800KbpsInvertedMethod NeoEsp8266BitBangWs2812InvertedNoIntrMethod;
typedef NeoEsp8266BitBangWs2812xInvertedMethod NeoEsp8266BitBangWs2816InvertedNoIntrMethod;
typedef NeoEsp8266BitBangTm1814InvertedMethod NeoEsp8266BitBangTm1914InvertedNoIntrMethod;
typedef NeoEsp8266BitBangSk6812InvertedMethod NeoEsp8266BitBangLc8812InvertedNoIntrMethod;

#endif // defined(ARDUINO_ARCH_ESP32)

// ESP bitbang doesn't have defaults and should avoided except for testing

#endif // defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
