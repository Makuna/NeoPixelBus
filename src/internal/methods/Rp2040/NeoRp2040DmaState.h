/*-------------------------------------------------------------------------
NeoPixel library helper functions for RP2040.

Written by Michael C. Miller.

I invest time and resources providing this open source code,
please support me by donating (see https://github.com/Makuna/NeoPixelBus)

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

#ifdef ARDUINO_ARCH_RP2040

// DMA Finished State Tracking
// --------------------------------------------------------

enum Rp2040PioDmaState
{
    Rp2040PioDmaState_Sending,
    Rp2040PioDmaState_DmaCompleted,
    Rp2040PioDmaState_FifoEmptied
};

template <uint V_IRQ_INDEX>
class NeoRp2040DmaState
{
public:
    NeoRp2040DmaState() :
        _endTime(0),
        _state(Rp2040PioDmaState_FifoEmptied)
    {
    }

    void SetSending()
    {
        _state = Rp2040PioDmaState_Sending;
    }

    void DmaFinished()
    {
        _endTime = micros();
        _state = Rp2040PioDmaState_DmaCompleted;
    }

    bool IsReadyToSend(uint32_t resetTimeUs)  const
    {
        bool isReady = false;

        switch (_state)
        {
        case Rp2040PioDmaState_Sending:
            break;

        case Rp2040PioDmaState_DmaCompleted:
        {
            uint32_t delta = micros() - _endTime;

            if (delta >= resetTimeUs)
            {
                // const method requires that we const cast to change state
                *const_cast<Rp2040PioDmaState*>(&_state) = Rp2040PioDmaState_FifoEmptied;
                isReady = true;
            }
        }
        break;

        default:
            isReady = true;
            break;
        }

        return isReady;
    }

    void Register(uint dmChannel)
    {
        if (s_dmaIrqObjectTable[dmChannel] == nullptr)
        {
            s_dmaIrqObjectTable[dmChannel] = this;

            int32_t refCount = s_refCountHandler++;

            if (refCount == 0)
            {
                // Set up end-of-DMA interrupt handler
                irq_add_shared_handler(V_IRQ_INDEX ? DMA_IRQ_1 : DMA_IRQ_0,
                    dmaFinishIrq,
                    PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
                irq_set_enabled(V_IRQ_INDEX ? DMA_IRQ_1 : DMA_IRQ_0, true);
            }
        }

#if defined(NEORP2040_DEBUG)

        Serial.print("  V_IRQ_INDEX = ");
        Serial.print(V_IRQ_INDEX);
        Serial.print(", s_dmaIrqObjectTable = ");
        Serial.print((uint32_t)(s_dmaIrqObjectTable)); // address of s_dmaIrqObjectTable
        Serial.println();

#endif

    }

    void Unregister(uint dmChannel)
    {
        if (s_dmaIrqObjectTable[dmChannel] == this)
        {
            int32_t refCount = s_refCountHandler--;

            if (refCount == 0)
            {
                irq_set_enabled(V_IRQ_INDEX ? DMA_IRQ_1 : DMA_IRQ_0, false);
                irq_remove_handler(V_IRQ_INDEX ? DMA_IRQ_1 : DMA_IRQ_0, dmaFinishIrq);
            }

            s_dmaIrqObjectTable[dmChannel] = nullptr;
        }
    }

private:
    volatile uint32_t _endTime;  // Latch/Reset timing reference
    volatile Rp2040PioDmaState _state;

    static NeoRp2040DmaState* s_dmaIrqObjectTable[NUM_DMA_CHANNELS];
    static volatile int32_t s_refCountHandler;

    static void dmaFinishIrq()
    {
        // dmaChannels are unique across both PIOs, while stateMachines are per
        // PIO, so this current model below works even if both PIOs are used
        for (uint dmaChannel = 0; dmaChannel < NUM_DMA_CHANNELS; dmaChannel++)
        {
            NeoRp2040DmaState* that = s_dmaIrqObjectTable[dmaChannel];
            if (that != nullptr)
            {
                if (dma_irqn_get_channel_status(V_IRQ_INDEX, dmaChannel))
                {
                    dma_irqn_acknowledge_channel(V_IRQ_INDEX, dmaChannel);
                    that->DmaFinished();
                }
            }
        }
    }
};

template <uint V_IRQ_INDEX>
NeoRp2040DmaState<V_IRQ_INDEX>* NeoRp2040DmaState<V_IRQ_INDEX>::s_dmaIrqObjectTable[NUM_DMA_CHANNELS] =
    {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    };

template <uint V_IRQ_INDEX>
volatile int32_t NeoRp2040DmaState<V_IRQ_INDEX>::s_refCountHandler = 0;


#endif