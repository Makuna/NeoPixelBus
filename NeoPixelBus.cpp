/*-------------------------------------------------------------------------
Arduino library to control a wide variety of WS2811- and WS2812-based RGB
LED devices such as Adafruit FLORA RGB Smart Pixels and NeoPixel strips.
Currently handles 400 and 800 KHz bitstreams on 8, 12 and 16 MHz ATmega
MCUs, with LEDs wired for RGB or GRB color order.  8 MHz MCUs provide
output on PORTB and PORTD, while 16 MHz chips can handle most output pins
(possible exception with upper PORT registers on the Arduino Mega).

Originally written by Phil Burgess / Paint Your Dragon for Adafruit Industries,
contributions by PJRC and other members of the open source community.

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

#include "NeoPixelBus.h"

#if defined(ESP8266)
// due to linker overriding the ICACHE_RAM_ATTR for cpp files, these methods are
// moved into a C file so the attribute will be applied correctly
extern "C" void ICACHE_RAM_ATTR send_pixels_800(uint8_t* pixels, uint8_t* end, uint8_t pin);
extern "C" void ICACHE_RAM_ATTR send_pixels_400(uint8_t* pixels, uint8_t* end, uint8_t pin);
#endif

NeoPixelBus::NeoPixelBus(uint16_t n, uint8_t p, uint8_t t) : 
    _countPixels(n), 
    _sizePixels(n * 3), 
    _pin(p), 
    _animationLastTick(0),
    _activeAnimations(0),
    _flagsPixels(t)
{
    setPin(p);

    _pixels = (uint8_t *)malloc(_sizePixels);
    if (_pixels) 
    {
        memset(_pixels, 0, _sizePixels);
    }

    uint16_t animationSize = n * sizeof(FadeAnimation);
    _animations = (FadeAnimation*)malloc(animationSize);
    if (_animations)
    {
        memset(_animations, 0, animationSize);
    }
}

NeoPixelBus::~NeoPixelBus() 
{
    if (_pixels) 
        free(_pixels);
    if (_animations) 
        free(_animations);

    pinMode(_pin, INPUT);
}

void NeoPixelBus::Begin(void) 
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);

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

    // In order to make this code runtime-configurable to work with any pin,
    // SBI/CBI instructions are eschewed in favor of full PORT writes via the
    // OUT or ST instructions.  It relies on two facts: that peripheral
    // functions (such as PWM) take precedence on output pins, so our PORT-
    // wide writes won't interfere, and that interrupts are globally disabled
    // while data is being issued to the LEDs, so no other code will be
    // accessing the PORT.  The code takes an initial 'snapshot' of the PORT
    // state, computes 'pin high' and 'pin low' values, and writes these back
    // to the PORT register as needed.

    noInterrupts(); // Need 100% focus on instruction timing

#ifdef __AVR__

    volatile uint16_t
        i   = _sizePixels; // Loop counter
    volatile uint8_t
        *ptr = _pixels,   // Pointer to next byte
        b   = *ptr++,   // Current byte value
        hi,             // PORT w/output bit set high
        lo;             // PORT w/output bit set low

    // Hand-tuned assembly code issues data to the LED drivers at a specific
    // rate.  There's separate code for different CPU speeds (8, 12, 16 MHz)
    // for both the WS2811 (400 KHz) and WS2812 (800 KHz) drivers.  The
    // datastream timing for the LED drivers allows a little wiggle room each
    // way (listed in the datasheets), so the conditions for compiling each
    // case are set up for a range of frequencies rather than just the exact
    // 8, 12 or 16 MHz values, permitting use with some close-but-not-spot-on
    // devices (e.g. 16.5 MHz DigiSpark).  The ranges were arrived at based
    // on the datasheet figures and have not been extensively tested outside
    // the canonical 8/12/16 MHz speeds; there's no guarantee these will work
    // close to the extremes (or possibly they could be pushed further).
    // Keep in mind only one CPU speed case actually gets compiled; the
    // resulting program isn't as massive as it might look from source here.

    // 8 MHz(ish) AVR ---------------------------------------------------------
#if (F_CPU >= 7400000UL) && (F_CPU <= 9500000UL)

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    if ((_flagsPixels & NEO_SPDMASK) == NEO_KHZ800) 
    { 
        // 800 KHz bitstream
#endif

        volatile uint8_t n1, n2 = 0;  // First, next bits out

        // Squeezing an 800 KHz stream out of an 8 MHz chip requires code
        // specific to each PORT register.  At present this is only written
        // to work with pins on PORTD or PORTB, the most likely use case --
        // this covers all the pins on the Adafruit Flora and the bulk of
        // digital pins on the Arduino Pro 8 MHz (keep in mind, this code
        // doesn't even get compiled for 16 MHz boards like the Uno, Mega,
        // Leonardo, etc., so don't bother extending this out of hand).
        // Additional PORTs could be added if you really need them, just
        // duplicate the else and loop and change the PORT.  Each add'l
        // PORT will require about 150(ish) bytes of program space.

        // 10 instruction clocks per bit: HHxxxxxLLL
        // OUT instructions:              ^ ^    ^   (T=0,2,7)

#ifdef PORTD // PORTD isn't present on ATtiny85, etc.

        if (_port == &PORTD) 
        {

            hi = PORTD |  _pinMask;
            lo = PORTD & ~_pinMask;
            n1 = lo;
            if(b & 0x80) n1 = hi;

            // Dirty trick: RJMPs proceeding to the next instruction are used
            // to delay two clock cycles in one instruction word (rather than
            // using two NOPs).  This was necessary in order to squeeze the
            // loop down to exactly 64 words -- the maximum possible for a
            // relative branch.

            asm volatile(
                "headD:"                   "\n\t" // Clk  Pseudocode
                // Bit 7:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
                "out  %[_port] , %[n1]"    "\n\t" // 1    PORT = n1
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 6"        "\n\t" // 1-2  if(b & 0x40)
                "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 6:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
                "out  %[_port] , %[n2]"    "\n\t" // 1    PORT = n2
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 5"        "\n\t" // 1-2  if(b & 0x20)
                "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 5:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
                "out  %[_port] , %[n1]"    "\n\t" // 1    PORT = n1
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 4"        "\n\t" // 1-2  if(b & 0x10)
                "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 4:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
                "out  %[_port] , %[n2]"    "\n\t" // 1    PORT = n2
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 3"        "\n\t" // 1-2  if(b & 0x08)
                "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 3:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
                "out  %[_port] , %[n1]"    "\n\t" // 1    PORT = n1
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 2"        "\n\t" // 1-2  if(b & 0x04)
                "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 2:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
                "out  %[_port] , %[n2]"    "\n\t" // 1    PORT = n2
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 1"        "\n\t" // 1-2  if(b & 0x02)
                "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo
                "rjmp .+0"                "\n\t" // 2    nop nop
                // Bit 1:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n2]   , %[lo]"    "\n\t" // 1    n2   = lo
                "out  %[_port] , %[n1]"    "\n\t" // 1    PORT = n1
                "rjmp .+0"                "\n\t" // 2    nop nop
                "sbrc %[byte] , 0"        "\n\t" // 1-2  if(b & 0x01)
                "mov %[n2]   , %[hi]"    "\n\t" // 0-1   n2 = hi
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo
                "sbiw %[count], 1"        "\n\t" // 2    i-- (don't act on Z flag yet)
                // Bit 0:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi
                "mov  %[n1]   , %[lo]"    "\n\t" // 1    n1   = lo
                "out  %[_port] , %[n2]"    "\n\t" // 1    PORT = n2
                "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++
                "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 0x80)
                "mov %[n1]   , %[hi]"    "\n\t" // 0-1   n1 = hi
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo
                "brne headD"              "\n"   // 2    while(i) (Z flag set above)
                : [byte]  "+r" (b),
                [n1]    "+r" (n1),
                [n2]    "+r" (n2),
                [count] "+w" (i)
                : [_port]   "I" (_SFR_IO_ADDR(PORTD)),
                [ptr]    "e" (ptr),
                [hi]     "r" (hi),
                [lo]     "r" (lo));

        }
        else if (_port == &PORTB) 
        {

#endif // PORTD

            // Same as above, just switched to PORTB and stripped of comments.
            hi = PORTB |  _pinMask;
            lo = PORTB & ~_pinMask;
            n1 = lo;
            if(b & 0x80) n1 = hi;

            asm volatile(
                "headB:"                   "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "mov  %[n2]   , %[lo]"    "\n\t"
                "out  %[_port] , %[n1]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "sbrc %[byte] , 6"        "\n\t"
                "mov %[n2]   , %[hi]"    "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "mov  %[n1]   , %[lo]"    "\n\t"
                "out  %[_port] , %[n2]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "sbrc %[byte] , 5"        "\n\t"
                "mov %[n1]   , %[hi]"    "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "mov  %[n2]   , %[lo]"    "\n\t"
                "out  %[_port] , %[n1]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "sbrc %[byte] , 4"        "\n\t"
                "mov %[n2]   , %[hi]"    "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "mov  %[n1]   , %[lo]"    "\n\t"
                "out  %[_port] , %[n2]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "sbrc %[byte] , 3"        "\n\t"
                "mov %[n1]   , %[hi]"    "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "mov  %[n2]   , %[lo]"    "\n\t"
                "out  %[_port] , %[n1]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "sbrc %[byte] , 2"        "\n\t"
                "mov %[n2]   , %[hi]"    "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "mov  %[n1]   , %[lo]"    "\n\t"
                "out  %[_port] , %[n2]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "sbrc %[byte] , 1"        "\n\t"
                "mov %[n1]   , %[hi]"    "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "mov  %[n2]   , %[lo]"    "\n\t"
                "out  %[_port] , %[n1]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "sbrc %[byte] , 0"        "\n\t"
                "mov %[n2]   , %[hi]"    "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "sbiw %[count], 1"        "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "mov  %[n1]   , %[lo]"    "\n\t"
                "out  %[_port] , %[n2]"    "\n\t"
                "ld   %[byte] , %a[ptr]+" "\n\t"
                "sbrc %[byte] , 7"        "\n\t"
                "mov %[n1]   , %[hi]"    "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "brne headB"              "\n"
                : [byte] "+r" (b), [n1] "+r" (n1), [n2] "+r" (n2), [count] "+w" (i)
                : [_port] "I" (_SFR_IO_ADDR(PORTB)), [ptr] "e" (ptr), [hi] "r" (hi),
                [lo] "r" (lo));

#ifdef PORTD
        }    // endif PORTB
#endif

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    } 
    else 
    { 
        // end 800 KHz, do 400 KHz

        // Timing is more relaxed; unrolling the inner loop for each bit is
        // not necessary.  Still using the peculiar RJMPs as 2X NOPs, not out
        // of need but just to trim the code size down a little.
        // This 400-KHz-datastream-on-8-MHz-CPU code is not quite identical
        // to the 800-on-16 code later -- the hi/lo timing between WS2811 and
        // WS2812 is not simply a 2:1 scale!

        // 20 inst. clocks per bit: HHHHxxxxxxLLLLLLLLLL
        // ST instructions:         ^   ^     ^          (T=0,4,10)

        volatile uint8_t next, bit;

        hi   = *_port |  _pinMask;
        lo   = *_port & ~_pinMask;
        next = lo;
        bit  = 8;

        asm volatile(
            "head20:"                  "\n\t" // Clk  Pseudocode    (T =  0)
            "st   %a[_port], %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
            "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 128)
            "mov  %[next], %[hi]"    "\n\t" // 0-1   next = hi    (T =  4)
            "st   %a[_port], %[next]"  "\n\t" // 2    PORT = next   (T =  6)
            "mov  %[next] , %[lo]"    "\n\t" // 1    next = lo     (T =  7)
            "dec  %[bit]"             "\n\t" // 1    bit--         (T =  8)
            "breq nextbyte20"         "\n\t" // 1-2  if(bit == 0)
            "rol  %[byte]"            "\n\t" // 1    b <<= 1       (T = 10)
            "st   %a[_port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 12)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 14)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 16)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 18)
            "rjmp head20"             "\n\t" // 2    -> head20 (next bit out)
            "nextbyte20:"              "\n\t" //                    (T = 10)
            "st   %a[_port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 12)
            "nop"                     "\n\t" // 1    nop           (T = 13)
            "ldi  %[bit]  , 8"        "\n\t" // 1    bit = 8       (T = 14)
            "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 16)
            "sbiw %[count], 1"        "\n\t" // 2    i--           (T = 18)
            "brne head20"             "\n"   // 2    if(i != 0) -> (next byte)
            : [_port]  "+e" (_port),
            [byte]  "+r" (b),
            [bit]   "+r" (bit),
            [next]  "+r" (next),
            [count] "+w" (i)
            : [hi]    "r" (hi),
            [lo]    "r" (lo),
            [ptr]   "e" (ptr));
    }
#endif

    // 12 MHz(ish) AVR --------------------------------------------------------
#elif (F_CPU >= 11100000UL) && (F_CPU <= 14300000UL)

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    if ((_flagsPixels & NEO_SPDMASK) == NEO_KHZ800) 
    { 
        // 800 KHz bitstream
#endif

        // In the 12 MHz case, an optimized 800 KHz datastream (no dead time
        // between bytes) requires a PORT-specific loop similar to the 8 MHz
        // code (but a little more relaxed in this case).

        // 15 instruction clocks per bit: HHHHxxxxxxLLLLL
        // OUT instructions:              ^   ^     ^     (T=0,4,10)

        volatile uint8_t next;

#ifdef PORTD

        if (_port == &PORTD) 
        {

            hi   = PORTD |  _pinMask;
            lo   = PORTD & ~_pinMask;
            next = lo;
            if(b & 0x80) next = hi;

            // Don't "optimize" the OUT calls into the bitTime subroutine;
            // we're exploiting the RCALL and RET as 3- and 4-cycle NOPs!
            asm volatile(
                "headD:"                   "\n\t" //        (T =  0)
                "out   %[_port], %[hi]"    "\n\t" //        (T =  1)
                "rcall bitTimeD"          "\n\t" // Bit 7  (T = 15)
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 6
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 5
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 4
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 3
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 2
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeD"          "\n\t" // Bit 1
                // Bit 0:
                "out  %[_port] , %[hi]"    "\n\t" // 1    PORT = hi    (T =  1)
                "rjmp .+0"                "\n\t" // 2    nop nop      (T =  3)
                "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++   (T =  5)
                "out  %[_port] , %[next]"  "\n\t" // 1    PORT = next  (T =  6)
                "mov  %[next] , %[lo]"    "\n\t" // 1    next = lo    (T =  7)
                "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 0x80) (T =  8)
                "mov %[next] , %[hi]"    "\n\t" // 0-1    next = hi  (T =  9)
                "nop"                     "\n\t" // 1                 (T = 10)
                "out  %[_port] , %[lo]"    "\n\t" // 1    PORT = lo    (T = 11)
                "sbiw %[count], 1"        "\n\t" // 2    i--          (T = 13)
                "brne headD"              "\n\t" // 2    if(i != 0) -> (next byte)
                "rjmp doneD"             "\n\t"
                "bitTimeD:"               "\n\t" //      nop nop nop     (T =  4)
                "out  %[_port], %[next]"  "\n\t" // 1    PORT = next     (T =  5)
                "mov  %[next], %[lo]"    "\n\t" // 1    next = lo       (T =  6)
                "rol  %[byte]"           "\n\t" // 1    b <<= 1         (T =  7)
                "sbrc %[byte], 7"        "\n\t" // 1-2  if(b & 0x80)    (T =  8)
                "mov %[next], %[hi]"    "\n\t" // 0-1   next = hi      (T =  9)
                "nop"                    "\n\t" // 1                    (T = 10)
                "out  %[_port], %[lo]"    "\n\t" // 1    PORT = lo       (T = 11)
                "ret"                    "\n\t" // 4    nop nop nop nop (T = 15)
                "doneD:"                 "\n"
                : [byte]  "+r" (b),
                [next]  "+r" (next),
                [count] "+w" (i)
                : [_port]   "I" (_SFR_IO_ADDR(PORTD)),
                [ptr]    "e" (ptr),
                [hi]     "r" (hi),
                [lo]     "r" (lo));

        } 
        else if (_port == &PORTB) 
        {

#endif // PORTD

            hi   = PORTB |  _pinMask;
            lo   = PORTB & ~_pinMask;
            next = lo;
            if(b & 0x80) next = hi;

            // Same as above, just set for PORTB & stripped of comments
            asm volatile(
                "headB:"                   "\n\t"
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeB"          "\n\t"
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeB"          "\n\t"
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeB"          "\n\t"
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeB"          "\n\t"
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeB"          "\n\t"
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeB"          "\n\t"
                "out   %[_port], %[hi]"    "\n\t"
                "rcall bitTimeB"          "\n\t"
                "out  %[_port] , %[hi]"    "\n\t"
                "rjmp .+0"                "\n\t"
                "ld   %[byte] , %a[ptr]+" "\n\t"
                "out  %[_port] , %[next]"  "\n\t"
                "mov  %[next] , %[lo]"    "\n\t"
                "sbrc %[byte] , 7"        "\n\t"
                "mov %[next] , %[hi]"    "\n\t"
                "nop"                     "\n\t"
                "out  %[_port] , %[lo]"    "\n\t"
                "sbiw %[count], 1"        "\n\t"
                "brne headB"              "\n\t"
                "rjmp doneB"             "\n\t"
                "bitTimeB:"               "\n\t"
                "out  %[_port], %[next]"  "\n\t"
                "mov  %[next], %[lo]"    "\n\t"
                "rol  %[byte]"           "\n\t"
                "sbrc %[byte], 7"        "\n\t"
                "mov %[next], %[hi]"    "\n\t"
                "nop"                    "\n\t"
                "out  %[_port], %[lo]"    "\n\t"
                "ret"                    "\n\t"
                "doneB:"                 "\n"
                : [byte] "+r" (b), [next] "+r" (next), [count] "+w" (i)
                : [_port] "I" (_SFR_IO_ADDR(PORTB)), [ptr] "e" (ptr), [hi] "r" (hi),
                [lo] "r" (lo));

#ifdef PORTD
        }
#endif

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    } 
    else 
    { 
        // 400 KHz

        // 30 instruction clocks per bit: HHHHHHxxxxxxxxxLLLLLLLLLLLLLLL
        // ST instructions:               ^     ^        ^    (T=0,6,15)

        volatile uint8_t next, bit;

        hi   = *_port |  _pinMask;
        lo   = *_port & ~_pinMask;
        next = lo;
        bit  = 8;

        asm volatile(
            "head30:"                  "\n\t" // Clk  Pseudocode    (T =  0)
            "st   %a[_port], %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
            "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 128)
            "mov  %[next], %[hi]"    "\n\t" // 0-1   next = hi    (T =  4)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T =  6)
            "st   %a[_port], %[next]"  "\n\t" // 2    PORT = next   (T =  8)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 10)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 12)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 14)
            "nop"                     "\n\t" // 1    nop           (T = 15)
            "st   %a[_port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 17)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 19)
            "dec  %[bit]"             "\n\t" // 1    bit--         (T = 20)
            "breq nextbyte30"         "\n\t" // 1-2  if(bit == 0)
            "rol  %[byte]"            "\n\t" // 1    b <<= 1       (T = 22)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 24)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 26)
            "rjmp .+0"                "\n\t" // 2    nop nop       (T = 28)
            "rjmp head30"             "\n\t" // 2    -> head30 (next bit out)
            "nextbyte30:"              "\n\t" //                    (T = 22)
            "nop"                     "\n\t" // 1    nop           (T = 23)
            "ldi  %[bit]  , 8"        "\n\t" // 1    bit = 8       (T = 24)
            "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 26)
            "sbiw %[count], 1"        "\n\t" // 2    i--           (T = 28)
            "brne head30"             "\n"   // 1-2  if(i != 0) -> (next byte)
            : [_port]  "+e" (_port),
            [byte]  "+r" (b),
            [bit]   "+r" (bit),
            [next]  "+r" (next),
            [count] "+w" (i)
            : [hi]     "r" (hi),
            [lo]     "r" (lo),
            [ptr]    "e" (ptr));
    }
#endif

    // 16 MHz(ish) AVR --------------------------------------------------------
#elif (F_CPU >= 15400000UL) && (F_CPU <= 19000000L)

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    if ((_flagsPixels & NEO_SPDMASK) == NEO_KHZ800)
    {
        // 800 KHz bitstream
#endif

        // WS2811 and WS2812 have different hi/lo duty cycles; this is
        // similar but NOT an exact copy of the prior 400-on-8 code.

        // 20 inst. clocks per bit: HHHHHxxxxxxxxLLLLLLL
        // ST instructions:         ^   ^        ^       (T=0,5,13)

        volatile uint8_t next, bit;

        hi = *_port | _pinMask;
        lo = *_port & ~_pinMask;
        next = lo;
        bit = 8;

        asm volatile(
            "head20:"                   "\n\t" // Clk  Pseudocode    (T =  0)
            "st   %a[_port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
            "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
            "mov  %[next], %[hi]"     "\n\t" // 0-1   next = hi    (T =  4)
            "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
            "st   %a[_port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
            "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
            "breq nextbyte20"          "\n\t" // 1-2  if(bit == 0) (from dec above)
            "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
            "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
            "nop"                      "\n\t" // 1    nop           (T = 13)
            "st   %a[_port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
            "nop"                      "\n\t" // 1    nop           (T = 16)
            "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
            "rjmp head20"              "\n\t" // 2    -> head20 (next bit out)
            "nextbyte20:"               "\n\t" //                    (T = 10)
            "ldi  %[bit]  ,  8"        "\n\t" // 1    bit = 8       (T = 11)
            "ld   %[byte] ,  %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 13)
            "st   %a[_port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
            "nop"                      "\n\t" // 1    nop           (T = 16)
            "sbiw %[count], 1"         "\n\t" // 2    i--           (T = 18)
            "brne head20"             "\n"   // 2    if(i != 0) -> (next byte)
            : [_port]  "+e" (_port),
            [byte]  "+r" (b),
            [bit]   "+r" (bit),
            [next]  "+r" (next),
            [count] "+w" (i)
            : [ptr]    "e" (ptr),
            [hi]     "r" (hi),
            [lo]     "r" (lo));
    

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    } 
    else 
    { 
        // 400 KHz

        // The 400 KHz clock on 16 MHz MCU is the most 'relaxed' version.

        // 40 inst. clocks per bit: HHHHHHHHxxxxxxxxxxxxLLLLLLLLLLLLLLLLLLLL
        // ST instructions:         ^       ^           ^         (T=0,8,20)

        volatile uint8_t next, bit;

        hi   = *_port |  _pinMask;
        lo   = *_port & ~_pinMask;
        next = lo;
        bit  = 8;

        asm volatile(
            "head40:"                 "\n\t" // Clk  Pseudocode    (T =  0)
                "st   %a[_port], %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
                "sbrc %[byte] , 7"        "\n\t" // 1-2  if(b & 128)
                "mov  %[next] , %[hi]"    "\n\t" // 0-1   next = hi    (T =  4)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T =  6)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T =  8)
                "st   %a[_port], %[next]"  "\n\t" // 2    PORT = next   (T = 10)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 12)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 14)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 16)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 18)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 20)
                "st   %a[_port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 22)
                "nop"                     "\n\t" // 1    nop           (T = 23)
                "mov  %[next] , %[lo]"    "\n\t" // 1    next = lo     (T = 24)
                "dec  %[bit]"             "\n\t" // 1    bit--         (T = 25)
                "breq nextbyte40"         "\n\t" // 1-2  if(bit == 0)
                "rol  %[byte]"            "\n\t" // 1    b <<= 1       (T = 27)
                "nop"                     "\n\t" // 1    nop           (T = 28)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 30)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 32)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 34)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 36)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 38)
                "rjmp head40"             "\n\t" // 2    -> head40 (next bit out)
            "nextbyte40:"             "\n\t" //                    (T = 27)
                "ldi  %[bit]  , 8"        "\n\t" // 1    bit = 8       (T = 28)
                "ld   %[byte] , %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 30)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 32)
                "st   %a[_port], %[lo]"    "\n\t" // 2    PORT = lo     (T = 34)
                "rjmp .+0"                "\n\t" // 2    nop nop       (T = 36)
                "sbiw %[count], 1"        "\n\t" // 2    i--           (T = 38)
                "brne head40"             "\n"   // 1-2  if(i != 0) -> (next byte)
                : [_port]  "+e" (_port),
                [byte]  "+r" (b),
                [bit]     "+r" (bit),
                [next]   "+r" (next),
                [count]  "+w" (i)
                : [ptr]  "e" (ptr),
                [hi]     "r" (hi),
                [lo]     "r" (lo));
    }
#endif

#else
#error "CPU SPEED NOT SUPPORTED"
#endif

#elif defined(ESP8266)
    
    uint8_t* p = _pixels;
    uint8_t* end = p + _sizePixels;

#ifdef INCLUDE_NEO_KHZ400_SUPPORT


    if ((_flagsPixels & NEO_SPDMASK) == NEO_KHZ800)
    {
#endif
        // 800 KHz bitstream
        send_pixels_800(p, end, _pin);

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    }
    else
    {
        // 400 kHz bitstream
        send_pixels_400(p, end, _pin);
    }
#endif 

#elif defined(__arm__) 


#if defined(__MK20DX128__) || defined(__MK20DX256__) // Teensy 3.0 & 3.1
#define CYCLES_800_T0H  (F_CPU / 4000000) // 0.4us
#define CYCLES_800_T1H  (F_CPU / 1250000) // 0.8us
#define CYCLES_800      (F_CPU /  800000) // 1.25us per bit
#define CYCLES_400_T0H  (F_CPU / 2000000)
#define CYCLES_400_T1H  (F_CPU /  833333)
#define CYCLES_400      (F_CPU /  400000) 

    uint8_t          *p   = _pixels,
                     *end = p + _sizePixels, pix, mask;
    volatile uint8_t *set = portSetRegister(_pin),
                     *clr = portClearRegister(_pin);
    uint32_t          cyc;

    ARM_DEMCR    |= ARM_DEMCR_TRCENA;
    ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    if ((_flagsPixels & NEO_SPDMASK) == NEO_KHZ800) 
    { 
#endif
        // 800 KHz bitstream
        cyc = ARM_DWT_CYCCNT + CYCLES_800;
        while (p < end) 
        {
            pix = *p++;
            for (mask = 0x80; mask; mask >>= 1) 
            {
                while (ARM_DWT_CYCCNT - cyc < CYCLES_800);
                cyc  = ARM_DWT_CYCCNT;
                *set = 1;
                if (pix & mask) 
                {
                    while (ARM_DWT_CYCCNT - cyc < CYCLES_800_T1H);
                } 
                else 
                {
                    while (ARM_DWT_CYCCNT - cyc < CYCLES_800_T0H);
                }
                *clr = 1;
            }
        }
        while (ARM_DWT_CYCCNT - cyc < CYCLES_800);
#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    } 
    else 
    { 
        // 400 kHz bitstream
        cyc = ARM_DWT_CYCCNT + CYCLES_400;
        while (p < end) 
        {
            pix = *p++;
            for(mask = 0x80; mask; mask >>= 1) 
            {
                while (ARM_DWT_CYCCNT - cyc < CYCLES_400);
                cyc  = ARM_DWT_CYCCNT;
                *set = 1;
                if (pix & mask) 
                {
                    while (ARM_DWT_CYCCNT - cyc < CYCLES_400_T1H);
                } 
                else 
                {
                    while (ARM_DWT_CYCCNT - cyc < CYCLES_400_T0H);
                }
                *clr = 1;
            }
        }
        while (ARM_DWT_CYCCNT - cyc < CYCLES_400);
    }
#endif

#elif defined(__MKL26Z64__) // Teensy-LC

#if F_CPU == 48000000
    uint8_t          *p = pixels,
    pix, count, dly,
    bitmask = digitalPinToBitMask(pin);
    volatile uint8_t *reg = portSetRegister(pin);
    uint32_t         num = numBytes;
    asm volatile(
        "L%=_begin:"				"\n\t"
        "ldrb	%[pix], [%[p], #0]"		"\n\t"
        "lsl	%[pix], #24"			"\n\t"
        "movs	%[count], #7"			"\n\t"
        "L%=_loop:"				"\n\t"
        "lsl	%[pix], #1"			"\n\t"
        "bcs	L%=_loop_one"			"\n\t"
        "L%=_loop_zero:"
        "strb	%[bitmask], [%[reg], #0]"	"\n\t"
        "movs	%[dly], #4"			"\n\t"
        "L%=_loop_delay_T0H:"			"\n\t"
        "sub	%[dly], #1"			"\n\t"
        "bne	L%=_loop_delay_T0H"		"\n\t"
        "strb	%[bitmask], [%[reg], #4]"	"\n\t"
        "movs	%[dly], #13"			"\n\t"
        "L%=_loop_delay_T0L:"			"\n\t"
        "sub	%[dly], #1"			"\n\t"
        "bne	L%=_loop_delay_T0L"		"\n\t"
        "b	L%=_next"			"\n\t"
        "L%=_loop_one:"
        "strb	%[bitmask], [%[reg], #0]"	"\n\t"
        "movs	%[dly], #13"			"\n\t"
        "L%=_loop_delay_T1H:"			"\n\t"
        "sub	%[dly], #1"			"\n\t"
        "bne	L%=_loop_delay_T1H"		"\n\t"
        "strb	%[bitmask], [%[reg], #4]"	"\n\t"
        "movs	%[dly], #4"			"\n\t"
        "L%=_loop_delay_T1L:"			"\n\t"
        "sub	%[dly], #1"			"\n\t"
        "bne	L%=_loop_delay_T1L"		"\n\t"
        "nop"					"\n\t"
        "L%=_next:"				"\n\t"
        "sub	%[count], #1"			"\n\t"
        "bne	L%=_loop"			"\n\t"
        "lsl	%[pix], #1"			"\n\t"
        "bcs	L%=_last_one"			"\n\t"
        "L%=_last_zero:"
        "strb	%[bitmask], [%[reg], #0]"	"\n\t"
        "movs	%[dly], #4"			"\n\t"
        "L%=_last_delay_T0H:"			"\n\t"
        "sub	%[dly], #1"			"\n\t"
        "bne	L%=_last_delay_T0H"		"\n\t"
        "strb	%[bitmask], [%[reg], #4]"	"\n\t"
        "movs	%[dly], #10"			"\n\t"
        "L%=_last_delay_T0L:"			"\n\t"
        "sub	%[dly], #1"			"\n\t"
        "bne	L%=_last_delay_T0L"		"\n\t"
        "b	L%=_repeat"			"\n\t"
        "L%=_last_one:"
        "strb	%[bitmask], [%[reg], #0]"	"\n\t"
        "movs	%[dly], #13"			"\n\t"
        "L%=_last_delay_T1H:"			"\n\t"
        "sub	%[dly], #1"			"\n\t"
        "bne	L%=_last_delay_T1H"		"\n\t"
        "strb	%[bitmask], [%[reg], #4]"	"\n\t"
        "movs	%[dly], #1"			"\n\t"
        "L%=_last_delay_T1L:"			"\n\t"
        "sub	%[dly], #1"			"\n\t"
        "bne	L%=_last_delay_T1L"		"\n\t"
        "nop"					"\n\t"
        "L%=_repeat:"				"\n\t"
        "add	%[p], #1"			"\n\t"
        "sub	%[num], #1"			"\n\t"
        "bne	L%=_begin"			"\n\t"
        "L%=_done:"				"\n\t"
        : [p] "+r" (p),
        [pix] "=&r" (pix),
        [count] "=&r" (count),
        [dly] "=&r" (dly),
        [num] "+r" (num)
        : [bitmask] "r" (bitmask),
        [reg] "r" (reg)
        );
#else
#error "Sorry, only 48 MHz is supported, please set Tools > CPU Speed to 48 MHz"
#endif

#else // Arduino Due

#define SCALE      VARIANT_MCK / 2UL / 1000000UL
#define INST       (2UL * F_CPU / VARIANT_MCK)
#define TIME_800_0 ((int)(0.40 * SCALE + 0.5) - (5 * INST))
#define TIME_800_1 ((int)(0.80 * SCALE + 0.5) - (5 * INST))
#define PERIOD_800 ((int)(1.25 * SCALE + 0.5) - (5 * INST))
#define TIME_400_0 ((int)(0.50 * SCALE + 0.5) - (5 * INST))
#define TIME_400_1 ((int)(1.20 * SCALE + 0.5) - (5 * INST))
#define PERIOD_400 ((int)(2.50 * SCALE + 0.5) - (5 * INST))

    int             pinMask, time0, time1, period, t;
    Pio            *port;
    volatile WoReg *portSet, *portClear, *timeValue, *timeReset;
    uint8_t        *p, *end, pix, mask;

    pmc_set_writeprotect(false);
    pmc_enable_periph_clk((uint32_t)TC3_IRQn);
    TC_Configure(TC1, 0,
        TC_CMR_WAVE | TC_CMR_WAVSEL_UP | TC_CMR_TCCLKS_TIMER_CLOCK1);
    TC_Start(TC1, 0);

    pinMask   = g_APinDescription[_pin].ulPin; // Don't 'optimize' these into
    port      = g_APinDescription[_pin].pPort; // declarations above.  Want to
    portSet   = &(port->PIO_SODR);            // burn a few cycles after
    portClear = &(port->PIO_CODR);            // starting timer to minimize
    timeValue = &(TC1->TC_CHANNEL[0].TC_CV);  // the initial 'while'.
    timeReset = &(TC1->TC_CHANNEL[0].TC_CCR);
    p         =  _pixels;
    end       =  p + _sizePixels;
    pix       = *p++;
    mask      = 0x80;

#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    if ((_flagsPixels & NEO_SPDMASK) == NEO_KHZ800) 
    { 
#endif
        // 800 KHz bitstream
        time0 = TIME_800_0;
        time1 = TIME_800_1;
        period = PERIOD_800;
#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    } 
    else 
    { 
        // 400 KHz bitstream
        time0 = TIME_400_0;
        time1 = TIME_400_1;
        period = PERIOD_400;
    }
#endif

    for (t = time0;; t = time0) 
    {
        if (pix & mask) 
            t = time1;
        while (*timeValue < period);
        *portSet   = pinMask;
        *timeReset = TC_CCR_CLKEN | TC_CCR_SWTRG;
        while (*timeValue < t);
        *portClear = pinMask;
        if (!(mask >>= 1)) 
        {   // This 'inside-out' loop logic utilizes
            if (p >= end) 
                break; // idle time to minimize inter-byte delays.
            pix = *p++;
            mask = 0x80;
        }
    }
    while (*timeValue < period); // Wait for last bit
    TC_Stop(TC1, 0);

#endif // end Arduino Due

#endif // end Architecture select

    interrupts();
    ResetDirty();
    _endTime = micros(); // Save EOD time for latch on next call
}


// Set the output pin number
void NeoPixelBus::setPin(uint8_t p) 
{
    pinMode(_pin, INPUT);
    _pin = p;
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
#ifdef __AVR__
    _port    = portOutputRegister(digitalPinToPort(p));
    _pinMask = digitalPinToBitMask(p);
#endif
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
        // clear any animation
        if (_animations[n].time != 0)
        {
            _activeAnimations--;
            _animations[n].time = 0;
            _animations[n].remaining = 0;
        }
        UpdatePixelColor(n, r, g, b);
    }
}

void NeoPixelBus::ClearTo(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t n = 0; n < _countPixels; n++)
    {
        SetPixelColor(n, r, g, b);
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

void NeoPixelBus::LinearFadePixelColor(uint16_t time, uint16_t n, RgbColor color)
{
    if (n >= _countPixels)
    {
        return;
    }

    if (_animations[n].time != 0)
    {
        _activeAnimations--;
    }

    _animations[n].time = time;
    _animations[n].remaining = time;
    _animations[n].target = color;
    _animations[n].origin = GetPixelColor(n);

    if (time > 0)
    {
        _activeAnimations++;
    }
    else
    {
        SetPixelColor(n, _animations[n].target);
    }
}

void NeoPixelBus::FadeTo(uint16_t time, RgbColor color)
{
    for (uint8_t n = 0; n < _countPixels; n++)
    {
        LinearFadePixelColor(time, n, color);
    }
}

void NeoPixelBus::StartAnimating()
{
    _animationLastTick = millis();
}

void NeoPixelBus::UpdateAnimations()
{
    uint32_t currentTick = millis();

    if (_animationLastTick != 0)
    {
        uint32_t delta = currentTick - _animationLastTick;
        if (delta > 0)
        {
            uint16_t countAnimations = _activeAnimations;

            FadeAnimation* pAnim;
            RgbColor color;

            for (uint16_t iAnim = 0; iAnim < _countPixels && countAnimations > 0; iAnim++)
            {
                pAnim = &_animations[iAnim];

                if (pAnim->remaining > delta)
                {
                    pAnim->remaining -= delta;

                    uint8_t progress = (pAnim->time - pAnim->remaining) * (uint32_t)256 / pAnim->time;

                    color = RgbColor::LinearBlend(pAnim->origin, 
                        pAnim->target, 
                        progress);

                    UpdatePixelColor(iAnim, color);
                    countAnimations--;
                }
                else if (pAnim->remaining > 0)
                {
                    // specifically calling SetPixelColor so it will clear animation state
                    SetPixelColor(iAnim, pAnim->target);
                    countAnimations--;
                }
            }
        }
    }

    _animationLastTick = currentTick;
}

