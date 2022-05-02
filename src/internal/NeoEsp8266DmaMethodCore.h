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

class NeoEsp8266DmaMethodCore
{
protected:
    static NeoEsp8266DmaMethodCore* s_this; // for the ISR

    volatile NeoDmaState _dmaState;

    slc_queue_item* _i2sBufDesc;  // dma block descriptors
    uint16_t _i2sBufDescCount;   // count of block descriptors in _i2sBufDesc


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
                (finished_item + 1)->next_link_ptr = s_this->_i2sBufDesc;

                s_this->_dmaState = NeoDmaState_Sending;
            }
            break;

            case NeoDmaState_Sending:
            {
                slc_queue_item* finished_item = (slc_queue_item*)SLCRXEDA;

                // the data block had actual data sent
                // point last state block to first state block thus
                // just looping and not sending the data blocks
                (finished_item + 1)->next_link_ptr = finished_item;

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
};

#endif // ARDUINO_ARCH_ESP8266