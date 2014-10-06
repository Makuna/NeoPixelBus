/*--------------------------------------------------------------------
This file is a modification of the Adafruit NeoPixel library.

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
--------------------------------------------------------------------*/

#ifndef NEOPIXELBUS_H
#define NEOPIXELBUS_H

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#include <pins_arduino.h>
#endif

#include "RgbColor.h"

// '_flagsPixels' flags for LED _pixels (third parameter to constructor):
#define NEO_GRB     0x01 // Wired for GRB data order
#define NEO_COLMASK 0x01
#define NEO_KHZ800  0x02 // 800 KHz datastream
#define NEO_SPDMASK 0x02
// Trinket flash space is tight, v1 NeoPixels aren't handled by default.
// Remove the ifndef/endif to add support -- but code will be bigger.
// Conversely, can comment out the #defines to save space on other MCUs.
#ifndef __AVR_ATtiny85__
#define NEO_RGB     0x00 // Wired for RGB data order
#define NEO_KHZ400  0x00 // 400 KHz datastream
#endif


class NeoPixelBus 
{
public:
    // Constructor: number of LEDs, pin number, LED type
    NeoPixelBus(uint16_t n, uint8_t p = 6, uint8_t t = NEO_GRB + NEO_KHZ800);
    ~NeoPixelBus();

    void Begin(void);
    void Show(void);

    uint8_t*  Pixels() const
    {
        return _pixels;
    };
    uint16_t  PixelCount(void) const
    {
        return _countPixels;
    };

    void SetPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    void SetPixelColor(uint16_t n, RgbColor c)
    {
        SetPixelColor(n, c.R, c.G, c.B);
    };

    RgbColor GetPixelColor(uint16_t n) const;

    void StartAnimating();
    void UpdateAnimations();

    bool IsAnimating() const
    {
        return _activeAnimations > 0;
    }
    void LinearFadePixelColor(uint16_t time, uint16_t n, RgbColor color);

private:
    void setPin(uint8_t p);

    const uint16_t    _countPixels;       // Number of RGB LEDs in strip
    const uint16_t    _sizePixels;      // Size of '_pixels' buffer below

#if defined(NEO_RGB) || defined(NEO_KHZ400)
    const uint8_t _flagsPixels;          // Pixel flags (400 vs 800 KHz, RGB vs GRB color)
#endif
    uint8_t _pin;           // Output pin number
    uint8_t* _pixels;        // Holds LED color values (3 bytes each)
    uint32_t _endTime;       // Latch timing reference
#ifdef __AVR__
    const volatile uint8_t* _port;         // Output PORT register
    uint8_t _pinMask;       // Output PORT bitmask
#endif

    struct FadeAnimation
    {
        uint16_t time;
        uint16_t remaining;

        RgbColor target;
        RgbColor origin;
    };

    uint16_t _activeAnimations;
    FadeAnimation* _animations;
    uint32_t _animationLastTick;

};

#endif // NEOPIXELBUS_H
