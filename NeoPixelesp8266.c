/*
NeoPixelEsp8266.h - NeoPixel library helper functions for Esp8266 using cycle count
Copyright (c) 2015 Michael C. Miller. All right reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if defined(ESP8266)

#include <Arduino.h>
#include <eagle_soc.h>

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

void ICACHE_RAM_ATTR send_pixels_800(uint8_t* pixels, uint8_t* end, uint8_t pin)
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

    // while accurate, this isn't needed due to the delays at the 
    // top of Show() to enforce between update timing
    // while ((_getCycleCount() - cyclesStart) < CYCLES_800);
}

void ICACHE_RAM_ATTR send_pixels_400(uint8_t* pixels, uint8_t* end, uint8_t pin)
{
    const uint32_t pinRegister = _BV(pin);
    uint8_t mask;
    uint8_t subpix;
    uint32_t cyclesStart;

    // trigger emediately
    cyclesStart = _getCycleCount() - CYCLES_400;
    while (pixels < end)
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
    }

    // while accurate, this isn't needed due to the delays at the 
    // top of Show() to enforce between update timing
    // while ((_getCycleCount() - cyclesStart) < CYCLES_400);
}

#endif