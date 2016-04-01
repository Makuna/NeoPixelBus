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

enum NeoDmaState
{
    NeoDmaState_Idle,
    NeoDmaState_Pending,
    NeoDmaState_Sending,
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

        s_this = this;
    }

    ~NeoEsp8266DmaMethodBase()
    {
        StopDma();

        free(_pixels);
        free(_i2sBlock);
    }

    bool IsReadyToUpdate() const
    {
        return (_dmaState == NeoDmaState_Idle);
    }

    void Initialize()
    {
        _dmaState = NeoDmaState_Idle;

        // prepare linked DMA descriptors, having EOF set only for the data
        // primary data item
        _i2sBufDescData.owner = 1;
        _i2sBufDescData.eof = 1;
        _i2sBufDescData.sub_sof = 0;
        _i2sBufDescData.datalen = sizeof(_i2sZeroes); // will get modified in ISR
        _i2sBufDescData.blocksize = sizeof(_i2sZeroes); // will get modified in ISR
        _i2sBufDescData.buf_ptr = (uint32_t)_i2sZeroes; // will get modified in ISR
        _i2sBufDescData.unused = 0;
        _i2sBufDescData.next_link_ptr = (uint32_t)&_i2sBufDescLatch;
 
        // this zero-buffer block helps implements the latch/reset signal 
        // and gives the ISR time to modify buf_ptr in _i2sBufDescData
        _i2sBufDescLatch.owner = 1;
        _i2sBufDescLatch.eof = 0; // no need to trigger interrupt
        _i2sBufDescLatch.sub_sof = 0;
        _i2sBufDescLatch.datalen = sizeof(_i2sZeroes);
        _i2sBufDescLatch.blocksize = sizeof(_i2sZeroes);
        _i2sBufDescLatch.buf_ptr = (uint32_t)_i2sZeroes;
        _i2sBufDescLatch.unused = 0;
        _i2sBufDescLatch.next_link_ptr = (uint32_t)&_i2sBufDescData;
        

        ETS_SLC_INTR_DISABLE();
        SLCC0 |= SLCRXLR | SLCTXLR;
        SLCC0 &= ~(SLCRXLR | SLCTXLR);
        SLCIC = 0xFFFFFFFF;

        // Configure DMA
        SLCC0 &= ~(SLCMM << SLCM); // clear DMA MODE
        SLCC0 |= (1 << SLCM); // set DMA MODE to 1
        SLCRXDC |= SLCBINR | SLCBTNR; // enable INFOR_NO_REPLACE and TOKEN_NO_REPLACE
        SLCRXDC &= ~(SLCBRXFE | SLCBRXEM | SLCBRXFM); // disable RX_FILL, RX_EOF_MODE and RX_FILL_MODE

        // Feed DMA the 1st buffer desc addr
        // To send data to the I2S subsystem, counter-intuitively we use the RXLINK part, not the TXLINK as you might
        // expect. The TXLINK part still needs a valid DMA descriptor, even if it's unused: the DMA engine will throw
        // an error at us otherwise. Just feed it any random descriptor.
        SLCTXL &= ~(SLCTXLAM << SLCTXLA); // clear TX descriptor address
        SLCTXL |= (uint32)&_i2sBufDescLatch << SLCTXLA; // set TX descriptor address. any random desc is OK, we don't use TX but it needs to be valid
        SLCRXL &= ~(SLCRXLAM << SLCRXLA); // clear RX descriptor address
        SLCRXL |= (uint32)&_i2sBufDescData << SLCRXLA; // set RX descriptor address

        ETS_SLC_INTR_ATTACH(i2s_slc_isr, NULL);
        SLCIE = SLCIRXEOF; // Enable only for RX EOF interrupt

        ETS_SLC_INTR_ENABLE();

        //Start transmission
        SLCTXL |= SLCTXLS;
        SLCRXL |= SLCRXLS;

        pinMode(3, FUNCTION_1); // I2S0_DATA

        I2S_CLK_ENABLE();
        I2SIC = 0x3F;
        I2SIE = 0;

        //Reset I2S
        I2SC &= ~(I2SRST);
        I2SC |= I2SRST;
        I2SC &= ~(I2SRST);

        I2SFC &= ~(I2SDE | (I2STXFMM << I2STXFM) | (I2SRXFMM << I2SRXFM)); // Set RX/TX FIFO_MOD=0 and disable DMA (FIFO only)
        I2SFC |= I2SDE; //Enable DMA
        I2SCC &= ~((I2STXCMM << I2STXCM) | (I2SRXCMM << I2SRXCM)); // Set RX/TX CHAN_MOD=0

        // set the rate
        uint32_t i2s_clock_div = T_SPEED::I2sClockDivisor & I2SCDM;
        uint8_t i2s_bck_div = T_SPEED::I2sBaseClockDivisor & I2SBDM;

        //!trans master, !bits mod, rece slave mod, rece msb shift, right first, msb right
        I2SC &= ~(I2STSM | (I2SBMM << I2SBM) | (I2SBDM << I2SBD) | (I2SCDM << I2SCD));
        I2SC |= I2SRF | I2SMR | I2SRSM | I2SRMS | (i2s_bck_div << I2SBD) | (i2s_clock_div << I2SCD);

        I2SC |= I2STXS; // Start transmission
    }

    void ICACHE_RAM_ATTR Update()
    {
        // wait for not actively sending data
        while (_dmaState != NeoDmaState_Idle)
        {
            yield();
        }
        FillBuffers();

        // toggle state so the ISR reacts
        _dmaState = NeoDmaState_Pending;
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
    static NeoEsp8266DmaMethodBase* s_this; // for the ISR

    size_t    _sizePixels;    // Size of '_pixels' buffer below
    uint8_t*  _pixels;        // Holds LED color values

    struct slc_queue_item _i2sBufDescData;
    struct slc_queue_item _i2sBufDescLatch;

    uint32_t _bitBufferSize;
    uint8_t* _i2sBlock;
    // normally 24 bytes creates the minimum 50us latch per spec but
    // with the new logic, this latch is used to space between three states
    uint8_t _i2sZeroes[8]; 

    volatile NeoDmaState _dmaState;

    // This routine is called as soon as the DMA routine has something to tell us. All we
    // handle here is the RX_EOF_INT status, which indicate the DMA has sent a buffer whose
    // descriptor has the 'EOF' field set to 1.
    volatile static void ICACHE_RAM_ATTR i2s_slc_isr(void)
    {
        uint32_t slc_intr_status = SLCIS;

        SLCIC = 0xFFFFFFFF;

        if (slc_intr_status & SLCIRXEOF)
        {
            ETS_SLC_INTR_DISABLE();

            slc_queue_item* finished_item = (slc_queue_item*)SLCRXEDA;

            if (finished_item == &(s_this->_i2sBufDescData))
            {
                switch (s_this->_dmaState)
                {
                case NeoDmaState_Idle:
                    break;

                case NeoDmaState_Pending:
                    // data block has pending data waiting to send, prepare it
                    finished_item->datalen = s_this->_bitBufferSize;
                    finished_item->blocksize = s_this->_bitBufferSize;
                    finished_item->buf_ptr = (uint32_t)(s_this->_i2sBlock);

                    s_this->_dmaState = NeoDmaState_Sending;
                    break;

                case NeoDmaState_Sending:
                    // the data block had actual data, clear it
                    finished_item->datalen = sizeof(s_this->_i2sZeroes);
                    finished_item->blocksize = sizeof(s_this->_i2sZeroes);
                    finished_item->buf_ptr = (uint32_t)(s_this->_i2sZeroes);

                    s_this->_dmaState = NeoDmaState_Idle;
                    break;
                }
            }

            ETS_SLC_INTR_ENABLE();
        }
    }

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
        ETS_SLC_INTR_DISABLE();
        SLCIC = 0xFFFFFFFF;
        SLCIE = 0;
        SLCTXL &= ~(SLCTXLAM << SLCTXLA); // clear TX descriptor address
        SLCRXL &= ~(SLCRXLAM << SLCRXLA); // clear RX descriptor address

        pinMode(3, INPUT);
    }
};

template<typename T_SPEED> 
NeoEsp8266DmaMethodBase<T_SPEED>* NeoEsp8266DmaMethodBase<T_SPEED>::s_this;

typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeed800Kbps> NeoEsp8266Dma800KbpsMethod;
typedef NeoEsp8266DmaMethodBase<NeoEsp8266DmaSpeed400Kbps> NeoEsp8266Dma400KbpsMethod;

// Dma  method is the default method for Esp8266
typedef NeoEsp8266Dma800KbpsMethod Neo800KbpsMethod;
typedef NeoEsp8266Dma400KbpsMethod Neo400KbpsMethod;

#endif