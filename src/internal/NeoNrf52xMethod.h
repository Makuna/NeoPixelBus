/*-------------------------------------------------------------------------
NeoPixel library helper functions for Nrf52* MCUs.
Nano 33 BLE

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/NeoPixelBus)

-------------------------------------------------------------------------
This file is part of the Makuna/NeoPixelBus library.
The contents of this file were taken from the Adafruit NeoPixel library
and modified only to fit within individual calling functions.

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

#if defined(ARDUINO_ARCH_NRF52840)

const uint16_t c_dmaBytesPerPixelByte = 8 * sizeof(uint16_t); // bits * bytes to represent pulse
const uint16_t c_dmaBytesForReset = 2 * sizeof(uint16_t); // two pulses to extend reset

// count 1 = 0.0625us, so max count (32768) is 2048us
class NeoNrf52xPwmSpeedWs2812x
{
public:
    const static uint32_t CountTop = 20UL; // 1.25us
    const static nrf_pwm_values_common_t Bit0 = 6UL | 0x8000; // ~0.4us
    const static nrf_pwm_values_common_t Bit1 = 13UL | 0x8000; // ~0.8us
    const static uint32_t CountReset = 4800; // 300us
    const static PinStatus IdleLevel = LOW;
};

class NeoNrf52xPwmSpeed400Kbps
{
public:
    const static uint32_t CountTop = 40UL; // 2.5us
    const static nrf_pwm_values_common_t Bit0 = 13UL | 0x8000; // ~0.8us
    const static nrf_pwm_values_common_t Bit1 = 26UL | 0x8000; // ~1.6us
    const static uint16_t CountReset = 800; // 50 us
    const static PinStatus IdleLevel = LOW;
};

class NeoNrf52xPwmSpeedWs2811
{
public:
    const static uint32_t CountTop = 20UL; // 1.25us
    const static nrf_pwm_values_common_t Bit0 = 5UL | 0x8000; // ~0.3us
    const static nrf_pwm_values_common_t Bit1 = 14UL | 0x8000; // ~0.9us
    const static uint16_t CountReset = 800; // 50 us
    const static PinStatus IdleLevel = LOW;
};

class NeoNrf52xPwm0
{
public:
    inline static NRF_PWM_Type* Pwm()
    {
        return NRF_PWM0;
    }
};

class NeoNrf52xPwm1
{
public:
    inline static NRF_PWM_Type* Pwm()
    {
        return NRF_PWM1;
    }
};

class NeoNrf52xPwm2
{
public:
    inline static NRF_PWM_Type* Pwm()
    {
        return NRF_PWM2;
    }
};

#if defined(NRF_PWM3)
class NeoNrf52xPwm3
{
public:
    inline static NRF_PWM_Type* Pwm()
    {
        return NRF_PWM3;
    }
};
#endif

template<typename T_SPEED, typename T_BUS> class NeoNrf52xMethodBase
{
public:
    NeoNrf52xMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize) :
        _pin(pin)
    {
        pinMode(pin, OUTPUT);

        _pixelsSize = pixelCount * elementSize;
        _pixels = static_cast<uint8_t*>(malloc(_pixelsSize));
        memset(_pixels, 0, _pixelsSize);

        _dmaBufferSize = c_dmaBytesPerPixelByte * _pixelsSize + c_dmaBytesForReset;
        _dmaBuffer = static_cast<uint8_t*>(malloc(_dmaBufferSize));
        memset(_dmaBuffer, 0, _dmaBufferSize);
    }

    ~NeoNrf52xMethodBase()
    {
        while (!IsReadyToUpdate())
        {
            yield();
        }

        dmaDeinit();

        pinMode(_pin, INPUT);

        free(_pixels);
        free(_dmaBuffer);
    }

    bool IsReadyToUpdate() const
    {
        // return (T_BUS::Pwm()->EVENTS_STOPPED);
        // return (T_BUS::Pwm()->EVENTS_LOOPSDONE);
        
        return (T_BUS::Pwm()->EVENTS_SEQEND[0]);
    }

    void Initialize()
    {
        digitalWrite(_pin, T_SPEED::IdleLevel);

        dmaInit();
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

        FillBuffer();
        
        // start the data send
        T_BUS::Pwm()->EVENTS_SEQEND[0] = 0;
        T_BUS::Pwm()->TASKS_SEQSTART[0] = 1;
    }

    uint8_t* getPixels() const
    {
        return _pixels;
    };

    size_t getPixelsSize() const
    {
        return _pixelsSize;
    };

private:
    const uint8_t _pin;      // output pin number

    size_t   _pixelsSize;    // Size of '_pixels' buffer below
    uint8_t* _pixels;        // Holds LED color values
    uint32_t _dmaBufferSize; // total size of _dmaBuffer
    uint8_t* _dmaBuffer;     // Holds pixel data in native format for PWM hardware

    void dmaInit()
    {
        T_BUS::Pwm()->MODE = NRF_PWM_MODE_UP;
        T_BUS::Pwm()->PRESCALER = NRF_PWM_CLK_16MHz;
        T_BUS::Pwm()->COUNTERTOP = T_SPEED::CountTop;
        T_BUS::Pwm()->LOOP = 0; // single fire
        nrf_pwm_decoder_set(T_BUS::Pwm(), NRF_PWM_LOAD_COMMON, NRF_PWM_STEP_AUTO);
        // T_BUS::Pwm()->EVENTS_SEQEND[0] = 0;
        //nrf_pwm_shorts_enable(T_BUS::Pwm(), NRF_PWM_SHORT_LOOPSDONE_STOP_MASK);

        T_BUS::Pwm()->SEQ[0].PTR = reinterpret_cast<uint32_t>(_dmaBuffer);
        T_BUS::Pwm()->SEQ[0].CNT = _dmaBufferSize / sizeof(uint16_t);
        T_BUS::Pwm()->SEQ[0].REFRESH = 0; // ignored
        T_BUS::Pwm()->SEQ[0].ENDDELAY = T_SPEED::CountReset; // ignored ?
        T_BUS::Pwm()->PSEL.OUT[0] = digitalPinToPinName(_pin);
        T_BUS::Pwm()->ENABLE = 1;
    }

    void dmaDeinit()
    {
        T_BUS::Pwm()->EVENTS_SEQEND[0] = 0;
        T_BUS::Pwm()->ENABLE = 0;
        T_BUS::Pwm()->PSEL.OUT[0] = NC;
    }

    void FillBuffer()
    {
        nrf_pwm_values_common_t* pDma = reinterpret_cast<nrf_pwm_values_common_t*>(_dmaBuffer);
        uint8_t* pPixelsEnd = _pixels + _pixelsSize;
        for (uint8_t* pPixel = _pixels; pPixel < pPixelsEnd; pPixel++)
        {
            uint8_t data = *pPixel;

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                *(pDma++) = (data & 0x80) ? T_SPEED::Bit1 : T_SPEED::Bit0;
                data <<= 1;
            }
        }
        // is this really needed?  The count is already part of the PWM structure
        // so this seems invalid, or does this cause the output signal to clear (no pulse)
        // and thus gets repeated at the end
        *(pDma++) = 0x8000; // end sequence
        *(pDma++) = 0x8000;
    }
};

typedef NeoNrf52xMethodBase<NeoNrf52xPwmSpeedWs2811, NeoNrf52xPwm0> NeoNrf52xPwm0Ws2811Method;
typedef NeoNrf52xMethodBase<NeoNrf52xPwmSpeedWs2812x, NeoNrf52xPwm0> NeoNrf52xPwm0Ws2812xMethod;
typedef NeoNrf52xMethodBase<NeoNrf52xPwmSpeed400Kbps, NeoNrf52xPwm0> NeoNrf52xPwm0400KbpsMethod;


#endif
