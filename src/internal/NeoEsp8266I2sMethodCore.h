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

#pragma once
#ifdef ARDUINO_ARCH_ESP8266

#include "Arduino.h"

extern "C"
{
#include "osapi.h"
#include "ets_sys.h"

#include "i2s_reg.h"

#ifdef ARDUINO_ESP8266_MAJOR    //this define was added in ESP8266 Arduino Core version v3.0.1
#include "core_esp8266_i2s.h" //for Arduino core >= 3.0.1
#else
#include "i2s.h"              //for Arduino core <= 3.0.0
#endif

#include "eagle_soc.h"
#include "esp8266_peri.h"
#include "slc_register.h"

#include "osapi.h"
#include "ets_sys.h"
#include "user_interface.h"

#if !defined(__CORE_ESP8266_VERSION_H) || defined(ARDUINO_ESP8266_RELEASE_2_5_0)
    void rom_i2c_writeReg_Mask(uint32_t block, uint32_t host_id, uint32_t reg_add, uint32_t Msb, uint32_t Lsb, uint32_t indata);
#endif
}

struct slc_queue_item
{
    uint32  blocksize : 12;
    uint32  datalen : 12;
    uint32  unused : 5;
    uint32  sub_sof : 1;
    uint32  eof : 1;
    uint32  owner : 1;
    uint8* buf_ptr;
    struct slc_queue_item* next_link_ptr;
};

enum NeoDmaState
{
    NeoDmaState_Idle,
    NeoDmaState_Pending,
    NeoDmaState_Sending,
    NeoDmaState_Zeroing,
};

const uint16_t c_maxDmaBlockSize = 4095;

const uint8_t c_I2sPin = 3; // due to I2S hardware, the pin used is restricted to this

class NeoEsp8266I2sMethodCore
{
private:
    static const uint8_t c_StateBlockCount = 2;

    // i2s sends 4 byte elements, 
    static const uint16_t c_I2sByteBoundarySize = 4;

protected:
    static NeoEsp8266I2sMethodCore* s_this; // for the ISR

    volatile NeoDmaState _dmaState;

    slc_queue_item* _i2sBufDesc;  // dma block descriptors
    uint16_t _i2sBufDescCount;   // count of block descriptors in _i2sBufDesc

    size_t _i2sBufferSize; // total size of _i2sBuffer
    uint8_t* _i2sBuffer;  // holds the DMA buffer that is referenced by _i2sBufDesc

    size_t _i2sIdleDataTotalSize; // total size of represented zeroes, mulitple uses of _i2sIdleData
    size_t _i2sIdleDataSize; // size of _i2sIdleData
    uint8_t* _i2sIdleData;

    uint16_t _is2BufMaxBlockSize; // max size based on size of a pixel of a single block

    size_t GetSendSize() const
    {
        return _i2sBufferSize + _i2sIdleDataTotalSize;
    }

    // This routine is called as soon as the DMA routine has something to tell us. All we
    // handle here is the RX_EOF_INT status, which indicate the DMA has sent a buffer whose
    // descriptor has the 'EOF' field set to 1.
    // in the case of this code, the second to last state descriptor
    static void IRAM_ATTR i2s_slc_isr(void)
    {
        ETS_SLC_INTR_DISABLE();

        uint32_t slc_intr_status = SLCIS;

        SLCIC = 0xFFFFFFFF;

        if ((slc_intr_status & SLCIRXEOF) && s_this)
        {
            switch (s_this->_dmaState)
            {
            case NeoDmaState_Idle:
                break;

            case NeoDmaState_Pending:
            {
                slc_queue_item* finished_item = (slc_queue_item*)SLCRXEDA;

                // data block has pending data waiting to send, prepare it
                // point last state block to top 
                (finished_item + c_StateBlockCount - 1)->next_link_ptr = s_this->_i2sBufDesc;

                s_this->_dmaState = NeoDmaState_Sending;
            }
            break;

            case NeoDmaState_Sending:
            {
                slc_queue_item* finished_item = (slc_queue_item*)SLCRXEDA;

                // the data block had actual data sent
                // point last state block to first state block thus
                // just looping and not sending the data blocks
                (finished_item + c_StateBlockCount - 1)->next_link_ptr = finished_item;

                s_this->_dmaState = NeoDmaState_Zeroing;
            }
            break;

            case NeoDmaState_Zeroing:
                s_this->_dmaState = NeoDmaState_Idle;
                break;
            }
        }

        ETS_SLC_INTR_ENABLE();
    }

    NeoEsp8266I2sMethodCore() 
    { };

    void AllocateI2s(const size_t i2sBufferSize, // expected multiples of c_I2sByteBoundarySize
            const size_t i2sZeroesSize, // expected multiples of c_I2sByteBoundarySize
            const size_t is2BufMaxBlockSize,
            const uint8_t idleLevel)
    {
        _i2sBufferSize = i2sBufferSize;
        _i2sIdleDataTotalSize = i2sZeroesSize;
        _i2sIdleDataSize = _i2sIdleDataTotalSize;

        size_t countIdleQueueItems = 1;
        if (_i2sIdleDataSize > 256)
        {
            // reuse a single idle data buffer of 256 with multiple dma slc_queue_items
            countIdleQueueItems = _i2sIdleDataSize / 256 + 1;
            _i2sIdleDataSize = 256;
        }
        else
        {
            _i2sIdleDataSize = NeoUtil::RoundUp(_i2sIdleDataSize, c_I2sByteBoundarySize);
        }
        _is2BufMaxBlockSize = is2BufMaxBlockSize;

        _i2sBuffer = static_cast<uint8_t*>(malloc(_i2sBufferSize));
        // no need to initialize it, it gets overwritten on every send
        _i2sIdleData = static_cast<uint8_t*>(malloc(_i2sIdleDataSize));
        memset(_i2sIdleData, idleLevel * 0xff, _i2sIdleDataSize);

        _i2sBufDescCount = (_i2sBufferSize / _is2BufMaxBlockSize) + 1 + 
                countIdleQueueItems +
                c_StateBlockCount; // need more for state/latch blocks

        _i2sBufDesc = (slc_queue_item*)malloc(_i2sBufDescCount * sizeof(slc_queue_item));

        s_this = this; // store this for the ISR
    }

    void FreeI2s()
    {
        StopI2s();

        s_this = nullptr;
        pinMode(c_I2sPin, INPUT);

        free(_i2sBuffer);
        free(_i2sBufDesc);
        free(_i2sIdleData);
    }

    bool IsIdle() const
    {
        return (_dmaState == NeoDmaState_Idle);
    }

    void InitializeI2s(const uint32_t i2sClockDivisor, const uint32_t i2sBaseClockDivisor)
    {
        StopI2s();

        pinMode(c_I2sPin, FUNCTION_1); // I2S0_DATA

        uint8_t* is2Buffer = _i2sBuffer;
        uint8_t* is2BufferEnd = _i2sBuffer + _i2sBufferSize;
        uint32_t is2BufferSize = _i2sBufferSize;
        uint16_t indexDesc = 0;

        Serial.print(" buf desc ");
        Serial.println(_i2sBufDescCount);

        // prepare main data block decriptors that point into our one static dma buffer
        while (is2Buffer < is2BufferEnd)
        {
            uint32_t blockSize = (is2BufferSize > _is2BufMaxBlockSize) ? _is2BufMaxBlockSize : is2BufferSize;
            
            Serial.print(indexDesc);
            Serial.print(" data ");
            Serial.println(blockSize);

            _i2sBufDesc[indexDesc].owner = 1;
            _i2sBufDesc[indexDesc].eof = 0; // no need to trigger interrupt generally
            _i2sBufDesc[indexDesc].sub_sof = 0;
            _i2sBufDesc[indexDesc].datalen = blockSize;
            _i2sBufDesc[indexDesc].blocksize = blockSize;
            _i2sBufDesc[indexDesc].buf_ptr = is2Buffer;
            _i2sBufDesc[indexDesc].unused = 0;
            _i2sBufDesc[indexDesc].next_link_ptr = reinterpret_cast<struct slc_queue_item*>(&(_i2sBufDesc[indexDesc + 1]));

            is2Buffer += blockSize;
            is2BufferSize -= blockSize;
            indexDesc++;
        }

        is2BufferSize = _i2sIdleDataTotalSize;

        // prepare idle block decriptors that point into our one idle dma buffer
        while (indexDesc < (_i2sBufDescCount - c_StateBlockCount))
        {
            uint32_t blockSize = (is2BufferSize > _i2sIdleDataSize) ? _i2sIdleDataSize : is2BufferSize;
            
            Serial.print(indexDesc);
            Serial.print(" idle ");
            Serial.println(blockSize);

            _i2sBufDesc[indexDesc].owner = 1;
            _i2sBufDesc[indexDesc].eof = 0; // no need to trigger interrupt generally
            _i2sBufDesc[indexDesc].sub_sof = 0;
            _i2sBufDesc[indexDesc].datalen = blockSize;
            _i2sBufDesc[indexDesc].blocksize = blockSize;
            _i2sBufDesc[indexDesc].buf_ptr = _i2sIdleData;
            _i2sBufDesc[indexDesc].unused = 0;
            _i2sBufDesc[indexDesc].next_link_ptr = reinterpret_cast<struct slc_queue_item*>(&(_i2sBufDesc[indexDesc + 1]));

            is2Buffer += blockSize;
            is2BufferSize -= blockSize;
            indexDesc++;
        }

        // prepare the two state/latch descriptors
        uint16_t indexDescFirstState = indexDesc;
        while (indexDesc < _i2sBufDescCount)
        {
            Serial.print(indexDesc);
            Serial.print(" state ");
            Serial.println(c_I2sByteBoundarySize);

            _i2sBufDesc[indexDesc].owner = 1;
            _i2sBufDesc[indexDesc].eof = 0; // no need to trigger interrupt generally
            _i2sBufDesc[indexDesc].sub_sof = 0;
            _i2sBufDesc[indexDesc].datalen = c_I2sByteBoundarySize; // minimum
            _i2sBufDesc[indexDesc].blocksize = c_I2sByteBoundarySize;
            _i2sBufDesc[indexDesc].buf_ptr = _i2sIdleData;
            _i2sBufDesc[indexDesc].unused = 0;
            _i2sBufDesc[indexDesc].next_link_ptr = reinterpret_cast<struct slc_queue_item*>(&(_i2sBufDesc[indexDesc + 1]));

            indexDesc++;
        }

        uint16_t indexDescLastState = indexDesc - 1;
        // the first state block will trigger the interrupt
        _i2sBufDesc[indexDescFirstState].eof = 1;
        // the last state block will loop to the first state block by defualt
        _i2sBufDesc[indexDescLastState].next_link_ptr = reinterpret_cast<struct slc_queue_item*>(&(_i2sBufDesc[indexDescFirstState]));

        // setup the rest of i2s DMA
        //
        ETS_SLC_INTR_DISABLE();

        // start off in sending state as that is what it will be all setup to be
        // for the interrupt
        _dmaState = NeoDmaState_Sending;

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
        // set TX descriptor address. any random desc is OK, we don't use TX but it needs to be valid
        SLCTXL |= (uint32) & (_i2sBufDesc[_i2sBufDescCount - 1]) << SLCTXLA;
        SLCRXL &= ~(SLCRXLAM << SLCRXLA); // clear RX descriptor address
        // set RX descriptor address.  use first of the data addresses
        SLCRXL |= (uint32) & (_i2sBufDesc[0]) << SLCRXLA;

        ETS_SLC_INTR_ATTACH(i2s_slc_isr, NULL);
        SLCIE = SLCIRXEOF; // Enable only for RX EOF interrupt

        ETS_SLC_INTR_ENABLE();

        //Start transmission
        SLCTXL |= SLCTXLS;
        SLCRXL |= SLCRXLS;

        I2S_CLK_ENABLE();
        I2SIC = 0x3F;
        I2SIE = 0;

        //Reset I2S
        I2SC &= ~(I2SRST);
        I2SC |= I2SRST;
        I2SC &= ~(I2SRST);

        // Set RX/TX FIFO_MOD=0 and disable DMA (FIFO only)
        I2SFC &= ~(I2SDE | (I2STXFMM << I2STXFM) | (I2SRXFMM << I2SRXFM));
        I2SFC |= I2SDE; //Enable DMA
        // Set RX/TX CHAN_MOD=0
        I2SCC &= ~((I2STXCMM << I2STXCM) | (I2SRXCMM << I2SRXCM));

        // set the rate
        uint32_t i2s_clock_div = i2sClockDivisor & I2SCDM;
        uint8_t i2s_bck_div = i2sBaseClockDivisor & I2SBDM;

        //!trans master, !bits mod, rece slave mod, rece msb shift, right first, msb right
        I2SC &= ~(I2STSM | I2SRSM | (I2SBMM << I2SBM) | (I2SBDM << I2SBD) | (I2SCDM << I2SCD));
        I2SC |= I2SRF | I2SMR | I2SRSM | I2SRMS | (i2s_bck_div << I2SBD) | (i2s_clock_div << I2SCD);

        I2SC |= I2STXS; // Start transmission
    }

    void StopI2s()
    {
        ETS_SLC_INTR_DISABLE();

        // Disable any I2S send or receive
        I2SC &= ~(I2STXS | I2SRXS);

        // Reset I2S
        I2SC &= ~(I2SRST);
        I2SC |= I2SRST;
        I2SC &= ~(I2SRST);

        SLCIC = 0xFFFFFFFF;
        SLCIE = 0;
        SLCTXL &= ~(SLCTXLAM << SLCTXLA); // clear TX descriptor address
        SLCRXL &= ~(SLCRXLAM << SLCRXLA); // clear RX descriptor address

        pinMode(c_I2sPin, INPUT);
    }
};

#endif // ARDUINO_ARCH_ESP8266