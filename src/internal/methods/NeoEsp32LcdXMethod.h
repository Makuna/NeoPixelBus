#pragma once

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)

extern "C"
{
#include <driver/periph_ctrl.h>
#include <esp_private/gdma.h>
#include <esp_rom_gpio.h>
#include <hal/dma_types.h>
#include <hal/gpio_hal.h>
#include <soc/lcd_cam_struct.h>
}

//
// true size of mux channel, 8 bit
//
class NeoEspLcdMuxBusSize8Bit
{
public:
    NeoEspLcdMuxBusSize8Bit() {};

    const static size_t MuxBusDataSize = 1;

    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
        uint8_t* pDma = dmaBuffer;
        const uint8_t* pEnd = data + sizeData;

        for (const uint8_t* pValue = data; pValue < pEnd; pValue++)
        {
            uint8_t value = *pValue;

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                // Get what's already there (offset 1)
                uint8_t dmaVal = *(pDma + 1);

                // Adjust
                dmaVal |= (value & 0x80) ? (0x01 << muxId) : 0x00;

                // Write it back
                *(pDma++) = 0xFF;
                *(pDma++) = dmaVal;
                *(pDma++) = 0x00;

                // Next
                value <<= 1;
            }
        }
    }
};

//
// true size of mux channel, 16 bit
//
class NeoEspLcdMuxBusSize16Bit
{
public:
    NeoEspLcdMuxBusSize16Bit() {};

    const static size_t MuxBusDataSize = 2;

    // sizeData = 135 confirmed
    // (value) at each point is valid, last 3 bytes are 0 but that should be fine, all the rest is legit
    // *dmaBuffer is valid because the start of the line is GREEN as expected
    static void EncodeIntoDma(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId)
    {
        uint8_t* pDma = dmaBuffer;
        const uint8_t* pEnd = data + sizeData;

        for (const uint8_t* pValue = data; pValue < pEnd; pValue++)
        {
            uint8_t value = *pValue;

            for (uint8_t bit = 0; bit < 8; bit++)
            {
                // Get what's already there
                uint8_t dmaVal1 = *(pDma + 2);
                uint8_t dmaVal2 = *(pDma + 3);

                // Adjust
                if (value & 0x80) {
                    if (muxId < 8) {
                        dmaVal1 |= 0x01 << muxId;
                    } else {
                        dmaVal2 |= 0x01 << (muxId - 8);
                    }
                }

                // Write it back
                *(pDma++) = 0xFF;
                *(pDma++) = 0xFF;
                *(pDma++) = dmaVal1;
                *(pDma++) = dmaVal2;
                *(pDma++) = 0x00;
                *(pDma++) = 0x00;

                // Next
                value <<= 1;
            }
        }
    }
};

//
// tracks mux channels used and if updated
// 
// T_FLAG - type used to store bit flags, UINT8_t for 8 channels, UINT16_t for 16
// T_MUXSIZE - true size of mux channel = NeoEspLcdMuxBusSize8Bit or NeoEspLcdMuxBusSize16Bit
//
template<typename T_FLAG, typename T_MUXSIZE> 
class NeoEspLcdMuxMap : public T_MUXSIZE
{
public:
    const static uint8_t InvalidMuxId = -1;
    const static size_t BusMaxCount = sizeof(T_FLAG) * 8;

    size_t MaxBusDataSize; // max size of stream data from any single mux bus
    T_FLAG UpdateMap;     // bitmap flags of mux buses to track update state
    T_FLAG UpdateMapMask; // mask to used bits in s_UpdateMap
    T_FLAG BusCount;      // count of mux buses

    // as a static instance, all members get initialized to zero
    // and the constructor is called at inconsistent time to other globals
    // so its not useful to have or rely on, 
    // but without it presence they get zeroed far too late
    NeoEspLcdMuxMap() 
    //    //:
    //    //MaxBusDataSize(0),
    //    //UpdateMap(0),
    //    //UpdateMapMask(0),
    //    //BusCount(0)
    {
    }

    uint8_t RegisterNewMuxBus(const size_t dataSize)
    {
        // find first available bus id
        uint8_t muxId = 0;
        while (muxId < BusMaxCount)
        {
            T_FLAG muxIdField = (1 << muxId);
            if ((UpdateMapMask & muxIdField) == 0)
            {
                // complete registration
                BusCount++;
                UpdateMapMask |= muxIdField;
                if (dataSize > MaxBusDataSize)
                {
                    MaxBusDataSize = dataSize;
                }
                break;
            }
            muxId++;
        }
        if (muxId == BusMaxCount)
        {
            log_e("exceded channel limit of %u on bus", BusMaxCount);
        }
        return muxId;
    }


    bool DeregisterMuxBus(uint8_t muxId)
    {
        T_FLAG muxIdField = (1 << muxId);
        if (UpdateMapMask & muxIdField)
        {
            // complete deregistration
            BusCount--;
            UpdateMapMask &= ~muxIdField;
            if (UpdateMapMask == 0)
            {
                return true;
            }
        }
        return false;
    }

    bool IsAllMuxBusesUpdated()
    {
        return (UpdateMap == UpdateMapMask);
    }

    bool IsNoMuxBusesUpdate()
    {
        return (UpdateMap == 0);
    }

    void MarkMuxBusUpdated(uint8_t muxId)
    {
        UpdateMap |= (1 << muxId);
    }

    void ResetMuxBusesUpdated()
    {
        UpdateMap = 0;
    }

    void Reset()
    {
        MaxBusDataSize = 0;
        UpdateMap = 0;
        UpdateMapMask = 0;
        BusCount = 0;
    }
};

static IRAM_ATTR bool dma_callback(gdma_channel_handle_t dma_chan,
                                   gdma_event_data_t *event_data,
                                   void *user_data) {
  esp_rom_delay_us(5);
  LCD_CAM.lcd_user.lcd_start = 0;
  return true;
}

//
// Implementation of a Single Buffered version of a LcdContext
// Manages the underlying I2S details including the buffer
// This creates only a actively sending back buffer, 
// Note that the back buffer must be DMA memory, a limited resource
// 
// T_MUXMAP - NeoEspLcdMuxMap - tracking class for mux state
//
template<typename T_MUXMAP> 
class NeoEspLcdMonoBuffContext 
{
public:
    // Each data bit requires 3 DMA byes
    const static size_t DmaBytesPerPixelByte = 24 * T_MUXMAP::MuxBusDataSize;

    size_t LcdBufferSize; // total size of LcdBuffer
    uint8_t* LcdBuffer;    // holds the DMA buffer that is referenced by LcdBufDesc
    size_t DmaBufferSize;
    uint8_t* DmaBuffer;     // holds the pointer to the aligned part of the LCD buffer for DMA
    T_MUXMAP MuxMap;
    dma_descriptor_t* desc;
    gdma_channel_handle_t dma_chan;

    // as a static instance, all members get initialized to zero
    // and the constructor is called at inconsistent time to other globals
    // so its not useful to have or rely on, 
    // but without it presence they get zeroed far too late
    NeoEspLcdMonoBuffContext()
        //:
        //LcdBufferSize(0),
        //LcdBuffer(nullptr),
        //LcdEditBuffer(nullptr),
        //MuxMap()
    {
    }

    void Construct()
    {
        // construct only once on first time called
        if (LcdBuffer == nullptr)
        {
            uint32_t DmaBufferSize = MuxMap.MaxBusDataSize * DmaBytesPerPixelByte;
            uint32_t buf_size = DmaBufferSize + 3;        // +3 for long align
            int num_desc = (DmaBufferSize + 4094) / 4095; // sic. (NOT 4096)
            uint32_t alloc_size =
                num_desc * sizeof(dma_descriptor_t) + buf_size;

            LcdBufferSize = alloc_size;

            LcdBuffer = static_cast<uint8_t*>(heap_caps_malloc(LcdBufferSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT));
            if (LcdBuffer == nullptr)
            {
                log_e("send buffer memory allocation failure (size %u)",
                    LcdBufferSize);
            }
            memset(LcdBuffer, 0x00, LcdBufferSize);

            // Find first 32-bit aligned address following descriptor list
            uint32_t *alignedAddr =
                (uint32_t *)((uint32_t)(&LcdBuffer[num_desc * sizeof(dma_descriptor_t) + 3]) & ~3);
            uint8_t *dmaBuf = (uint8_t *)alignedAddr;
            DmaBuffer = dmaBuf;

            // LCD_CAM isn't enabled by default -- MUST begin with this:
            periph_module_enable(PERIPH_LCD_CAM_MODULE);
            periph_module_reset(PERIPH_LCD_CAM_MODULE);

            // Reset LCD bus
            LCD_CAM.lcd_user.lcd_reset = 1;
            esp_rom_delay_us(100);

            // Configure LCD clock
            LCD_CAM.lcd_clock.clk_en = 1;             // Enable clock
            LCD_CAM.lcd_clock.lcd_clk_sel = 2;        // PLL240M source
            LCD_CAM.lcd_clock.lcd_clkm_div_a = 1;     // 1/1 fractional divide,
            LCD_CAM.lcd_clock.lcd_clkm_div_b = 1;     // plus '99' below yields...
            LCD_CAM.lcd_clock.lcd_clkm_div_num = 99;  // 1:100 prescale (2.4 MHz CLK)
            LCD_CAM.lcd_clock.lcd_ck_out_edge = 0;    // PCLK low in 1st half cycle
            LCD_CAM.lcd_clock.lcd_ck_idle_edge = 0;   // PCLK low idle
            LCD_CAM.lcd_clock.lcd_clk_equ_sysclk = 1; // PCLK = CLK (ignore CLKCNT_N)

            // Configure frame format
            LCD_CAM.lcd_ctrl.lcd_rgb_mode_en = 0;    // i8080 mode (not RGB)
            LCD_CAM.lcd_rgb_yuv.lcd_conv_bypass = 0; // Disable RGB/YUV converter
            LCD_CAM.lcd_misc.lcd_next_frame_en = 0;  // Do NOT auto-frame
            LCD_CAM.lcd_data_dout_mode.val = 0;      // No data delays
            LCD_CAM.lcd_user.lcd_always_out_en = 1;  // Enable 'always out' mode
            LCD_CAM.lcd_user.lcd_8bits_order = 0;    // Do not swap bytes
            LCD_CAM.lcd_user.lcd_bit_order = 0;      // Do not reverse bit order
            LCD_CAM.lcd_user.lcd_2byte_en = T_MUXMAP::MuxBusDataSize > 1 ? 1 : 0;
            LCD_CAM.lcd_user.lcd_dummy = 1;          // Dummy phase(s) @ LCD start
            LCD_CAM.lcd_user.lcd_dummy_cyclelen = 0; // 1 dummy phase
            LCD_CAM.lcd_user.lcd_cmd = 0;            // No command at LCD start
            // Dummy phase(s) MUST be enabled for DMA to trigger reliably.

            // Set up DMA descriptor list (length and data are set before xfer)
            desc = (dma_descriptor_t *)LcdBuffer; // At start of alloc'd buffer
            for (int i = 0; i < num_desc; i++) {
                desc[i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
                desc[i].dw0.suc_eof = 0;
                desc[i].next = &desc[i + 1];
            }
            desc[num_desc - 1].dw0.suc_eof = 1;
            desc[num_desc - 1].next = NULL;

            // Alloc DMA channel & connect it to LCD periph
            gdma_channel_alloc_config_t dma_chan_config = {
                .sibling_chan = NULL,
                .direction = GDMA_CHANNEL_DIRECTION_TX,
                .flags = {.reserve_sibling = 0}};
            gdma_new_channel(&dma_chan_config, &dma_chan);
            gdma_connect(dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
            gdma_strategy_config_t strategy_config = {.owner_check = false,
                                                        .auto_update_desc = false};
            gdma_apply_strategy(dma_chan, &strategy_config);

            // Enable DMA transfer callback
            gdma_tx_event_callbacks_t tx_cbs = {.on_trans_eof = dma_callback};
            gdma_register_tx_event_callbacks(dma_chan, &tx_cbs, NULL);
        }
    }

    void Destruct()
    {
        if (LcdBuffer == nullptr)
        {
            return;
        }

        // TODO: destroy peripheral pin settings
        // lcdSetPins(busNumber, -1, -1, -1, false);
        // lcdDeinit(busNumber);

        gdma_reset(dma_chan);

        heap_caps_free(LcdBuffer);

        LcdBufferSize = 0;
        LcdBuffer = nullptr;
        DmaBufferSize = 0;
        DmaBuffer = nullptr;
        desc = nullptr;

        MuxMap.Reset();
    }

    void StartWrite()
    {
        if (MuxMap.IsAllMuxBusesUpdated())
        {
            MuxMap.ResetMuxBusesUpdated();
            
            gdma_reset(dma_chan);
            LCD_CAM.lcd_user.lcd_dout = 1;
            LCD_CAM.lcd_user.lcd_update = 1;
            LCD_CAM.lcd_misc.lcd_afifo_reset = 1;

            uint32_t xfer_size = MuxMap.MaxBusDataSize * DmaBytesPerPixelByte;
            int num_desc = (xfer_size + 4094) / 4095; // sic. (NOT 4096)
            int bytesToGo = xfer_size;
            int offset = 0;
            for (int i = 0; i < num_desc; i++) {
                int bytesThisPass = bytesToGo;
                if (bytesThisPass > 4095)
                bytesThisPass = 4095;
                desc[i].dw0.size = desc[i].dw0.length = bytesThisPass;
                desc[i].buffer = &DmaBuffer[offset];
                bytesToGo -= bytesThisPass;
                offset += bytesThisPass;
            }

            gdma_start(dma_chan, (intptr_t)&desc[0]);
            esp_rom_delay_us(1);
            LCD_CAM.lcd_user.lcd_start = 1; 
        }
    }

    void FillBuffers(const uint8_t* data, 
            size_t sizeData, 
            uint8_t muxId)
    {
        // wait for not actively sending data
        while (LCD_CAM.lcd_user.lcd_start)
        {
            yield();
        }


        // to keep the inner loops for EncodeIntoDma smaller
        // they will just OR in their values
        // so the buffer must be cleared first
        if (MuxMap.IsNoMuxBusesUpdate())
        {
            // clear all the data in preperation for each mux channel to add
            memset(DmaBuffer, 0x00, DmaBufferSize);
        }

        MuxMap.EncodeIntoDma(DmaBuffer,
            data,
            sizeData,
            muxId);

        MuxMap.MarkMuxBusUpdated(muxId);
    }
};

//
// Implementation of the low level interface into lcd mux bus
// 
// T_BUSCONTEXT - the context to use, currently only NeoEspLcdDblBuffContext but there is
//      a plan to provide one that doesn't implement the front buffer but would be less
//      async as it would have to wait until the last frame was completely sent before
//      updating and new data
// T_BUS - the bus id, NeoEsp32LcdBusZero, NeoEsp32LcdBusOne
//
template<typename T_BUSCONTEXT> 
class NeoEsp32LcdMuxBus
{
public:    
    NeoEsp32LcdMuxBus() :
        _muxId(s_context.MuxMap.InvalidMuxId)
    {
    }

    void RegisterNewMuxBus(size_t dataSize)
    {
        _muxId = s_context.MuxMap.RegisterNewMuxBus(dataSize);
    }

    void Initialize(uint8_t pin)
    {
        s_context.Construct();
        
        uint8_t muxIdx = LCD_DATA_OUT0_IDX + _muxId;
        esp_rom_gpio_connect_out_signal(pin, muxIdx, false, false);
        gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
        gpio_set_drive_capability((gpio_num_t)pin, (gpio_drive_cap_t)3);
    }

    void DeregisterMuxBus(uint8_t pin)
    {
        if (s_context.MuxMap.DeregisterMuxBus(_muxId))
        {
            s_context.Destruct();
        }

        // disconnect muxed pin
        gpio_matrix_out(pin, SIG_GPIO_OUT_IDX, false, false);
        pinMode(pin, INPUT);

        _muxId = s_context.MuxMap.InvalidMuxId;
    }

    void StartWrite()
    {
        s_context.StartWrite();
    }

    bool IsWriteDone() const
    {
        bool busy = LCD_CAM.lcd_user.lcd_start;
        return !busy;
    }

    void FillBuffers(const uint8_t* data, size_t sizeData)
    {
        s_context.FillBuffers(data, sizeData, _muxId);
    }

    void MarkUpdated()
    {
        s_context.MuxMap.MarkMuxBusUpdated(_muxId);
    }

private:
    static T_BUSCONTEXT s_context;
    uint8_t _muxId; 
};

template<typename T_BUSCONTEXT> T_BUSCONTEXT NeoEsp32LcdMuxBus<T_BUSCONTEXT>::s_context = T_BUSCONTEXT();

//
// wrapping layer of the lcd mux bus as a NeoMethod
// 
// T_SPEED - NeoEsp32LcdSpeed* (ex NeoEsp32LcdSpeedWs2812x) used to define output signal form
// T_BUS - NeoEsp32LcdMuxBus, the bus to use
//
template<typename T_SPEED, typename T_BUS> 
class NeoEsp32LcdXMethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoEsp32LcdXMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizeData(pixelCount * elementSize + settingsSize),
        _pin(pin),
        _bus()
    {
        size_t numResetBytes = T_SPEED::ResetTimeUs / T_SPEED::ByteSendTimeUs;
        _bus.RegisterNewMuxBus(_sizeData + numResetBytes);        
    }

    ~NeoEsp32LcdXMethodBase()
    {
        while (!_bus.IsWriteDone())
        {
            yield();
        }

        _bus.DeregisterMuxBus(_pin);

        free(_data);
    }

    bool IsReadyToUpdate() const
    {
        return _bus.IsWriteDone();
    }

    void Initialize()
    {
        _bus.Initialize(_pin);

        _data = static_cast<uint8_t*>(malloc(_sizeData));
        if (_data == nullptr)
        {
            log_e("front buffer memory allocation failure");
        }
        // data cleared later in Begin()
    }

    void Update(bool)
    {
        _bus.FillBuffers(_data, _sizeData);
        _bus.StartWrite(); // only triggers actual write after all mux busses have updated
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called even if no changes to method buffer
        // as edit buffer is always cleared and then copied to send buffer and all
        // mux bus needs to included
        return true;
    }

    uint8_t* getData() const
    {
        return _data;
    };

    size_t getDataSize() const
    {
        return _sizeData;
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    const size_t  _sizeData;    // Size of '_data' buffer 
    const uint8_t _pin;         // output pin number

    T_BUS _bus;          // holds instance for mux bus support
    uint8_t* _data;      // Holds LED color values
};



typedef NeoEsp32LcdMuxBus<NeoEspLcdMonoBuffContext<NeoEspLcdMuxMap<uint8_t, NeoEspLcdMuxBusSize8Bit>>> NeoEsp32LcdMux8Bus;
typedef NeoEsp32LcdMuxBus<NeoEspLcdMonoBuffContext<NeoEspLcdMuxMap<uint16_t, NeoEspLcdMuxBusSize16Bit>>> NeoEsp32LcdMux16Bus;

class NeoEsp32LcdSpeedWs2812x
{
public:
    // Used to calculate how many bytes in a reset pulse
    const static uint16_t ByteSendTimeUs = 10;
    const static uint16_t ResetTimeUs = 300;
};

typedef NeoEsp32LcdXMethodBase<NeoEsp32LcdSpeedWs2812x, NeoEsp32LcdMux8Bus> NeoEsp32LcdX8Ws2812xMethod;
typedef NeoEsp32LcdXMethodBase<NeoEsp32LcdSpeedWs2812x, NeoEsp32LcdMux16Bus> NeoEsp32LcdX16Ws2812xMethod;


#endif // defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32S3)
