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
#pragma once

#include <Arduino.h>

enum ColorType
{
    ColorType_Rgb,
    ColorType_Hsl
};

#include "RgbColor.h"
#include "HslColor.h"
#include "HsbColor.h"
#include "NeoPixelAnimator.h"

// '_flagsPixels' flags for LED _pixels (third parameter to constructor):
#define NEO_RGB     0x00 // Wired for RGB data order
#define NEO_GRB     0x01 // Wired for GRB data order
#define NEO_BRG     0x04
#define NEO_COLMASK 0x05

#define NEO_KHZ400  0x10 // 400 KHz datastream
#define NEO_KHZ800  0x00 // 800 KHz datastream (default)
#define NEO_SPDMASK 0x10 // which bits are used for speed flags

#define NEO_SYNC    0x20 // wait for vsync before updating buffers



// '_flagsState' flags for internal state
#define NEO_EXTMEMORY 0x01 // external memory, don't deallocate it
#define NEO_STARTED 0x40 // flag to know that the transmission has started
#define NEO_DIRTY   0x80 // a change was made it _pixels that requires a show


struct slc_queue_item {
  uint32  blocksize:12;
  uint32  datalen:12;
  uint32  unused:5;
  uint32  sub_sof:1;
  uint32  eof:1;
  uint32  owner:1;
  uint32  buf_ptr;
  uint32  next_link_ptr;
};

class NeoPixelBus
{
public:
    // Constructor: number of LEDs, pin number, LED type
    // NOTE:  Pin Number is ignored in this version due to use of hardware I2S. hardwired to GPIO3 due to hardware restrictions.
    // but it is left in the argument list for easy switching between versions of the library
    
    NeoPixelBus(uint16_t n, uint8_t p, uint8_t t, uint8_t* pixelBuf, uint8_t* bitBuf);
    NeoPixelBus(uint16_t n, uint8_t p, uint8_t t = NEO_GRB | NEO_KHZ800);
    ~NeoPixelBus();

    inline uint16_t getPixelCount()
    {
        return _countPixels;
    }

    void Begin();
    void Show();
    inline bool CanShow(void) const
    { 
        // the I2s model is async, so latching isn't interesting
        return true;
    };

    void ClearTo(uint8_t r, uint8_t g, uint8_t b);
    void ClearTo(RgbColor c)
    {
        ClearTo(c.R, c.G, c.B);
    };

    bool IsExternalMemory() const
    {
        return (_flagsState & NEO_EXTMEMORY);
    };

    bool IsStarted() const
    {
        return (_flagsState & NEO_STARTED);
    };

    bool IsDirty() const
    {
        return  (_flagsState & NEO_DIRTY);
    };

    void Dirty()
    {
        _flagsState |= NEO_DIRTY;
    };

    void ResetDirty()
    {
        _flagsState &= ~NEO_DIRTY;
    };

    bool Is400mhzPixels() const
    {
        return ((_flagsPixels & NEO_SPDMASK) == NEO_KHZ400);
    };

    bool IsSyncWithOutput() const
    {
        return  (_flagsPixels & NEO_SYNC);
    };

    uint8_t* Pixels() const
    {
        return _pixels;
    };

    uint16_t PixelCount() const
    {
        return _countPixels;
    };

    void SetPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    void SetPixelColor(uint16_t n, RgbColor c)
    {
        SetPixelColor(n, c.R, c.G, c.B);
    };

    RgbColor GetPixelColor(uint16_t n) const;
    void SyncWait(void);
    void FillBuffers(void);

    /* 24 bit per LED, 3 I2S bits per databit, and of that the byte count */
    static uint32_t CalculateI2sBufferSize(uint16_t pixelCount)
    {
        return ((uint32_t)pixelCount * 24 * 4 + 7) / 8;
    }

private:

    struct slc_queue_item _i2sBufDescOut;
    struct slc_queue_item _i2sBufDescLatch;

    uint32_t _bitBufferSize;
    uint8_t _i2sZeroes[24]; // 24 buytes creates the minimum 50us latch per spec
    uint8_t* _i2sBlock;

    friend NeoPixelAnimator;

    void UpdatePixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
    void UpdatePixelColor(uint16_t n, RgbColor c)
    {
        UpdatePixelColor(n, c.R, c.G, c.B);
    };

    void Started()
    {
        _flagsState |= NEO_STARTED;
    };

    void ExternalMemory()
    {
        _flagsState |= NEO_EXTMEMORY;
    }

    const uint32_t _usPixelTime800mhz = 30; // us it takes to send a single pixel at 800mhz speed
#ifdef INCLUDE_NEO_KHZ400_SUPPORT
    const uint32_t _usPixelTime400mhz = 60; // us it takes to send a single pixel at 400mhz speed
#endif
    const uint16_t    _countPixels;     // Number of RGB LEDs in strip
    const uint16_t    _sizePixels;      // Size of '_pixels' buffer below
    
    uint8_t _flagsPixels;    // Pixel flags (400 vs 800 KHz, RGB vs GRB color)
    uint8_t _flagsState;     // internal state
    uint8_t* _pixels;        // Holds LED color values (3 bytes each)
};

