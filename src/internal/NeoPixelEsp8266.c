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

#ifdef ARDUINO_ARCH_ESP8266

#include <Arduino.h>
#include <eagle_soc.h>

void ICACHE_RAM_ATTR esp8266_uart1_send_pixels(uint8_t* pixels, uint8_t* end)
{
    const uint8_t _uartData[4] = { 0b00110111, 0b00000111, 0b00110100, 0b00000100 };
    const uint8_t _uartFifoTrigger = 124; // tx fifo should be 128 bytes. minus the four we need to send

    do
    {
        uint8_t subpix = *pixels++;
        uint8_t buf[4] = { _uartData[(subpix >> 6) & 3], 
            _uartData[(subpix >> 4) & 3], 
            _uartData[(subpix >> 2) & 3], 
            _uartData[subpix & 3] };

        // now wait till this the FIFO buffer has room to send more
        while (((U1S >> USTXC) & 0xff) > _uartFifoTrigger);

        for (uint8_t i = 0; i < 4; i++)
        {
            // directly write the byte to transfer into the UART1 FIFO register
            U1F = buf[i];
        }

    } while (pixels < end);
}

inline uint32_t _getCycleCount()
{
    uint32_t ccount;
    __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
    return ccount;
}

#define CYCLES_800_T0H  (F_CPU / 2500000) // 0.4us
#define CYCLES_800_T1H  (F_CPU / 1250000) // 0.8us
#define CYCLES_800      (F_CPU /  800000) // 1.25us per bit
#define CYCLES_400_T0H  (F_CPU / 2000000)
#define CYCLES_400_T1H  (F_CPU /  833333)
#define CYCLES_400      (F_CPU /  400000) 

void ICACHE_RAM_ATTR bitbang_send_pixels_800(uint8_t* pixels, uint8_t* end, uint8_t pin)
{
    const uint32_t pinRegister = _BV(pin);
    uint8_t mask;
    uint8_t subpix;
    uint32_t cyclesStart;

    // trigger emediately
    cyclesStart = _getCycleCount() - CYCLES_800;
    do
    {
        subpix = *pixels++;
        for (mask = 0x80; mask != 0; mask >>= 1)
        {
            // do the checks here while we are waiting on time to pass
            uint32_t cyclesBit = ((subpix & mask)) ? CYCLES_800_T1H : CYCLES_800_T0H;
            uint32_t cyclesNext = cyclesStart;
            uint32_t delta;

            // after we have done as much work as needed for this next bit
            // now wait for the HIGH
            do
            {
                // cache and use this count so we don't incur another 
                // instruction before we turn the bit high
                cyclesStart = _getCycleCount();
            } while ((cyclesStart - cyclesNext) < CYCLES_800);

            // set high
            GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinRegister);

            // wait for the LOW
            do
            {
                cyclesNext = _getCycleCount();
            } while ((cyclesNext - cyclesStart) < cyclesBit);

            // set low
            GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinRegister);
        }
    } while (pixels < end);
}

void ICACHE_RAM_ATTR bitbang_send_pixels_400(uint8_t* pixels, uint8_t* end, uint8_t pin)
{
    const uint32_t pinRegister = _BV(pin);
    uint8_t mask;
    uint8_t subpix;
    uint32_t cyclesStart;

    // trigger emediately
    cyclesStart = _getCycleCount() - CYCLES_400;
    do
    {
        subpix = *pixels++;
        for (mask = 0x80; mask; mask >>= 1)
        {
            uint32_t cyclesBit = ((subpix & mask)) ? CYCLES_400_T1H : CYCLES_400_T0H;
            uint32_t cyclesNext = cyclesStart;

            // after we have done as much work as needed for this next bit
            // now wait for the HIGH
            do
            {
                // cache and use this count so we don't incur another 
                // instruction before we turn the bit high
                cyclesStart = _getCycleCount();
            } while ((cyclesStart - cyclesNext) < CYCLES_400);

            // set high
            GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinRegister);

            // wait for the LOW
            do
            {
                cyclesNext = _getCycleCount();
            } while ((cyclesNext - cyclesStart) < cyclesBit);

            // set low
            GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinRegister);
        }
    } while (pixels < end);
}

#endif
