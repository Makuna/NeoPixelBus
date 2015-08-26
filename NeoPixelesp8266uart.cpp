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

#include <Arduino.h>
#include <eagle_soc.h>
#include "uart_register.h"


#if defined(ESP8266)


#define UART_INV_MASK  (0x3f<<19)
#define UART 1



const char data[4] = { 0b00110111, 0b00000111, 0b00110100, 0b00000100 };


void ICACHE_RAM_ATTR send_pixels_UART(uint8_t* pixels, uint8_t* end)
{
    char buff[4];
    
    do
    {
        uint8_t subpix = *pixels++;

        buff[0] = data[(subpix >> 6) & 3];
        buff[1] = data[(subpix >> 4) & 3];
        buff[2] = data[(subpix >> 2) & 3];
        buff[3] = data[subpix & 3];
        Serial1.write(buff, sizeof(buff));    

    } while (pixels < end);

}

#endif