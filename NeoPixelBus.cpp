
// TODO: check license compatibility - or rewrite

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


/* Do not use interrupts if you e.g. want to use SPIFFS also. The rom calls will crash if the interrupt code gets executed. */
//#define USE_2812_INTERRUPTS

#define ABS(x) (((x)>0)?(x):(-(x)))

#include "NeoPixelBus.h"

extern "C"
{
#include "Arduino.h"
#include "osapi.h"
#include "ets_sys.h"

#include "i2s_reg.h"
#include "i2s.h"
#include "eagle_soc.h"
#include "esp8266_peri.h"
#include "slc_register.h"

#include "osapi.h"
#include "ets_sys.h"
#include "user_interface.h"

/* ugly.. where to get the real prototype from? */
extern void rom_i2c_writeReg_Mask(uint32_t block, uint32_t host_id, uint32_t reg_add, uint32_t Msb, uint32_t Lsb, uint32_t indata);
}

#ifdef USE_2812_INTERRUPTS
extern "C"
{
    /* it seems this interrupts is causing trouble, get an exception occasionally */
    static void NeoPixelBus_DmaInterrupt(NeoPixelBus *bus) 
    {
        WRITE_PERI_REG(SLC_INT_CLR, SLC_RX_EOF_INT_CLR);
    }
}
#endif

/* TODO: constructor if you dont want to use malloc but a static buffer. dont delete this object as it would call free */
NeoPixelBus::NeoPixelBus(uint16_t n, uint8_t p, uint8_t t, uint8_t *pixelBuf, uint8_t *bitBuf) : 
    _countPixels(n), 
    _sizePixels(n * 3), 
    _flagsPixels(t)
{
    /* 24 bit per LED, 3 I2S bits per databit, and of that the byte count */
    bitBufferSize = (_countPixels * 24 * 4 + 7) / 8;
    
    _pixels = pixelBuf;
    i2sBlock = bitBuf;
    
    if(_pixels)
    {
        memset(_pixels, 0, _sizePixels);
    }
}

NeoPixelBus::NeoPixelBus(uint16_t n, uint8_t p, uint8_t t) : 
    _countPixels(n), 
    _sizePixels(n * 3), 
    _flagsPixels(t)
{
    /* 24 bit per LED, 3 I2S bits per databit, and of that the byte count */
    bitBufferSize = (_countPixels * 24 * 4 + 7) / 8;
    
    _pixels = (uint8_t *)malloc(_sizePixels);
    i2sBlock = (uint8_t *)malloc(bitBufferSize);
    
    if(_pixels)
    {
        memset(_pixels, 0, _sizePixels);
    }
}

NeoPixelBus::~NeoPixelBus() 
{
    if (_pixels) 
        free(_pixels);
    if (i2sBlock) 
        free(i2sBlock);
}

void NeoPixelBus::Begin(void) 
{
    if(!_pixels || !i2sBlock)
    {
        return;
    }
    
    memset(i2sZeroes, 0x00, sizeof(i2sZeroes));
    memset(i2sBlock, 0x00, bitBufferSize);
    
    /* first populate bit buffers with data */
    FillBuffers();
    
	/* reset DMA registers */
	SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);
	CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);

	/* clear all interrupt flags */
	SET_PERI_REG_MASK(SLC_INT_CLR, 0xffffffff);

	/* set up DMA */
	CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_CONF0, (1<<SLC_MODE_S));
	SET_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_INFOR_NO_REPLACE|SLC_TOKEN_NO_REPLACE);
	CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN|SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);

    /* prepare linked DMA descriptors, having EOF set for all */
	i2sBufDescOut.owner = 1;
	i2sBufDescOut.eof = 1;
	i2sBufDescOut.sub_sof = 0;
	i2sBufDescOut.datalen = bitBufferSize;
	i2sBufDescOut.blocksize = bitBufferSize;
	i2sBufDescOut.buf_ptr = (uint32_t)i2sBlock;
	i2sBufDescOut.unused = 0;
	i2sBufDescOut.next_link_ptr = (uint32_t)&i2sBufDescZeroes;

    /* this zero-buffer block implements the reset signal */
	i2sBufDescZeroes.owner = 1;
	i2sBufDescZeroes.eof = 1;
	i2sBufDescZeroes.sub_sof = 0;
	i2sBufDescZeroes.datalen = sizeof(i2sZeroes);
	i2sBufDescZeroes.blocksize = sizeof(i2sZeroes);
	i2sBufDescZeroes.buf_ptr = (uint32_t)i2sZeroes;
	i2sBufDescZeroes.unused = 0;
	i2sBufDescZeroes.next_link_ptr = (uint32_t)&i2sBufDescOut;
    
    /* configure the first descriptor. TX_LINK isnt used, but has to be configured */
	CLEAR_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_DESCADDR_MASK);
	CLEAR_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&i2sBufDescOut) & SLC_RXLINK_DESCADDR_MASK);
	SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&i2sBufDescOut) & SLC_TXLINK_DESCADDR_MASK);

    /* we dont need interrupts */
    ETS_SLC_INTR_DISABLE();
	WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);
    
#ifdef USE_2812_INTERRUPTS
    /* ouh, user wants them. enable. on your own risk! */
	WRITE_PERI_REG(SLC_INT_ENA, SLC_RX_EOF_INT_ENA);
    ETS_SLC_INTR_ATTACH((int_handler_t)NeoPixelBus_DmaInterrupt, (void *)this);
	ETS_SLC_INTR_ENABLE();
#endif

	/* start RX link */
	SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

	/* configure RDX0/GPIO3 for output. it is the only supported pin unfortunately. */
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);

	/* configure I2S subsystem */
    I2S_CLK_ENABLE();
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
	SET_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);

	/* Select 16bits per channel (FIFO_MOD=0), no DMA access (FIFO only) */
	CLEAR_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN|(I2S_I2S_RX_FIFO_MOD<<I2S_I2S_RX_FIFO_MOD_S)|(I2S_I2S_TX_FIFO_MOD<<I2S_I2S_TX_FIFO_MOD_S));
	/* Enable DMA in i2s subsystem */
	SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);

	/* autodetect best settings */
    uint32_t reqBitRate = (_flagsPixels & NEO_KHZ800) ? 800000 : 400000;
    uint32_t reqFreq = (reqBitRate * 4);
	uint32_t bestClkmDiv = 0;
    uint32_t bestBckDiv = 0;
    uint32_t bestFreq = 0;
    
    /* try with lowest CLKM_DIV */
    for (int clkmDiv = 5; clkmDiv < 128; clkmDiv++)
    {
        /* find lowest BCK_DIV */
        for (int bckDiv = 2; bckDiv < 128; bckDiv++)
        {
            int testFreq = F_CPU / (bckDiv*clkmDiv);
            
            if (ABS(reqFreq-testFreq) < ABS(reqFreq-bestFreq))
            {
                bestFreq = testFreq;
                bestClkmDiv = clkmDiv;
                bestBckDiv = bckDiv;
            }
		}
	}
    
    /* configure the rates */
	CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD|
						(I2S_BITS_MOD<<I2S_BITS_MOD_S)|
						(I2S_BCK_DIV_NUM <<I2S_BCK_DIV_NUM_S)|
						(I2S_CLKM_DIV_NUM<<I2S_CLKM_DIV_NUM_S));
	SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST|I2S_MSB_RIGHT|I2S_RECE_SLAVE_MOD|
						I2S_RECE_MSB_SHIFT|I2S_TRANS_MSB_SHIFT|
						(((bestBckDiv-1)&I2S_BCK_DIV_NUM )<<I2S_BCK_DIV_NUM_S)|
						(((bestClkmDiv-1)&I2S_CLKM_DIV_NUM)<<I2S_CLKM_DIV_NUM_S));

    /* disable all interrupts */
	SET_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR|I2S_I2S_PUT_DATA_INT_CLR|I2S_I2S_TAKE_DATA_INT_CLR);
    
	/* fire the macine */
	SET_PERI_REG_MASK(I2SCONF, I2S_I2S_TX_START);
    
    /* set internal flag so we know that DMA is up and running */
    started = true;
    
    Dirty();
}

void NeoPixelBus::SyncWait(void)
{
    if(!started)
    {
        return;
    }
    
    /* poll for SLC_RX_EOF_DES_ADDR getting set to the buffer with pixel data */
    WRITE_PERI_REG(SLC_RX_EOF_DES_ADDR, 0);
    
	while(READ_PERI_REG(SLC_RX_EOF_DES_ADDR) != (uint32_t)&i2sBufDescOut)
    {
    }
    
    /* okay right now we are somewhere in the blank "reset" section */
}

void NeoPixelBus::FillBuffers(void)
{
    const uint16_t bitpatterns[16] =
    {
        0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
        0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
        0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
        0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
    };

    if (!_pixels || !i2sBlock)
    {
        return;
    }
    
    /* only wait for a done transmission before updating, if user configured it */
    if(_flagsPixels & NEO_SYNC)
    {
        SyncWait();
    }
    
    /* now it is transferring blank area, so it is safe to update buffers */
	uint16_t *ptr = (uint16_t*)i2sBlock;

	for (int pixel = 0; pixel < _sizePixels; pixel++ )
	{
		uint8_t dataByte = _pixels[pixel];
		*(ptr++) = bitpatterns[(dataByte & 0x0f)];
		*(ptr++) = bitpatterns[(dataByte >> 4) & 0x0f];
	}
}

void NeoPixelBus::Show(void)
{
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
    
    /* transfer pixel data over to I2S bit buffers */
    FillBuffers();
    
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
