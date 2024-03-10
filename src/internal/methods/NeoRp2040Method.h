/*-------------------------------------------------------------------------
NeoPixel library helper functions for RP2040.

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

#ifdef ARDUINO_ARCH_RP2040

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/mutex.h"

// DMA Finished State Tracking
// --------------------------------------------------------

enum Rp2040PioDmaState
{
    Rp2040PioDmaState_Sending,
    Rp2040PioDmaState_DmaCompleted,
    Rp2040PioDmaState_FifoEmptied
};

class NeoRp2040DmaState
{
private:
    volatile uint32_t _endTime;  // Latch/Reset timing reference
    volatile Rp2040PioDmaState _state;

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
//                digitalWrite(7, HIGH);
                uint32_t delta = micros() - _endTime;

                if (delta >= resetTimeUs)
                {
//                    digitalWrite(7, LOW);

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
};

// PIO Instances
// --------------------------------------------------------
class NeoRp2040PioInstance0
{
public:
    NeoRp2040PioInstance0() :
        Instance(pio0)
    {};

    const PIO Instance;
};

class NeoRp2040PioInstance1
{
public:
    NeoRp2040PioInstance1() :
        Instance(pio1)
    {};

    const PIO Instance;
};

// dynamic channel support
class NeoRp2040PioInstanceN
{
public:
    NeoRp2040PioInstanceN(NeoBusChannel channel) :
        Instance(channel == NeoBusChannel_0 ? pio0 : pio1)
    {
    }
    NeoRp2040PioInstanceN() = delete; // no default constructor

    const PIO Instance;
};

// PIO Programs (cadence)
// --------------------------------------------------------
// use https://wokwi.com/tools/pioasm
// copy relevant parts into NeoRp2040PioCadenceMono3Step and NeoRp2040PioCadenceMono4Step
// 
// 3 step program
// the pulse width is divided by 3, using 33% for each stage
// where 0 = 33% and 1 = 66%
/*
.program rgbic_mono
.side_set 1

; 0 bit
; TH0 TH1 TL1 
; +++ ___ ___ 

; 1 bit
; TH0 TH1 TL1
; +++ +++ ___
.define public TH0 1 ; T1
.define public TH1 1 ; T2
.define public TL1 1 ; T3

.wrap_target
bitloop:
    out x, 1		side 0 [TL1 - 1] ; Side-set still takes place when instruction stalls
    jmp !x do_zero	side 1 [TH0 - 1] ; Branch on the bit we shifted out. Positive pulse
do_one:
    jmp bitloop		side 1 [TH1 - 1] ; Continue driving high, for a long pulse
do_zero:
    nop		side 0 [TH1 - 1] ; Or drive low, for a short pulse 
.wrap
*/
//
class NeoRp2040PioCadenceMono3Step
{
protected:
    static constexpr uint8_t wrap_target = 0;
    static constexpr uint8_t wrap = 3;

    static constexpr uint8_t TH0 = 1;
    static constexpr uint8_t TH1 = 1;
    static constexpr uint8_t TL1 = 1;

    // changed from constepr with initializtion due to 
    // that is only supported in c17+
    static const uint16_t program_instructions[];

public:
    // changed from constepr with initializtion due to 
    // that is only supported in c17+
    static const struct pio_program program;

    static inline pio_sm_config get_default_config(uint offset)
    {
        pio_sm_config c = pio_get_default_sm_config();
        sm_config_set_wrap(&c, offset + wrap_target, offset + wrap);
        sm_config_set_sideset(&c, 1, false, false);
        return c;
    }

    static constexpr uint8_t bit_cycles = TH0 + TH1 + TL1;
};

// 4 step program
// the pulse width is divided by 4, using 25% for each stage
// where 0 = 25% and 1 = 75%
//
/*
.program rgbic_mono
.side_set 1

; 0 bit
; TH0 TH1 TL1
; +++ ___ ___

; 1 bit
; TH0 TH1 TL1
; +++ +++ ___
.define public TH0 1 ; T1
.define public TH1 2 ; T2
.define public TL1 1 ; T3

.wrap_target
bitloop:
    out x, 1		side 0 [TL1 - 1] ; Side-set still takes place when instruction stalls
    jmp !x do_zero	side 1 [TH0 - 1] ; Branch on the bit we shifted out. Positive pulse
do_one:
    jmp bitloop		side 1 [TH1 - 1] ; Continue driving high, for a long pulse
do_zero:
    nop		side 0 [TH1 - 1] ; Or drive low, for a short pulse
.wrap
*/
//
class NeoRp2040PioCadenceMono4Step
{
protected:
    static constexpr uint8_t wrap_target = 0;
    static constexpr uint8_t wrap = 3;

    static constexpr uint8_t TH0 = 1;
    static constexpr uint8_t TH1 = 2;
    static constexpr uint8_t TL1 = 1;

    // changed from constepr with initializtion due to 
    // that is only supported in c17+
    static const uint16_t program_instructions[];

public:
    // changed from constepr with initializtion due to 
    // that is only supported in c17+
    static const struct pio_program program;

    static inline pio_sm_config get_default_config(uint offset)
    {
        pio_sm_config c = pio_get_default_sm_config();
        sm_config_set_wrap(&c, offset + wrap_target, offset + wrap);
        sm_config_set_sideset(&c, 1, false, false);
        return c;
    }

    static constexpr uint8_t bit_cycles = TH0 + TH1 + TL1;
};


// Program Wrapper
// --------------------------------------------------------
template<typename T_CADENCE> 
class NeoRp2040PioMonoProgram
{
public:
    static inline uint add(PIO pio_instance)
    {
        return pio_add_program(pio_instance, &T_CADENCE::program);
    }

    static inline void init(PIO pio_instance, uint sm, uint offset, uint pin, float bitrate, uint shiftBits)
    {
        float div = clock_get_hz(clk_sys) / (bitrate * T_CADENCE::bit_cycles);
        pio_sm_config c = T_CADENCE::get_default_config(offset);

        sm_config_set_sideset_pins(&c, pin);
        
        sm_config_set_out_shift(&c, false, true, shiftBits); // was 32 ? is this needed
        sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
        sm_config_set_clkdiv(&c, div);

        // Set this pin's GPIO function (connect PIO to the pad)
        pio_gpio_init(pio_instance, pin);

        // Set the pin direction to output at the PIO
        pio_sm_set_consecutive_pindirs(pio_instance, sm, pin, 1, true);

        // Load our configuration, and jump to the start of the program
        pio_sm_init(pio_instance, sm, offset, &c);

        // Set the state machine running
        pio_sm_set_enabled(pio_instance, sm, true);
    }
};

// Speeds
// --------------------------------------------------------
class NeoRp2040PioSpeedWs2811 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz =  800000.0f; // 300+950
    static constexpr uint32_t ResetTimeUs = 300;
};

class NeoRp2040PioSpeedWs2812x : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz =  800000.0f; // 400+850
    static constexpr uint32_t ResetTimeUs = 300;
};

class NeoRp2040PioSpeedSk6812 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz =  800000.0f; // 400+850
    static constexpr uint32_t ResetTimeUs = 80;
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1814 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz =  800000.00f; // 360+890
    static constexpr uint32_t ResetTimeUs = 200;
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1829 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz =  833333.33f; // 300+900
    static constexpr uint32_t ResetTimeUs = 200;
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1914 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz =  800000.0f; // 360+890
    static constexpr uint32_t ResetTimeUs = 200;
};

class NeoRp2040PioSpeed800Kbps : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz =  800000.0f; // 400+850
    static constexpr uint32_t ResetTimeUs = 50;
};

class NeoRp2040PioSpeed400Kbps : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz =  400000.0f; // 800+1700
    static constexpr uint32_t ResetTimeUs = 50;
};

class NeoRp2040PioSpeedApa106 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz =  588235.29f; // 350+1350
    static constexpr uint32_t ResetTimeUs = 50;
};

class NeoRp2040PioSpeedTx1812 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz =  1111111.11f; // 300+600
    static constexpr uint32_t ResetTimeUs = 80;
};

class NeoRp2040PioSpeedGs1903 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz =  833333.33f; // 300+900
    static constexpr uint32_t ResetTimeUs = 40;
};

// Method
// --------------------------------------------------------
template<typename T_SPEED, 
        typename T_PIO_INSTANCE, 
        bool V_INVERT = false, 
        uint V_IRQ_INDEX = 1> 
class NeoRp2040PioMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoRp2040PioMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize)  :
        _sizeData(NeoUtil::RoundUp(pixelCount * elementSize + settingsSize, c_DataByteAlignment)),
        _pin(pin)
    {
        construct();
    }

    NeoRp2040PioMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize, NeoBusChannel channel) :
        _sizeData(NeoUtil::RoundUp(pixelCount* elementSize + settingsSize, c_DataByteAlignment)),
        _pin(pin),
        _pio(channel)
    {
        construct();
    }

    ~NeoRp2040PioMethodBase()
    {
        // wait until the last send finishes before destructing everything
        dma_channel_wait_for_finish_blocking(_dmaChannel);

        // disable the state machine
        pio_sm_set_enabled(_pio.Instance, _sm, false);

        // Disable and remove interrupts
        // dma_channel_cleanup(_dmaChannel); // NOT PRESENT?!
        dma_irqn_set_channel_enabled(V_IRQ_INDEX, _dmaChannel, false);

        irq_remove_handler(V_IRQ_INDEX ? DMA_IRQ_1 : DMA_IRQ_0, dmaFinishIrq);

        // unregister static dma callback object
        s_dmaIrqObjectTable[_dmaChannel] = nullptr;

        // unclaim dma channel and then state machine
        dma_channel_unclaim(_dmaChannel);
        pio_sm_unclaim(_pio.Instance, _sm);

        pinMode(_pin, INPUT);

        free(_dataEditing);
        free(_dataSending);
    }

    bool IsReadyToUpdate() const
    {
        uint16_t fifoCacheLatency = 1000000.0f / T_SPEED::BitRateHz * 8 * 8.0f + 6; // 8 bits * (8.5 DMA words in merged FIFO)
        return _dmaState.IsReadyToSend(T_SPEED::ResetTimeUs + fifoCacheLatency);
    }

    void Initialize()
    {
        // Select the largest FIFO fetch size that aligns with our data size
        //
        uint shiftBits = 8;
        /*
        if (_sizeData % 4 == 0)
        {
            shiftBits = 32;
        }
        else if (_sizeData % 2 == 0)
        {
            shiftBits = 16;
        }
        */
        dma_channel_transfer_size dmaTransferSize = static_cast<dma_channel_transfer_size>(shiftBits / 16);
        uint transfer_count = _sizeData / (shiftBits / 8);;

        uint16_t fifoCacheLatency = 1000000.0f / T_SPEED::BitRateHz * shiftBits * 8.0f + 6; // shift bits * (8.5 DMA words in merged FIFO)

Serial.print(", _sizeData = ");
Serial.print(_sizeData);
Serial.print(", dmaTransferSize = ");
Serial.print(dmaTransferSize);
Serial.print(", transfer_count = ");
Serial.print(transfer_count);
Serial.print(", shiftBits = ");
Serial.print(shiftBits);
Serial.print(", fifoCacheLatency = ");
Serial.print(fifoCacheLatency);

Serial.println();

        // Our assembled program needs to be loaded into this PIO's instruction
        // memory. This SDK function will find a location (offset) in the
        // instruction memory where there is enough space for our program. We need
        // to remember this location!
        uint offset = T_SPEED::add(_pio.Instance);
        
        // Find a free state machine on our chosen PIO (erroring if there are
        // none?). Configure it to run our program, and start it, using the
        // helper function we included in our .pio file.
        _sm = pio_claim_unused_sm(_pio.Instance, true); // panic if none available

Serial.print("*_sm = ");
Serial.print(_sm);

        T_SPEED::init(_pio.Instance, _sm, offset, _pin, T_SPEED::BitRateHz, shiftBits);
        
        if (V_INVERT)
        {
            gpio_set_oeover(_pin, GPIO_OVERRIDE_INVERT);
        }

        // Set up DMA transfer
        _dmaChannel = dma_claim_unused_channel(true); // panic if none available

Serial.print(", *_dmaChannel = ");
Serial.print(_dmaChannel);

        // register for IRQ shared static endTime update
        s_dmaIrqObjectTable[_dmaChannel] = &_dmaState;

Serial.print(", s_dmaIrqObjectTable = ");
Serial.print((uint32_t)(s_dmaIrqObjectTable)); // address of s_dmaIrqObjectTable

Serial.println();

        dma_channel_config dmaConfig = dma_channel_get_default_config(_dmaChannel);
        channel_config_set_transfer_data_size(&dmaConfig, dmaTransferSize);
        channel_config_set_read_increment(&dmaConfig, true);
        channel_config_set_write_increment(&dmaConfig, false);

        // Set DMA trigger
//        channel_config_set_irq_quiet(&dmaConfig, true);
        channel_config_set_dreq(&dmaConfig, pio_get_dreq(_pio.Instance, _sm, true));

        dma_channel_configure(_dmaChannel, 
            &dmaConfig,
            &(_pio.Instance->txf[_sm]),  // dest
            _dataSending,                // src
            transfer_count,
            false);

        // Set up end-of-DMA interrupt
        irq_add_shared_handler(V_IRQ_INDEX ? DMA_IRQ_1 : DMA_IRQ_0,
            dmaFinishIrq,
            PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);

        dma_irqn_set_channel_enabled(V_IRQ_INDEX, _dmaChannel, true);
 
        irq_set_enabled(V_IRQ_INDEX ? DMA_IRQ_1 : DMA_IRQ_0, true);
    }

    void Update(bool maintainBufferConsistency)
    {
        // wait for last send
        while (!IsReadyToUpdate())
        {
            yield();  
        }
  
        _dmaState.SetSending();

        // start next send
        // 
        dma_channel_set_read_addr(_dmaChannel, _dataEditing, false);
        pio_sm_clear_fifos(_pio.Instance, _sm); // not really needed
        dma_channel_start(_dmaChannel); // Start new transfer

        if (maintainBufferConsistency)
        {
            // copy editing to sending,
            // this maintains the contract that "colors present before will
            // be the same after", otherwise GetPixelColor will be inconsistent
            memcpy(_dataSending, _dataEditing, _sizeData);
        }

        // swap so the user can modify without affecting the async operation
        std::swap(_dataSending, _dataEditing);
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called only if changes to buffer
        return false;
    }

    uint8_t* getData() const
    {
        return _dataEditing;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    static constexpr uint8_t c_DataByteAlignment = 1; // DMA_SIZE_8 or  4 for DMA_SIZE_32

    const size_t  _sizeData;     // Size of '_data*' buffers 
    const uint8_t _pin;          // output pin number
    const T_PIO_INSTANCE _pio;   // holds instance for multi channel support

    NeoRp2040DmaState _dmaState;   // Latch timing reference

    // Holds data stream which include LED color values and other settings as needed
    uint8_t*  _dataEditing;   // exposed for get and set
    uint8_t*  _dataSending;   // used for async send using DMA

    // holds pio state
    int _sm;
    int _dmaChannel;

    // instead of a table of max channels to walk
    // this could be a table of size max state machines,
    // which is true limiting factor to instances
    // and the table entries would then have to include not only
    // the object pointer but also the dmaChannel it was also assigned
    // volatile static NeoRp2040PioMethodBase<T_SPEED, T_PIO_INSTANCE, V_INVERT, V_IRQNUM>* s_dmaIrqObjectTable[NUM_DMA_CHANNELS];
    static NeoRp2040DmaState* s_dmaIrqObjectTable[NUM_DMA_CHANNELS];

    void construct()
    {
        _dataEditing = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin() with a ClearTo(0)

        _dataSending = static_cast<uint8_t*>(malloc(_sizeData));
        // no need to initialize it, it gets overwritten on every send
    }

    static void dmaFinishIrq()
    {
        // alternat thought of using a table of max state machine instances (4 per PIO)
        // it would then contain an .object and a .dmaChannel
        // so 4 * (sizeof(object*) + sizeof(int)) = 4 * (4 + 2) = 24 byes
        // versus
        // 12 * (sizeof(object*) = 12 * 4 = 48 bytes
        // along with reduce interation of the array for every interrupt
        /*
        for (uint stateMachine = 0; stateMachine < NUM_PIO_STATE_MACHINES; stateMachine++)
        {
            if (s_dmaIrqObjectTable[stateMachine].object != nullptr)
            {
                uint dmaChannel = s_dmaIrqObjectTable[stateMachine].dmaChannel;
                if (dma_irqn_get_channel_status(V_IRQNUM, dmaChannel))
                {
                    dma_irqn_acknowledge_channel(V_IRQNUM, dmaChannel);
                    s_dmaIrqObjectTable[stateMachine].object->dmaCallback();
                }
            }
        }
*/
// FURTHER alternat thought of using a table of max state machine instances (4 per PIO)
// it would then contain an .pEndTime and a .dmaChannel.  Without a object pointer the
// alternate Bus definitions (feature/method template) could share a single static table
// 
// so 4 * (sizeof(EndTime*) + sizeof(int)) = 4 * (4 + 2) = 24 byes
// versus
// 12 * (sizeof(EndTime*) = 12 * 4 = 48 bytes
// along with reduce interation of the array for every interrupt AND
// reduce code due to object method call
/*
for (uint stateMachine = 0; stateMachine < NUM_PIO_STATE_MACHINES; stateMachine++)
{
    uint32_t* pEndTime = s_dmaIrqObjectTable[stateMachine].pEndTime;
    if (pEndTime != nullptr)
    {
        uint dmaChannel = s_dmaIrqObjectTable[stateMachine].dmaChannel;
        if (dma_irqn_get_channel_status(V_IRQNUM, dmaChannel))
        {
            dma_irqn_acknowledge_channel(V_IRQNUM, dmaChannel);
            *pEndTime = micros();
        }
    }
}
*/
        
        // dmaChannels are unique across both PIOs, while stateMachines are per
        // PIO, so this current model below works even if both PIOs are used
        for (uint dmaChannel = 0; dmaChannel < NUM_DMA_CHANNELS; dmaChannel++)
        {
            if (s_dmaIrqObjectTable[dmaChannel] != nullptr)
            {
                if (dma_irqn_get_channel_status(V_IRQ_INDEX, dmaChannel))
                {
                    dma_irqn_acknowledge_channel(V_IRQ_INDEX, dmaChannel);
                    s_dmaIrqObjectTable[dmaChannel]->DmaFinished();
                }
            }
        }
        
    }
};

template<typename T_SPEED, typename T_PIO_INSTANCE, bool V_INVERT, uint V_IRQ_INDEX>
//volatile NeoRp2040PioMethodBase<T_SPEED, T_PIO_INSTANCE, V_INVERT, V_IRQNUM>* NeoRp2040PioMethodBase<T_SPEED, T_PIO_INSTANCE, V_INVERT, V_IRQNUM>::s_dmaIrqObjectTable[] = { nullptr };
NeoRp2040DmaState* NeoRp2040PioMethodBase<T_SPEED, T_PIO_INSTANCE, V_INVERT, V_IRQ_INDEX>::s_dmaIrqObjectTable[] = { nullptr };

// normal
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstanceN> Rp2040NWs2811Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstanceN> Rp2040NWs2812xMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstanceN> Rp2040NWs2816Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstanceN> Rp2040NSk6812Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstanceN, true> Rp2040NTm1814Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstanceN, true> Rp2040NTm1829Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstanceN, true> Rp2040NTm1914Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstanceN> Rp2040NApa106Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstanceN> Rp2040NTx1812Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstanceN> Rp2040NGs1903Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstanceN> Rp2040N800KbpsMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstanceN> Rp2040N400KbpsMethod;

typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstance0> Rp2040Pio0Ws2811Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance0> Rp2040Pio0Ws2812xMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance0> Rp2040Pio0Ws2816Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstance0> Rp2040Pio0Sk6812Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstance0, true> Rp2040Pio0Tm1814Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstance0, true> Rp2040Pio0Tm1829Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstance0, true> Rp2040Pio0Tm1914Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstance0> Rp2040Pio0Apa106Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstance0> Rp2040Pio0Tx1812Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstance0> Rp2040Pio0Gs1903Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstance0> Rp2040Pio0800KbpsMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstance0> Rp2040Pio0400KbpsMethod;

typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstance1> Rp2040Pio1Ws2811Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance1> Rp2040Pio1Ws2812xMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance1> Rp2040Pio1Ws2816Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstance1> Rp2040Pio1Sk6812Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstance1, true> Rp2040Pio1Tm1814Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstance1, true> Rp2040Pio1Tm1829Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstance1, true> Rp2040Pio1Tm1914Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstance1> Rp2040Pio1Apa106Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstance1> Rp2040Pio1Tx1812Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstance1> Rp2040Pio1Gs1903Method;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstance1> Rp2040Pio1800KbpsMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstance1> Rp2040Pio1400KbpsMethod;

// inverted
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstanceN, true> Rp2040NWs2811InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstanceN, true> Rp2040NWs2812xInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstanceN, true> Rp2040NWs2816InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstanceN, true> Rp2040NSk6812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstanceN> Rp2040NTm1814InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstanceN> Rp2040NTm1829InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstanceN> Rp2040NTm1914InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstanceN, true> Rp2040NApa106InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstanceN, true> Rp2040NTx1812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstanceN, true> Rp2040NGs1903InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstanceN, true> Rp2040N800KbpsInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstanceN, true> Rp2040N400KbpsInvertedMethod;

typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstance0, true> Rp2040Pio0Ws2811InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance0, true> Rp2040Pio0Ws2812xInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance0, true> Rp2040Pio0Ws2816InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstance0, true> Rp2040Pio0Sk6812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstance0> Rp2040Pio0Tm1814InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstance0> Rp2040Pio0Tm1829InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstance0> Rp2040Pio0Tm1914InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstance0, true> Rp2040Pio0Apa106InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstance0, true> Rp2040Pio0Tx1812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstance0, true> Rp2040Pio0Gs1903InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstance0, true> Rp2040Pio0800KbpsInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstance0, true> Rp2040Pio0400KbpsInvertedMethod;

typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstance1, true> Rp2040Pio1Ws2811InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance1, true> Rp2040Pio1Ws2812xInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance1, true> Rp2040Pio1Ws2816InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstance1, true> Rp2040Pio1Sk6812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstance1> Rp2040Pio1Tm1814InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstance1> Rp2040Pio1Tm1829InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstance1> Rp2040Pio1Tm1914InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstance1, true> Rp2040Pio1Apa106InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstance1, true> Rp2040Pio1Tx1812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstance1, true> Rp2040Pio1Gs1903InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstance1, true> Rp2040Pio1800KbpsInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstance1, true> Rp2040Pio1400KbpsInvertedMethod;

// IRQ 1 method is the default method 
typedef Rp2040Pio1Ws2812xMethod NeoWs2813Method;
typedef Rp2040Pio1Ws2812xMethod NeoWs2812xMethod;
typedef Rp2040Pio1800KbpsMethod NeoWs2812Method;
typedef Rp2040Pio1Ws2812xMethod NeoWs2811Method;
typedef Rp2040Pio1Ws2812xMethod NeoWs2816Method;
typedef Rp2040Pio1Sk6812Method NeoSk6812Method;
typedef Rp2040Pio1Tm1814Method NeoTm1814Method;
typedef Rp2040Pio1Tm1829Method NeoTm1829Method;
typedef Rp2040Pio1Tm1914Method NeoTm1914Method;
typedef Rp2040Pio1Sk6812Method NeoLc8812Method;
typedef Rp2040Pio1Apa106Method NeoApa106Method;
typedef Rp2040Pio1Tx1812Method NeoTx1812Method;
typedef Rp2040Pio1Gs1903Method NeoGs1903Method;

typedef Rp2040Pio1Ws2812xMethod Neo800KbpsMethod;
typedef Rp2040Pio1400KbpsMethod Neo400KbpsMethod;

typedef Rp2040Pio1Ws2812xInvertedMethod NeoWs2813InvertedMethod;
typedef Rp2040Pio1Ws2812xInvertedMethod NeoWs2812xInvertedMethod;
typedef Rp2040Pio1Ws2812xInvertedMethod NeoWs2811InvertedMethod;
typedef Rp2040Pio1800KbpsInvertedMethod NeoWs2812InvertedMethod;
typedef Rp2040Pio1Ws2812xInvertedMethod NeoWs2816InvertedMethod;
typedef Rp2040Pio1Sk6812InvertedMethod NeoSk6812InvertedMethod;
typedef Rp2040Pio1Tm1814InvertedMethod NeoTm1814InvertedMethod;
typedef Rp2040Pio1Tm1829InvertedMethod NeoTm1829InvertedMethod;
typedef Rp2040Pio1Tm1914InvertedMethod NeoTm1914InvertedMethod;
typedef Rp2040Pio1Sk6812InvertedMethod NeoLc8812InvertedMethod;
typedef Rp2040Pio1Apa106InvertedMethod NeoApa106InvertedMethod;
typedef Rp2040Pio1Tx1812InvertedMethod NeoTx1812InvertedMethod;
typedef Rp2040Pio1Gs1903InvertedMethod NeoGs1903InvertedMethod;

typedef Rp2040Pio1Ws2812xInvertedMethod Neo800KbpsInvertedMethod;
typedef Rp2040Pio1400KbpsInvertedMethod Neo400KbpsInvertedMethod;


#endif
