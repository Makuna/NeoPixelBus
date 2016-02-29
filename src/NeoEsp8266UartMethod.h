/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp8266.

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

#ifdef ARDUINO_ARCH_ESP8266

extern "C"
{
#include "eagle_soc.h"
#include "uart_register.h"
}

// due to linker overriding ICACHE_RAM_ATTR for cpp files, this function was
// moved into a NeoPixelEsp8266.c file.
extern "C" void ICACHE_RAM_ATTR esp8266_uart1_send_pixels(uint8_t* pixels, uint8_t* end);

class NeoEsp8266UartSpeed800Kbps
{
public:
    static const uint32_t ByteSendTimeUs =  10; // us it takes to send a single pixel element at 800mhz speed
    static const uint32_t UartBaud = 3200000; // 800mhz, 4 serial bytes per NeoByte
};

class NeoEsp8266UartSpeed400Kbps
{
public:
    static const uint32_t ByteSendTimeUs = 20; // us it takes to send a single pixel element at 400mhz speed
    static const uint32_t UartBaud = 1600000; // 400mhz, 4 serial bytes per NeoByte
};

#define UART1 1
#define UART1_INV_MASK (0x3f << 19)

template<typename T_SPEED> class NeoEsp8266UartMethodBase
{
public:
    NeoEsp8266UartMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize)
    {
        _sizePixels = pixelCount * elementSize;
        _pixels = (uint8_t*)malloc(_sizePixels);
        memset(_pixels, 0x00, _sizePixels);
    }

    ~NeoEsp8266UartMethodBase()
    {
        free(_pixels);
    }

    bool IsReadyToUpdate() const
    {
        uint32_t delta = micros() - _endTime;

        return (delta >= 50L && delta <= (4294967296L - getPixelTime()));
    }

    void Initialize()
    {
        Serial1.begin(T_SPEED::UartBaud, SERIAL_6N1, SERIAL_TX_ONLY);

        CLEAR_PERI_REG_MASK(UART_CONF0(UART1), UART1_INV_MASK);
        SET_PERI_REG_MASK(UART_CONF0(UART1), (BIT(22)));

        _endTime = micros();
    }

    void Update()
    {
        // Data latch = 50+ microsecond pause in the output stream.  Rather than
        // put a delay at the end of the function, the ending time is noted and
        // the function will simply hold off (if needed) on issuing the
        // subsequent round of data until the latch time has elapsed.  This
        // allows the mainline code to start generating the next frame of data
        // rather than stalling for the latch.
        
        while (!IsReadyToUpdate())
        {
            yield();
        }
        
        // since uart is async buffer send, we have to calc the endtime that it will take
        // to correctly manage the data latch in the above code
        // add the calculated time to the current time 
        _endTime = micros() + getPixelTime();

        // esp hardware uart sending of data
        esp8266_uart1_send_pixels(_pixels, _pixels + _sizePixels);
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
    uint32_t getPixelTime() const
    {
        return (T_SPEED::ByteSendTimeUs * _sizePixels);
    };

    size_t    _sizePixels;   // Size of '_pixels' buffer below
    uint8_t* _pixels;        // Holds LED color values
    uint32_t _endTime;       // Latch timing reference
};

typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed800Kbps> NeoEsp8266Uart800KbpsMethod;
typedef NeoEsp8266UartMethodBase<NeoEsp8266UartSpeed400Kbps> NeoEsp8266Uart400KbpsMethod;

#endif