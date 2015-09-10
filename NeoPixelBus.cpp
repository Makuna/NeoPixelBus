/*-------------------------------------------------------------------------
Arduino library to control a wide variety of WS2811- and WS2812-based RGB
LED devices such as Adafruit FLORA RGB Smart Pixels and NeoPixel strips.
Currently handles 400 and 800 KHz bitstreams on 8, 12 and 16 MHz ATmega
MCUs, with LEDs wired for RGB or GRB color order.  8 MHz MCUs provide
output on PORTB and PORTD, while 16 MHz chips can handle most output pins
(possible exception with upper PORT registers on the Arduino Mega).

Originally written by Phil Burgess / Paint Your Dragon for Adafruit Industries,
contributions by PJRC and other members of the open source community.

Modified for esp8266 by Michael C. Miller (makuna)
Modified further for hardware uart by sticilface (sticilface) Shelby Merrick (forkineye)

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing products
from Adafruit!

-------------------------------------------------------------------------
NeoPixel is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

NeoPixel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

extern "C" 
{
#include "eagle_soc.h"
#include "uart_register.h"
}

#include "NeoPixelBus.h"

#define UART_INV_MASK  (0x3f<<19)  
#define UART 1

NeoPixelBus::NeoPixelBus(uint16_t n, uint8_t p, uint8_t t) : 
    _countPixels(n), 
    _sizePixels(n * 3), 
    _flagsPixels(t)
{
    _pixels = (uint8_t *)malloc(_sizePixels);
    if (_pixels) 
    {
        memset(_pixels, 0, _sizePixels);
    }
}

NeoPixelBus::~NeoPixelBus() 
{
    if (_pixels) 
        free(_pixels);
}

void NeoPixelBus::Begin(void) 
{
    /* Serial rate is 4x 800KHz for WS2811 */

    uint32_t baud = 3200000; // 800 Support
#ifdef INCLUDE_NEO_KHZ400_SUPPORT

    if ((_flagsPixels & NEO_SPDMASK) == NEO_KHZ400)
    {
        // 400 Support
        buad = 1600000;
    }

#endif

    Serial1.begin(baud, SERIAL_6N1, SERIAL_TX_ONLY);

    CLEAR_PERI_REG_MASK(UART_CONF0(UART), UART_INV_MASK);
    //SET_PERI_REG_MASK(UART_CONF0(UART), UART_TXD_INV);
    SET_PERI_REG_MASK(UART_CONF0(UART), (BIT(22)));

    Dirty();
}

void NeoPixelBus::Show(void)
{
    if (!_pixels) 
        return;
    if (!IsDirty())
        return;

    // Data latch = 50+ microsecond pause in the output stream.  Rather than
    // put a delay at the end of the function, the ending time is noted and
    // the function will simply hold off (if needed) on issuing the
    // subsequent round of data until the latch time has elapsed.  This
    // allows the mainline code to start generating the next frame of data
    // rather than stalling for the latch.
    while (!CanShow())
    {
        delay(0); // allows for system yield if needed
    }
    // _endTime is a private member (rather than global var) so that mutliple
    // instances on different pins can be quickly issued in succession (each
    // instance doesn't delay the next).

    // esp hardware uart sending of data
    char buff[4];
    uint8_t* p = _pixels;
    uint8_t* end = p + _sizePixels;


    do
    {
        uint8_t subpix = *p++;

        buff[0] = _uartData[(subpix >> 6) & 3];
        buff[1] = _uartData[(subpix >> 4) & 3];
        buff[2] = _uartData[(subpix >> 2) & 3];
        buff[3] = _uartData[subpix & 3];
        Serial1.write(buff, sizeof(buff));

    } while (p < end);

    ResetDirty();
    _endTime = micros(); // Save EOD time for latch on next call
}


// Set pixel color from separate R,G,B components:
void NeoPixelBus::SetPixelColor(
    uint16_t n, 
    uint8_t r, 
    uint8_t g, 
    uint8_t b) 
{
    if (n < _countPixels) 
    {
        UpdatePixelColor(n, r, g, b);
    }
}

void NeoPixelBus::ClearTo(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint16_t n = 0; n < _countPixels; n++)
    {
        UpdatePixelColor(n, r, g, b);
    }
}

// Set pixel color from separate R,G,B components:
void NeoPixelBus::UpdatePixelColor(
    uint16_t n, 
    uint8_t r, 
    uint8_t g, 
    uint8_t b) 
{
    Dirty();

    uint8_t *p = &_pixels[n * 3];

    uint8_t colorOrder = (_flagsPixels & NEO_COLMASK);
    if (colorOrder == NEO_GRB)
    {
        *p++ = g;
        *p++ = r;
        *p = b;
    } 
    else if (colorOrder == NEO_RGB)
    {
        *p++ = r;
        *p++ = g;
        *p = b;
    }
    else
    {
        *p++ = b;
        *p++ = r;
        *p = g;
    }
}

// Query color from previously-set pixel (returns packed 32-bit RGB value)
RgbColor NeoPixelBus::GetPixelColor(uint16_t n) const 
{
    if (n < _countPixels) 
    {
        RgbColor c;
        uint8_t *p = &_pixels[n * 3];

        uint8_t colorOrder = (_flagsPixels & NEO_COLMASK);
        if (colorOrder == NEO_GRB)
        {
            c.G = *p++;
            c.R = *p++;
            c.B = *p;
        }
        else if (colorOrder == NEO_RGB)
        {
            c.R = *p++;
            c.G = *p++;
            c.B = *p;
        }
        else 
        {
            c.B = *p++;
            c.R = *p++;
            c.G = *p;
        }
        
        return c;
    }

    return RgbColor(0); // Pixel # is out of bounds
}
