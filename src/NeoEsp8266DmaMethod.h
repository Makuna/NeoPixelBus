/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp8266.


Written by Michael C. Miller.
Thanks to g3gg0.de for porting the initial DMA support.
Thanks to github/cnlohr for the original work on DMA support, which is
located at https://github.com/cnlohr/esp8266ws2812i2s.

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

    void rom_i2c_writeReg_Mask(uint32_t block, uint32_t host_id, uint32_t reg_add, uint32_t Msb, uint32_t Lsb, uint32_t indata);
}

struct slc_queue_item
{
    uint32  blocksize : 12;
    uint32  datalen : 12;
    uint32  unused : 5;
    uint32  sub_sof : 1;
    uint32  eof : 1;
    uint32  owner : 1;
    uint32  buf_ptr;
    uint32  next_link_ptr;
};

class NeoEsp8266DmaSpeed800Kbps
{
public:
    const static uint32_t I2sClockDivisor = 3; 
    const static uint32_t I2sBaseClockDivisor = 16;
};

class NeoEsp8266DmaSpeed400Kbps
{
public:
    const static uint32_t I2sClockDivisor = 6; 
    const static uint32_t I2sBaseClockDivisor = 16;
};

template<typename T_SPEED> class NeoEsp8266DmaMethodBase
{
public:
    NeoEsp8266DmaMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize) 
    {
        _sizePixels = pixelCount * elementSize;
        _bitBufferSize = CalculateI2sBufferSize(pixelCount, elementSize);

        _pixels = (uint8_t*)malloc(_sizePixels);
        memset(_pixels, 0x00, _sizePixels);

        _i2sBlock = (uint8_t*)malloc(_bitBufferSize);
        memset(_i2sBlock, 0x00, _bitBufferSize);

        memset(_i2sZeroes, 0x00, sizeof(_i2sZeroes));
    }

    ~NeoEsp8266DmaMethodBase()
    {
        free(_pixels);
        free(_i2sBlock);
    }

    bool IsReadyToUpdate() const
    {
        return IsStoppedDmaState();
    }

    void Initialize()
    {
        InitializeDma();
    }

    void InitializeDma()
    {
        // reset DMA registers 
        SET_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);
        CLEAR_PERI_REG_MASK(SLC_CONF0, SLC_RXLINK_RST);

        // clear all interrupt flags 
        SET_PERI_REG_MASK(SLC_INT_CLR, 0xffffffff);

        // set up DMA 
        CLEAR_PERI_REG_MASK(SLC_CONF0, (SLC_MODE << SLC_MODE_S));
        SET_PERI_REG_MASK(SLC_CONF0, (1 << SLC_MODE_S));
        SET_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_INFOR_NO_REPLACE | SLC_TOKEN_NO_REPLACE);
        CLEAR_PERI_REG_MASK(SLC_RX_DSCR_CONF, SLC_RX_FILL_EN | SLC_RX_EOF_MODE | SLC_RX_FILL_MODE);

        // prepare linked DMA descriptors, having EOF set for all 
        _i2sBufDescOut.owner = 1;
        _i2sBufDescOut.eof = 1;
        _i2sBufDescOut.sub_sof = 0;
        _i2sBufDescOut.datalen = _bitBufferSize;
        _i2sBufDescOut.blocksize = _bitBufferSize;
        _i2sBufDescOut.buf_ptr = (uint32_t)_i2sBlock;
        _i2sBufDescOut.unused = 0;
        _i2sBufDescOut.next_link_ptr = (uint32_t)&_i2sBufDescLatch;

        // this zero-buffer block implements the latch/reset signal 
        _i2sBufDescLatch.owner = 1;
        _i2sBufDescLatch.eof = 1;
        _i2sBufDescLatch.sub_sof = 0;
        _i2sBufDescLatch.datalen = sizeof(_i2sZeroes);
        _i2sBufDescLatch.blocksize = sizeof(_i2sZeroes);
        _i2sBufDescLatch.buf_ptr = (uint32_t)_i2sZeroes;
        _i2sBufDescLatch.unused = 0;
        _i2sBufDescLatch.next_link_ptr = (uint32_t)&_i2sBufDescStopped;

        // this empty block will stop the output and provide a flag that latch/reset has been sent
        // it basically loops
        _i2sBufDescStopped.owner = 1;
        _i2sBufDescStopped.eof = 1;
        _i2sBufDescStopped.sub_sof = 0;
        _i2sBufDescStopped.datalen = sizeof(_i2sZeroes);
        _i2sBufDescStopped.blocksize = sizeof(_i2sZeroes);
        _i2sBufDescStopped.buf_ptr = (uint32_t)_i2sZeroes;;
        _i2sBufDescStopped.unused = 0;
        _i2sBufDescStopped.next_link_ptr = (uint32_t)&_i2sBufDescStopped;

        // configure the first descriptor
        // TX_LINK isnt used, but has to be configured 
        CLEAR_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_DESCADDR_MASK);
        CLEAR_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_DESCADDR_MASK);
        SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&_i2sBufDescOut) & SLC_RXLINK_DESCADDR_MASK);
        SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&_i2sBufDescOut) & SLC_TXLINK_DESCADDR_MASK);

        // we dont need interrupts 
        ETS_SLC_INTR_DISABLE();
        WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);

        // configure RDX0/GPIO3 for output. it is the only supported pin unfortunately. 
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_I2SO_DATA);

        // configure I2S subsystem 
        I2S_CLK_ENABLE();
        CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
        SET_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);
        CLEAR_PERI_REG_MASK(I2SCONF, I2S_I2S_RESET_MASK);

        // Select 16bits per channel (FIFO_MOD=0), no DMA access (FIFO only) 
        CLEAR_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN | 
                (I2S_I2S_RX_FIFO_MOD << I2S_I2S_RX_FIFO_MOD_S) | 
                (I2S_I2S_TX_FIFO_MOD << I2S_I2S_TX_FIFO_MOD_S));
        // Enable DMA in i2s subsystem 
        SET_PERI_REG_MASK(I2S_FIFO_CONF, I2S_I2S_DSCR_EN);
        
        // configure the rates 
        CLEAR_PERI_REG_MASK(I2SCONF, I2S_TRANS_SLAVE_MOD |
                (I2S_BITS_MOD << I2S_BITS_MOD_S) |
                (I2S_BCK_DIV_NUM << I2S_BCK_DIV_NUM_S) |
                (I2S_CLKM_DIV_NUM << I2S_CLKM_DIV_NUM_S));
        SET_PERI_REG_MASK(I2SCONF, I2S_RIGHT_FIRST | I2S_MSB_RIGHT | I2S_RECE_SLAVE_MOD |
                I2S_RECE_MSB_SHIFT | I2S_TRANS_MSB_SHIFT |
                ((T_SPEED::I2sBaseClockDivisor & I2S_BCK_DIV_NUM) << I2S_BCK_DIV_NUM_S) |
                ((T_SPEED::I2sClockDivisor & I2S_CLKM_DIV_NUM) << I2S_CLKM_DIV_NUM_S));

        // disable all interrupts 
        SET_PERI_REG_MASK(I2SINT_CLR, I2S_I2S_RX_WFULL_INT_CLR | 
                I2S_I2S_PUT_DATA_INT_CLR | 
                I2S_I2S_TAKE_DATA_INT_CLR);

        // dma is now ready to start
    }

    void Update()
    {
        FillBuffers();

        // wait for latch to complete if it hasn't already
        NullWait();

        StopDma();
        StartDma();
    }

    uint8_t* getPixels() const
    {
        return _pixels;
    };

    size_t getPixelsSize() const
    {
        return _sizePixels;
    }

private:
    
    static uint32_t CalculateI2sBufferSize(uint16_t pixelCount, size_t elementSize)
    {
        // 4 I2S bytes per pixels byte 
        return ((uint32_t)pixelCount * elementSize * 4);
    }

    void FillBuffers()
    {
        const uint16_t bitpatterns[16] =
        {
            0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
            0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
            0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
            0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
        };

        // wait for the data to be done transmission before updating, 
        // it may still be sending the latch/reset though, buts OK
        SyncWait();

        // now it is transferring blank area, so it is safe to update dma buffers 
        uint16_t* pDma = (uint16_t*)_i2sBlock;
        uint8_t* pPixelsEnd = _pixels + _sizePixels;
        for (uint8_t* pPixel = _pixels; pPixel < pPixelsEnd; pPixel++)
        {
            *(pDma++) = bitpatterns[((*pPixel) & 0x0f)];
            *(pDma++) = bitpatterns[((*pPixel) >> 4) & 0x0f];
        }
    }

    void StopDma()
    {
        SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_STOP);
    }

    void StartDma()
    {
        // clear all interrupt flags 
        SET_PERI_REG_MASK(SLC_INT_CLR, 0xffffffff);

        // configure the first descriptor
        // TX_LINK isnt used, but has to be configured 
        CLEAR_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_DESCADDR_MASK);
        CLEAR_PERI_REG_MASK(SLC_TX_LINK, SLC_TXLINK_DESCADDR_MASK);
        SET_PERI_REG_MASK(SLC_RX_LINK, ((uint32)&_i2sBufDescOut) & SLC_RXLINK_DESCADDR_MASK);
        SET_PERI_REG_MASK(SLC_TX_LINK, ((uint32)&_i2sBufDescOut) & SLC_TXLINK_DESCADDR_MASK);

        WRITE_PERI_REG(SLC_INT_CLR, 0xffffffff);

        // start RX link 
        SET_PERI_REG_MASK(SLC_RX_LINK, SLC_RXLINK_START);

        // fire the machine again
        SET_PERI_REG_MASK(I2SCONF, I2S_I2S_TX_START);
    }

    void NullWait() const
    {
        while (!IsStoppedDmaState())
        {
            // due to how long the data send could be (300 pixels is 9ms)
            // we will yield while we wait
            yield();
        }

        // okay right now we are not sending anything 
    }

    void SyncWait() const
    {
        // poll for SLC_RX_EOF_DES_ADDR getting set to anything other than 
        // the buffer with pixel data 

        while (READ_PERI_REG(SLC_RX_EOF_DES_ADDR) == (uint32_t)&_i2sBufDescLatch)
        {
            WRITE_PERI_REG(SLC_RX_EOF_DES_ADDR, 0xffffffff);
            // due to how long the data send could be (300 pixels is 9ms)
            // we will yield while we wait
            yield();
        }

        // okay right now we are somewhere in the blank "latch/reset" section or
        // stopped
    }

    bool IsStoppedDmaState() const
    {
        uint32_t state = READ_PERI_REG(SLC_RX_EOF_DES_ADDR);
        return (state == 0 || state == (uint32_t)&_i2sBufDescStopped);
    }

    size_t    _sizePixels;      // Size of '_pixels' buffer below
    uint8_t* _pixels;        // Holds LED color values

    struct slc_queue_item _i2sBufDescOut;
    struct slc_queue_item _i2sBufDescLatch;
    struct slc_queue_item _i2sBufDescStopped;

    uint32_t _bitBufferSize;
    uint8_t* _i2sBlock;
    uint8_t _i2sZeroes[24]; // 24 bytes creates the minimum 50us latch per spec
};

typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeed800Kbps> NeoEsp8266Dma800KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeed400Kbps> NeoEsp8266Dma400KbpsMethod;

#endif