/*-------------------------------------------------------------------------
NeoPixel library helper functions for ARM that support a cycle count

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

#if defined(ARDUINO_ARCH_ARM)

#define CYCLES_LOOPTEST   (4) // adjustment due to loop exit test instruction cycles

class NeoArmCcSpeedWs2811
{
public:
    const static uint32_t T0H = (F_CPU / 3333333 - CYCLES_LOOPTEST); // 0.3us
    const static uint32_t T1H = (F_CPU / 1052632 - CYCLES_LOOPTEST); // 0.95us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit
};

class NeoArmCcSpeedTm1814
{
public:
    const static uint32_t T0H = (F_CPU / 2916666 - CYCLES_LOOPTEST); // 0.35us
    const static uint32_t T1H = (F_CPU / 1666666 - CYCLES_LOOPTEST); // 0.75us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit
};

class NeoArmCcSpeedTm1829
{
public:
    const static uint32_t T0H = (F_CPU / 3333333 - CYCLES_LOOPTEST); // 0.3us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit
};

class NeoArmCcSpeed800Mhz
{
public:
    const static uint32_t T0H = (F_CPU / 2500000 - CYCLES_LOOPTEST); // 0.4us
    const static uint32_t T1H = (F_CPU / 1250000 - CYCLES_LOOPTEST); // 0.8us
    const static uint32_t Period = (F_CPU / 800000 - CYCLES_LOOPTEST); // 1.25us per bit
};

class NeoArmCcSpeed400Mhz
{
public:
    const static uint32_t T0H = (F_CPU / 2000000 - CYCLES_LOOPTEST); 
    const static uint32_t T1H = (F_CPU /  833333 - CYCLES_LOOPTEST); 
    const static uint32_t Period = (F_CPU / 400000 - CYCLES_LOOPTEST);
};

class NeoArmCcSpeedApa106
{
public:
    const static uint32_t T0H = (F_CPU / 2857143 - CYCLES_LOOPTEST); // 0.35us
    const static uint32_t T1H = (F_CPU / 740741 - CYCLES_LOOPTEST); // 1.35
    const static uint32_t Period = (F_CPU / 606061 - CYCLES_LOOPTEST); // 1.65us
};

class NeoArmCcPinset
{
public:
    const static uint8_t IdleLevel = LOW;

    inline static void setPin(const uint8_t pin)
    {
        digitalWrite(pin, HIGH);
    }

    inline static void resetPin(const uint8_t pin)
    {
        digitalWrite(pin, LOW);
    }
};

class NeoArmCcPinsetInverted
{
public:
    const static uint8_t IdleLevel = HIGH;

    inline static void setPin(const uint8_t pin)
    {
        digitalWrite(pin, LOW);
    }

    inline static void resetPin(const uint8_t pin)
    {
        digitalWrite(pin, HIGH);
    }
};

template<typename T_SPEED, typename T_PINSET> class NeoArmCcBitBangBase
{
public:
    static void send_pixels(uint8_t* pixels, uint8_t* end, uint8_t pin)
    {
        uint8_t mask = 0x80;
        uint8_t subpix = *pixels++;
        uint32_t cyclesStart = 0; // trigger emediately
        uint32_t cyclesNext = 0;

        startCycleCount();

        for (;;)
        {
            // do the checks here while we are waiting on time to pass
            uint32_t cyclesBit = T_SPEED::T0H;
            if (subpix & mask)
            {
                cyclesBit = T_SPEED::T1H;
            }

            // after we have done as much work as needed for this next bit
            // now wait for the HIGH
            while (((cyclesStart = getCycleCount()) - cyclesNext) < T_SPEED::Period);

            // set pin state
            T_PINSET::setPin(pin);

            // wait for the LOW
            while ((getCycleCount() - cyclesStart) < cyclesBit);

            // reset pin start
            T_PINSET::resetPin(pin);

            cyclesNext = cyclesStart;

            // next bit
            mask >>= 1;
            if (mask == 0)
            {
                // no more bits to send in this byte
                // check for another byte
                if (pixels >= end)
                {
                    // no more bytes to send so stop
                    break;
                }
                // reset mask to first bit and get the next byte
                mask = 0x80;
                subpix = *pixels++;
            }
        }

        stopCycleCount();
    }

protected:
    static inline uint32_t getCycleCount(void)
    {
        return *((volatile uint32_t*)0xE0001004);
    }

    static inline void startCycleCount()
    {
        // init DWT feature
        (*((volatile uint32_t*)0xE000EDFC)) |= 0x01000000;
        // init DWT Count to zero
        *((volatile uint32_t*)0xE0001004) = 0;
        // start DWT
        (*(volatile uint32_t*)0xe0001000) |= 0x40000001;
    }

    static inline void stopCycleCount()
    {
        // stop DWT
        (*(volatile uint32_t*)0xe0001000) &= ~0x00000001;
    }
};

class NeoArmCcBitBangSpeedWs2811 : public NeoArmCcBitBangBase<NeoArmCcSpeedWs2811, NeoArmCcPinset>
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoArmCcBitBangSpeedWs2812x : public NeoArmCcBitBangBase<NeoArmCcSpeed800Mhz, NeoArmCcPinset>
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoArmCcBitBangSpeedSk6812 : public NeoArmCcBitBangBase<NeoArmCcSpeed800Mhz, NeoArmCcPinset>
{
public:
    static const uint32_t ResetTimeUs = 80;
};

// normal is inverted signal
class NeoArmCcBitBangSpeedTm1814 : public NeoArmCcBitBangBase<NeoArmCcSpeedTm1814, NeoArmCcPinsetInverted>
{
public:
    static const uint32_t ResetTimeUs = 200;
};

// normal is inverted signal
class NeoArmCcBitBangSpeedTm1829 : public NeoArmCcBitBangBase<NeoArmCcSpeedTm1829, NeoArmCcPinsetInverted>
{
public:
    static const uint32_t ResetTimeUs = 200;
};

class NeoArmCcBitBangSpeed800Kbps : public NeoArmCcBitBangBase<NeoArmCcSpeed800Mhz, NeoArmCcPinset>
{
public:
    static const uint32_t ResetTimeUs = 50; 
};

class NeoArmCcBitBangSpeed400Kbps : public NeoArmCcBitBangBase<NeoArmCcSpeed400Mhz, NeoArmCcPinset>
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoArmCcBitBangSpeedApa106 : public NeoArmCcBitBangBase<NeoArmCcSpeedApa106, NeoArmCcPinset>
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoArmCcBitBangInvertedSpeedWs2811 : public NeoArmCcBitBangBase<NeoArmCcSpeedWs2811, NeoArmCcPinsetInverted>
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoArmCcBitBangInvertedSpeedWs2812x : public NeoArmCcBitBangBase<NeoArmCcSpeed800Mhz, NeoArmCcPinsetInverted>
{
public:
    static const uint32_t ResetTimeUs = 300;
};

class NeoArmCcBitBangInvertedSpeedSk6812 : public NeoArmCcBitBangBase<NeoArmCcSpeed800Mhz, NeoArmCcPinsetInverted>
{
public:
    static const uint32_t ResetTimeUs = 80;
};

// normal is inverted signal, so inverted is normal
class NeoArmCcBitBangInvertedSpeedTm1814 : public NeoArmCcBitBangBase<NeoArmCcSpeedTm1814, NeoArmCcPinset>
{
public:
    static const uint32_t ResetTimeUs = 200;
};

// normal is inverted signal, so inverted is normal
class NeoArmCcBitBangInvertedSpeedTm1829 : public NeoArmCcBitBangBase<NeoArmCcSpeedTm1829, NeoArmCcPinset>
{
public:
    static const uint32_t ResetTimeUs = 200;
};

class NeoArmCcBitBangInvertedSpeed800Kbps : public NeoArmCcBitBangBase<NeoArmCcSpeed800Mhz, NeoArmCcPinsetInverted>
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoArmCcBitBangInvertedSpeed400Kbps : public NeoArmCcBitBangBase<NeoArmCcSpeed400Mhz, NeoArmCcPinsetInverted>
{
public:
    static const uint32_t ResetTimeUs = 50;
};

class NeoArmCcBitBangInvertedSpeedApa106 : public NeoArmCcBitBangBase<NeoArmCcSpeedApa106, NeoArmCcPinsetInverted>
{
public:
    static const uint32_t ResetTimeUs = 50;
};

template<typename T_SPEED, typename T_PINSET> class NeoArmCcBitBangMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoArmCcBitBangMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin)
    {
        pinMode(pin, OUTPUT);

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin()
    }

    ~NeoArmCcBitBangMethodBase()
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
        digitalWrite(_pin, T_PINSET::IdleLevel);

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
        noInterrupts(); 

        T_SPEED::send_pixels(_data, _data + _sizeData, _pin);

        interrupts();

        // save EOD time for latch on next call
        _endTime = micros();
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
    }

private:
    const size_t  _sizeData;   // Size of '_data' buffer below
    const uint8_t _pin;            // output pin number

    uint32_t _endTime;       // Latch timing reference
    uint8_t* _data;        // Holds LED color values
};

typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangSpeedWs2811, NeoArmCcPinset> NeoArmBitBangWs2811Method;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangSpeedWs2812x, NeoArmCcPinset> NeoArmBitBangWs2812xMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangSpeedSk6812, NeoArmCcPinset> NeoArmBitBangSk6812Method;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangSpeedTm1814, NeoArmCcPinsetInverted> NeoArmBitBangTm1814Method;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangSpeedTm1829, NeoArmCcPinsetInverted> NeoArmBitBangTm1829Method;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangSpeed800Kbps, NeoArmCcPinset> NeoArmBitBang800KbpsMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangSpeed400Kbps, NeoArmCcPinset> NeoArmBitBang400KbpsMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangSpeedApa106, NeoArmCcPinset> NeoArmBitBangApa106Method;

typedef NeoArmBitBangWs2812xMethod NeoArmBitBangWs2813Method;
typedef NeoArmBitBang800KbpsMethod NeoArmBitBangWs2812Method;
typedef NeoArmBitBangSk6812Method NeoArmBitBangLc8812Method;

typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangInvertedSpeedWs2811, NeoArmCcPinsetInverted> NeoArmBitBangWs2811InvertedMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangInvertedSpeedWs2812x, NeoArmCcPinsetInverted> NeoArmBitBangWs2812xInvertedMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangInvertedSpeedSk6812, NeoArmCcPinsetInverted> NeoArmBitBangSk6812InvertedMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangInvertedSpeedTm1814, NeoArmCcPinset> NeoArmBitBangTm1814InvertedMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangInvertedSpeedTm1829, NeoArmCcPinset> NeoArmBitBangTm1829InvertedMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangInvertedSpeed800Kbps, NeoArmCcPinsetInverted> NeoArmBitBang800KbpsInvertedMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangInvertedSpeed400Kbps, NeoArmCcPinsetInverted> NeoArmBitBang400KbpsInvertedMethod;
typedef NeoArmCcBitBangMethodBase<NeoArmCcBitBangInvertedSpeedApa106, NeoArmCcPinsetInverted> NeoArmBitBangApa106InvertedMethod;

typedef NeoArmBitBangWs2812xInvertedMethod NeoArmBitBangWs2813InvertedMethod;
typedef NeoArmBitBang800KbpsInvertedMethod NeoArmBitBangWs2812InvertedMethod;
typedef NeoArmBitBangSk6812InvertedMethod NeoArmBitBangLc8812InvertedMethod;

#endif
