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


class NeoRp2040PioInstance0
{
public:
    NeoRp2040PioInstance0() {};

    const static PIO Instance = pio0;
};

class NeoRp2040PioInstance1
{
public:
    NeoRp2040PioInstance1() {};

    const static PIO Instance = pio1;
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

// use https://wokwi.com/tools/pioasm
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
class NeoRp2040CandenceMono3Step
{
public:
    static const struct pio_program program =
    {
        .instructions = program_instructions,
        .length = 4,
        .origin = -1,
    };

    static inline pio_sm_config program_get_default_config(uint offset)
    {
        pio_sm_config c = pio_get_default_sm_config();
        sm_config_set_wrap(&c, offset + wrap_target, offset + wrap);
        sm_config_set_sideset(&c, 1, false, false);
        return c;
    }

    static constexpr uint8_t bit_cycles = TH0 + TH1 + TL1;

protected:
    static constexpr uint8_t wrap_target = 0;
    static constexpr uint8_t wrap = 3;

    static constexpr uint8_t TH0 = 1;
    static constexpr uint8_t TH1 = 1;
    static constexpr uint8_t TL1 = 1;

    static const uint16_t program_instructions[] = 
    {
                //     .wrap_target
        0x6021, //  0: out    x, 1            side 0     
        0x1023, //  1: jmp    !x, 3           side 1     
        0x1000, //  2: jmp    0               side 1     
        0xa042, //  3: nop                    side 0     
                //     .wrap
    };
};

// use https://wokwi.com/tools/pioasm
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
class NeoRp2040PioCandenceMono4Step
{
public:
    static const struct pio_program program =
    {
        .instructions = program_instructions,
        .length = 4,
        .origin = -1,
    };

    static inline pio_sm_config get_default_config(uint offset)
    {
        pio_sm_config c = pio_get_default_sm_config();
        sm_config_set_wrap(&c, offset + wrap_target, offset + wrap);
        sm_config_set_sideset(&c, 1, false, false);
        return c;
    }

    static constexpr uint8_t bit_cycles = TH0 + TH1 + TL1;

protected:
    static constexpr uint8_t wrap_target = 0;
    static constexpr uint8_t wrap = 3;

    static constexpr uint8_t TH0 = 1;
    static constexpr uint8_t TH1 = 2;
    static constexpr uint8_t TL1 = 1;

    static const uint16_t program_instructions[] =
    {
        //     .wrap_target
        0x6021, //  0: out    x, 1            side 0     
        0x1023, //  1: jmp    !x, 3           side 1     
        0x1100, //  2: jmp    0               side 1 [1] 
        0xa142, //  3: nop                    side 0 [1] 
        //     .wrap
    };
};

template<typename T_CANDENCE> class NeoRp2040PioMonoProgram
{
public:
    static inline uint add(PIO& pio_instance)
    {
        return pio_add_program(pio_instance, &T_CANDENCE::program);
    }

    static inline void init(PIO pio_instance, uint sm, uint offset, uint pin, float bitrate)
    {
        float div = clock_get_hz(clk_sys) / (bitrate * T_CANDENCE::bit_cycles);
        pio_sm_config c = T_CANDENCE::get_default_config(offset);

        sm_config_set_sideset_pins(&c, pin);
        sm_config_set_out_shift(&c, false, true, 32); // ? is this needed
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
}

/*

class NeoRp2040PioSpeedWs2811 : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono4Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(300, 950);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(900, 350);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(300000); // 300us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

class NeoRp2040PioSpeedWs2812x : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono3Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(400, 850);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(800, 450);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(300000); // 300us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

class NeoRp2040PioSpeedSk6812 : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono3Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(400, 850);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(800, 450);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(80000); // 80us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1814 : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono3Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(360, 890);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(720, 530);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(200000); // 200us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1829 : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono4Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(300, 900);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(800, 400);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(200000); // 200us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1914 : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono3Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(360, 890);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(720, 530);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(200000); // 200us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

class NeoRp2040PioSpeed800Kbps : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono4Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(400, 850);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(800, 450);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(50000); // 50us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

class NeoRp2040PioSpeed400Kbps : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono3Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(800, 1700);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(1600, 900);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(50000); // 50us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

class NeoRp2040PioSpeedApa106 : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono4Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(350, 1350);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(1350, 350);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(50000); // 50us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

class NeoRp2040PioSpeedTx1812 : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono3Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(300, 600);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(600, 300);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(80000); // 80us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};

class NeoRp2040PioSpeedGs1903 : public NeoRp2040PioMonoProgram<NeoRp2040PioCandenceMono4Step>
{
public:
    const static DRAM_ATTR uint32_t RmtBit0 = Item32Val(300, 900);
    const static DRAM_ATTR uint32_t RmtBit1 = Item32Val(900, 300);
    const static DRAM_ATTR uint16_t RmtDurationReset = FromNs(40000); // 40us

    static void IRAM_ATTR Translate(const void* src,
        rmt_item32_t* dest,
        size_t src_size,
        size_t wanted_num,
        size_t* translated_size,
        size_t* item_num);
};


*/

template<typename T_SPEED, typename T_PIO_INSTANCE, bool V_INVERT = false, uint V_IRQNUM = DMA_IRQ_0> class NeoRp2040MethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoRp2040MethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize)  :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin)
    {
        construct();
    }

    NeoRp2040MethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize, NeoBusChannel channel) :
        _sizeData(pixelCount* elementSize + settingsSize),
        _pin(pin),
        _pio(channel)
    {
        construct();
    }

    ~NeoRp2040MethodBase()
    {
        // wait until the last send finishes before destructing everything
        dma_channel_wait_for_finish_blocking(_dmaChannel);
        pio_sm_set_enabled(_pio.Instance, _sm, false);

        // unregister static dma callback object
        s_dmaIrqObjectTable[_dmaChannel] = nullptr;

        dma_channel_unclaim(_dmaChannel);
        pio_sm_unclaim(_pio.Instance, _sm);

        pinMode(_pin, INPUT);

        free(_dataEditing);
        free(_dataSending);
    }


    bool IsReadyToUpdate() const
    {
        if (!dma_channel_is_busy(_dmaChannel))
        {
            uint32_t delta = micros() - _endTime;

            return (delta >= T_SPEED::ResetTimeUs);
        }

        return false;
    }

    void Initialize()
    {
        // Our assembled program needs to be loaded into this PIO's instruction
        // memory. This SDK function will find a location (offset) in the
        // instruction memory where there is enough space for our program. We need
        // to remember this location!
        uint offset = T_PIO_PROGRAM::add(_pio.Instance);
        
        // Find a free state machine on our chosen PIO (erroring if there are
        // none). Configure it to run our program, and start it, using the
        // helper function we included in our .pio file.
        _sm = pio_claim_unused_sm(_pio.Instance, true);
        T_PIO_PROGRAM::init(_pio.Instance, _sm, offset, _pin, T_SPEED::Hz);
        
        if (V_INVERT)
        {
            gpio_set_oeover(pin, GPIO_OVERRIDE_INVERT);
        }

        // Set up DMA transfer
        _dmaChannel = dma_claim_unused_channel(true); // panic if none available


        // register for IRQ shared static callback
        s_dmaIrqObjectTable[_dmaChannel] = this;

        dma_channel_transfer_size dmaTransferSize = DMA_SIZE_32;
        dma_config = dma_channel_get_default_config(_dmaChannel);
        channel_config_set_transfer_data_size(&dma_config, dmaTransferSize);
        channel_config_set_read_increment(&dma_config, true);
        channel_config_set_write_increment(&dma_config, false);

        // Set DMA trigger
        channel_config_set_dreq(&dma_config, pio_get_dreq(_pio.Instance, _sm, true));
        uint transfer_count = _sizeData / ((dmaTransferSize == DMA_SIZE_8) ? 1 : dmaTransferSize * 2);

        dma_channel_configure(_dmaChannel, 
            &dma_config,
            &(_pio.Instance->txf[_sm]),      // dest
            _dataSending, // src
            transfer_count,
            false);

        // Set up end-of-DMA interrupt
        irq_add_shared_handler(V_IRQNUM,
            dmaFinishIrq,
            PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);

        if (V_IRQNUM == DMA_IRQ_0)
        {
            dma_channel_set_irq0_enabled(_dmaChannel, true);
        }
        else
        {
            dma_channel_set_irq1_enabled(_dmaChannel, true);
        }

        irq_set_enabled(V_IRQNUM, true);

        _endTime = micros();
    }

    void Update(bool maintainBufferConsistency)
    {
        // wait for last send
        while (!IsReadyToUpdate())
        {
            yeild();
        }

        // start next send
        // 
        dma_channel_set_read_addr(_dmaChannel, _dataEditing, false);
        pio_sm_clear_fifos(_pio.Instance, _sm);
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
    constexpr int c_DmaChannelsCount = 16;

    const size_t  _sizeData;      // Size of '_data*' buffers 
    const uint8_t _pin;            // output pin number
    const T_PIO_INSTANCE _pio; // holds instance for multi channel support

    uint32_t _endTime;       // Latch timing reference

    // Holds data stream which include LED color values and other settings as needed
    uint8_t*  _dataEditing;   // exposed for get and set
    uint8_t*  _dataSending;   // used for async send using DMA

    // holds pio state
    int _sm;
    int _dmaChannel;

    static typeof(this) s_dmaIrqObjectTable[c_DmaChannelsCount] = { nullptr };

    void construct()
    {
        _dataEditing = static_cast<uint8_t*>(malloc(_sizeData));
        // data cleared later in Begin() with a ClearTo(0)

        _dataSending = static_cast<uint8_t*>(malloc(_sizeData));
        // no need to initialize it, it gets overwritten on every send
    }

    void dmaCallback()
    {
        // save EOD time for latch on next call
        _endTime = micros();
    }

    static void dmaFinishIrq()
    {
        for (int dmaChannel = 0; dmaChannel < c_DmaChannelsCount; dmaChannel++)
        {
            if (s_dmaIrqObjectTable[dmaChannel])
            {
                if (dma_irqn_get_channel_status(V_IRQNUM, dmaChannel))
                {
                    dma_irqn_acknowledge_channel(V_IRQNUM, dmaChannel);
                    s_dmaIrqObjectTable[dmaChannel]->dmaCallback();
                }
            }
        }
    }
};

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
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2811, NeoRp2040PioInstanceN, true> Rp2040NWs2811InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2812x, NeoRp2040PioInstanceN, true> Rp2040NWs2812xInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2812x, NeoRp2040PioInstanceN, true> Rp2040NWs2816InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedSk6812, NeoRp2040PioInstanceN, true> Rp2040NSk6812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1814, NeoRp2040PioInstanceN> Rp2040NTm1814InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1829, NeoRp2040PioInstanceN> Rp2040NTm1829InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1914, NeoRp2040PioInstanceN> Rp2040NTm1914InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedApa106, NeoRp2040PioInstanceN, true> Rp2040NApa106InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTx1812, NeoRp2040PioInstanceN, true> Rp2040NTx1812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedGs1903, NeoRp2040PioInstanceN, true> Rp2040NGs1903InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeed800Kbps, NeoRp2040PioInstanceN, true> Rp2040N800KbpsInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeed400Kbps, NeoRp2040PioInstanceN, true> Rp2040N400KbpsInvertedMethod;

typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2811, NeoRp2040PioInstance0, true> Rp2040Pio0Ws2811InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2812x, NeoRp2040PioInstance0, true> Rp2040Pio0Ws2812xInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2812x, NeoRp2040PioInstance0, true> Rp2040Pio0Ws2816InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedSk6812, NeoRp2040PioInstance0, true> Rp2040Pio0Sk6812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1814, NeoRp2040PioInstance0> Rp2040Pio0Tm1814InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1829, NeoRp2040PioInstance0> Rp2040Pio0Tm1829InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1914, NeoRp2040PioInstance0> Rp2040Pio0Tm1914InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedApa106, NeoRp2040PioInstance0, true> Rp2040Pio0Apa106InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTx1812, NeoRp2040PioInstance0, true> Rp2040Pio0Tx1812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedGs1903, NeoRp2040PioInstance0, true> Rp2040Pio0Gs1903InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeed800Kbps, NeoRp2040PioInstance0, true> Rp2040Pio0800KbpsInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeed400Kbps, NeoRp2040PioInstance0, true> Rp2040Pio0400KbpsInvertedMethod;

typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2811, NeoRp2040PioInstance1, true> Rp2040Pio1Ws2811InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2812x, NeoRp2040PioInstance1, true> Rp2040Pio1Ws2812xInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedWs2812x, NeoRp2040PioInstance1, true> Rp2040Pio1Ws2816InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedSk6812, NeoRp2040PioInstance1, true> Rp2040Pio1Sk6812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1814, NeoRp2040PioInstance1> Rp2040Pio1Tm1814InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1829, NeoRp2040PioInstance1> Rp2040Pio1Tm1829InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTm1914, NeoRp2040PioInstance1> Rp2040Pio1Tm1914InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedApa106, NeoRp2040PioInstance1, true> Rp2040Pio1Apa106InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedTx1812, NeoRp2040PioInstance1, true> Rp2040Pio1Tx1812InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeedGs1903, NeoRp2040PioInstance1, true> Rp2040Pio1Gs1903InvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeed800Kbps, NeoRp2040PioInstance1, true> Rp2040Pio1800KbpsInvertedMethod;
typedef NeoRp2040PioMethodBase<NeoRp2040PioInvertedSpeed400Kbps, NeoRp2040PioInstance1, true> Rp2040Pio1400KbpsInvertedMethod;

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
