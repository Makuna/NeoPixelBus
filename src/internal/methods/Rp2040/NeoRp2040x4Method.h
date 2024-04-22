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

#if defined(ARDUINO_ARCH_RP2040)

//#define NEORP2040_DEBUG 

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/mutex.h"

#include "NeoRp2040DmaState.h"
#include "NeoRp2040PioMonoProgram.h"
#include "NeoRp2040PioInstance.h"
#include "NeoRp2040PioSpeed.h"


// Method
// --------------------------------------------------------
template<typename T_SPEED, 
        typename T_PIO_INSTANCE, 
        bool V_INVERT = false, 
        uint V_IRQ_INDEX = 1> 
class NeoRp2040x4MethodBase
{
public:
    typedef NeoNoSettings SettingsObject;

    NeoRp2040x4MethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize)  :
        _pin(pin),
        _sizeData(pixelCount * elementSize + settingsSize),
        _pixelCount(pixelCount),
        _mergedFifoCount((_pio.Instance->dbg_cfginfo & PIO_DBG_CFGINFO_FIFO_DEPTH_BITS) * 2) // merged TX / RX FIFO buffer in words
    {
        construct();
    }

    NeoRp2040x4MethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize, size_t settingsSize, NeoBusChannel channel) :
        _pin(pin),
        _sizeData(pixelCount* elementSize + settingsSize),
        _pixelCount(pixelCount),
        _pio(channel),
        _mergedFifoCount((_pio.Instance->dbg_cfginfo& PIO_DBG_CFGINFO_FIFO_DEPTH_BITS) * 2) // merged TX / RX FIFO buffer in words
    {
        construct();
    }

    ~NeoRp2040x4MethodBase()
    {
        // wait for last send
        while (!IsReadyToUpdate())
        {
            yield();
        }

        // clear any remaining just to be extra sure
        pio_sm_clear_fifos(_pio.Instance, _sm); 

        // disable the state machine
        pio_sm_set_enabled(_pio.Instance, _sm, false);

        // Disable and remove interrupts
        // dma_channel_cleanup(_dmaChannel); // NOT PRESENT?!
        dma_irqn_set_channel_enabled(V_IRQ_INDEX, _dmaChannel, false);

        // unregister static dma callback object
        _dmaState.Unregister(_dmaChannel);

        // unclaim dma channel and then state machine
        dma_channel_unclaim(_dmaChannel);
        pio_sm_unclaim(_pio.Instance, _sm);

        pinMode(_pin, INPUT);

        free(_dataSending);
    }

    bool IsReadyToUpdate() const
    {
        return _dmaState.IsReadyToSend(T_SPEED::ResetTimeUs + _fifoCacheEmptyDelta);
    }

    void Initialize()
    {
        // Select the largest FIFO fetch size that aligns with our data size
        // BUT, since RP2040 is little endian, if the source element size is
        // 8 bits, then the larger shift bits accounts for endianess
        // and will mangle the order thinking its source was a 16/32 bits
        // must you use channel_config_set_bswap() to address this, see below
        uint fifoWordBits = 8; // size of a FIFO word in bits
        
        if (_sizeData % 4 == 0)
        {
            // data is 4 byte aligned in size,
            // use a 32 bit fifo word for effeciency
            fifoWordBits = 32;
        }
        else if (_sizeData % 2 == 0)
        {
            // data is 2 byte aligned in size,
            // use a 16 bit fifo word for effeciency
            fifoWordBits = 16;
        }
        
        // calc the two related values from fifoWordBits
        dma_channel_transfer_size dmaTransferSize = static_cast<dma_channel_transfer_size>(fifoWordBits / 16);
        uint dmaTransferCount = _sizeData / (fifoWordBits / 8);;

        // IRQ triggers on DMA buffer finished, 
        // not the FIFO buffer finished sending, 
        // so we calc this delta so it can be added to the reset time
        //
 
        // 1000000.0f / T_SPEED::BitRateHz = us to send one bit
        float bitLengthUs = 1000000.0f / T_SPEED::BitRateHz;

        // _mergedFifoCount is merged TX/RX FIFO buffer in words
        // Add another word for any IRQ trigger latency (error) as 
        // too short is catastrophic and too long is fine
        _fifoCacheEmptyDelta = bitLengthUs * fifoWordBits * (_mergedFifoCount + 1);

#if defined(NEORP2040_DEBUG)

Serial.print(", _pio.Instance = ");
Serial.print((_pio.Instance == pio1));
Serial.print(", _sizeData = ");
Serial.print(_sizeData);
Serial.print(", dmaTransferSize = ");
Serial.print(dmaTransferSize);
Serial.print(", dmaTransferCount = ");
Serial.print(dmaTransferCount);
Serial.print(", fifoWordBits = ");
Serial.print(fifoWordBits);
Serial.print(", _mergedFifoCount = ");
Serial.print(_mergedFifoCount);
Serial.print(", _fifoCacheEmptyDelta = ");
Serial.print(_fifoCacheEmptyDelta);

#endif

        // Our assembled program needs to be loaded into this PIO's instruction
        // memory. This SDK function will find a location (offset) in the
        // instruction memory where there is enough space for our program. We need
        //
        uint offset = T_SPEED::add(_pio.Instance);

#if defined(NEORP2040_DEBUG)

Serial.println();
Serial.print("offset = ");
Serial.print(offset);

#endif

        // Find a free state machine on our chosen PIO. 
        _sm = pio_claim_unused_sm(_pio.Instance, true); // panic if none available

#if defined(NEORP2040_DEBUG)

Serial.print(", _sm = ");
Serial.print(_sm);

#endif

        // Configure it to run our program, and start it, using the
        // helper function we included in our .pio file.
        T_SPEED::init(_pio.Instance, 
            _sm, 
            offset, 
            _pin, 
            T_SPEED::BitRateHz, 
            fifoWordBits);
        
        // invert output if needed
        if (V_INVERT)
        {
            gpio_set_outover(_pin, GPIO_OVERRIDE_INVERT);
        }

        // find a free dma channel
        _dmaChannel = dma_claim_unused_channel(true); // panic if none available

#if defined(NEORP2040_DEBUG)

Serial.print(", *_dmaChannel = ");
Serial.print(_dmaChannel);
Serial.println();

#endif

        // register for IRQ shared static endTime updates
        _dmaState.Register(_dmaChannel);

        // Set up DMA transfer
        dma_channel_config dmaConfig = dma_channel_get_default_config(_dmaChannel);
        channel_config_set_transfer_data_size(&dmaConfig, dmaTransferSize);
        channel_config_set_read_increment(&dmaConfig, true);
        channel_config_set_write_increment(&dmaConfig, false);
        // source is byte data stream, even with 16/32 transfer size
        channel_config_set_bswap(&dmaConfig, true); 

        // Set DMA trigger
        channel_config_set_dreq(&dmaConfig, pio_get_dreq(_pio.Instance, _sm, true));

        dma_channel_configure(_dmaChannel, 
            &dmaConfig,
            &(_pio.Instance->txf[_sm]),  // dest
            _dataSending,                // src
            dmaTransferCount,
            false);

        dma_irqn_set_channel_enabled(V_IRQ_INDEX, _dmaChannel, true);
    }

    template <typename T_COLOR_OBJECT,
        typename T_COLOR_FEATURE,
        typename T_SHADER>
    void Update(
        T_COLOR_OBJECT* pixels,
        size_t countPixels,
        const typename T_COLOR_FEATURE::SettingsObject& featureSettings,
        const T_SHADER& shader)
    {
        // wait for last send
        while (!IsReadyToUpdate())
        {
            yield();  
        }
  
        const size_t sendDataSize = T_COLOR_FEATURE::SettingsSize >= T_COLOR_FEATURE::PixelSize ? T_COLOR_FEATURE::SettingsSize : T_COLOR_FEATURE::PixelSize;
        uint8_t sendData[sendDataSize];
        uint8_t* transaction = this->BeginTransaction();

        // if there are settings at the front
        //
        if (T_COLOR_FEATURE::applyFrontSettings(sendData, sendDataSize, featureSettings))
        {
            this->AppendData(&transaction, sendData, T_COLOR_FEATURE::SettingsSize);
        }

        // fill primary color data
        //
        T_COLOR_OBJECT* pixel = pixels;
        const T_COLOR_OBJECT* pixelEnd = pixel + countPixels;
        uint16_t stripCount = this->_pixelCount;

        while (stripCount--)
        {
            typename T_COLOR_FEATURE::ColorObject color = shader.Apply(*pixel);
            T_COLOR_FEATURE::applyPixelColor(sendData, sendDataSize, color);

            this->AppendData(&transaction, sendData, T_COLOR_FEATURE::PixelSize);

            pixel++;
            if (pixel >= pixelEnd)
            {
                // restart at first
                pixel = pixels;
            }
        }

        // if there are settings at the back
        //
        if (T_COLOR_FEATURE::applyBackSettings(sendData, sendDataSize, featureSettings))
        {
            this->AppendData(&transaction, sendData, T_COLOR_FEATURE::SettingsSize);
        }

        this->EndTransaction();
    }

    uint8_t* BeginTransaction()
    {
        return _dataSending;
    }

    bool AppendData(uint8_t** buffer, const uint8_t* sendData, size_t sendDataSize)
    {
        memcpy(*buffer, sendData, sendDataSize);
        *buffer += sendDataSize;
        return true;
    }

    bool EndTransaction()
    {
        _dmaState.SetSending();

        // start next send
        // 
        dma_channel_set_read_addr(_dmaChannel, _dataSending, false);
        dma_channel_start(_dmaChannel); // Start new transfer

        return true;
    }

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
    }

private:
    const uint8_t _pin;          // output pin number
    const size_t  _sizeData;     // Size of '_data*' buffers 
    const uint16_t _pixelCount; // count of pixels in the strip

    const T_PIO_INSTANCE _pio;   // holds instance for multi channel support
    const uint8_t _mergedFifoCount;

    NeoRp2040DmaState<V_IRQ_INDEX> _dmaState;   // Latch timing reference
    uint32_t _fifoCacheEmptyDelta; // delta between dma IRQ finished and PIO Fifo empty

    // Holds data stream which include LED color values and other settings as needed
    uint8_t*  _dataSending;   // used for async send using DMA

    // holds pio state
    int _sm;
    int _dmaChannel;

    void construct()
    {
        _dataSending = static_cast<uint8_t*>(malloc(_sizeData));
        // no need to initialize it, it gets overwritten on every send
    }
};

// normal
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstanceN> Rp2040x4NWs2811Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstanceN> Rp2040x4NWs2812xMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstanceN> Rp2040x4NWs2816Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2805, NeoRp2040PioInstanceN> Rp2040x4NWs2805Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstanceN> Rp2040x4NSk6812Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstanceN, true> Rp2040x4NTm1814Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstanceN, true> Rp2040x4NTm1829Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstanceN, true> Rp2040x4NTm1914Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstanceN> Rp2040x4NApa106Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstanceN> Rp2040x4NTx1812Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstanceN> Rp2040x4NGs1903Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstanceN> Rp2040x4N800KbpsMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstanceN> Rp2040x4N400KbpsMethod;
typedef Rp2040x4NWs2805Method Rp2040x4NWs2814Method;

typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstance0> Rp2040x4Pio0Ws2811Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance0> Rp2040x4Pio0Ws2812xMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance0> Rp2040x4Pio0Ws2816Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2805, NeoRp2040PioInstance0> Rp2040x4Pio0Ws2805Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstance0> Rp2040x4Pio0Sk6812Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstance0, true> Rp2040x4Pio0Tm1814Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstance0, true> Rp2040x4Pio0Tm1829Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstance0, true> Rp2040x4Pio0Tm1914Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstance0> Rp2040x4Pio0Apa106Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstance0> Rp2040x4Pio0Tx1812Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstance0> Rp2040x4Pio0Gs1903Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstance0> Rp2040x4Pio0800KbpsMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstance0> Rp2040x4Pio0400KbpsMethod;
typedef Rp2040x4Pio0Ws2805Method Rp2040x4Pio0Ws2814Method;

typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstance1> Rp2040x4Pio1Ws2811Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance1> Rp2040x4Pio1Ws2812xMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance1> Rp2040x4Pio1Ws2816Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2805, NeoRp2040PioInstance1> Rp2040x4Pio1Ws2805Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstance1> Rp2040x4Pio1Sk6812Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstance1, true> Rp2040x4Pio1Tm1814Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstance1, true> Rp2040x4Pio1Tm1829Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstance1, true> Rp2040x4Pio1Tm1914Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstance1> Rp2040x4Pio1Apa106Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstance1> Rp2040x4Pio1Tx1812Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstance1> Rp2040x4Pio1Gs1903Method;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstance1> Rp2040x4Pio1800KbpsMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstance1> Rp2040x4Pio1400KbpsMethod;
typedef Rp2040x4Pio1Ws2805Method Rp2040x4Pio1Ws2814Method;

// inverted
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstanceN, true> Rp2040x4NWs2811InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstanceN, true> Rp2040x4NWs2812xInvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstanceN, true> Rp2040x4NWs2816InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2805, NeoRp2040PioInstanceN, true> Rp2040x4NWs2805InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstanceN, true> Rp2040x4NSk6812InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstanceN> Rp2040x4NTm1814InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstanceN> Rp2040x4NTm1829InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstanceN> Rp2040x4NTm1914InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstanceN, true> Rp2040x4NApa106InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstanceN, true> Rp2040x4NTx1812InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstanceN, true> Rp2040x4NGs1903InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstanceN, true> Rp2040x4N800KbpsInvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstanceN, true> Rp2040x4N400KbpsInvertedMethod;
typedef Rp2040x4NWs2805InvertedMethod Rp2040x4NWs2814InvertedMethod;

typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstance0, true> Rp2040x4Pio0Ws2811InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance0, true> Rp2040x4Pio0Ws2812xInvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance0, true> Rp2040x4Pio0Ws2816InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2805, NeoRp2040PioInstance0, true> Rp2040x4Pio0Ws2805InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstance0, true> Rp2040x4Pio0Sk6812InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstance0> Rp2040x4Pio0Tm1814InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstance0> Rp2040x4Pio0Tm1829InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstance0> Rp2040x4Pio0Tm1914InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstance0, true> Rp2040x4Pio0Apa106InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstance0, true> Rp2040x4Pio0Tx1812InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstance0, true> Rp2040x4Pio0Gs1903InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstance0, true> Rp2040x4Pio0800KbpsInvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstance0, true> Rp2040x4Pio0400KbpsInvertedMethod;
typedef Rp2040x4Pio0Ws2805InvertedMethod Rp2040x4Pio0Ws2814InvertedMethod;

typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2811, NeoRp2040PioInstance1, true> Rp2040x4Pio1Ws2811InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance1, true> Rp2040x4Pio1Ws2812xInvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2812x, NeoRp2040PioInstance1, true> Rp2040x4Pio1Ws2816InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedWs2805, NeoRp2040PioInstance1, true> Rp2040x4Pio1Ws2805InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedSk6812, NeoRp2040PioInstance1, true> Rp2040x4Pio1Sk6812InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1814, NeoRp2040PioInstance1> Rp2040x4Pio1Tm1814InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1829, NeoRp2040PioInstance1> Rp2040x4Pio1Tm1829InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTm1914, NeoRp2040PioInstance1> Rp2040x4Pio1Tm1914InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedApa106, NeoRp2040PioInstance1, true> Rp2040x4Pio1Apa106InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedTx1812, NeoRp2040PioInstance1, true> Rp2040x4Pio1Tx1812InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeedGs1903, NeoRp2040PioInstance1, true> Rp2040x4Pio1Gs1903InvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed800Kbps, NeoRp2040PioInstance1, true> Rp2040x4Pio1800KbpsInvertedMethod;
typedef NeoRp2040x4MethodBase<NeoRp2040PioSpeed400Kbps, NeoRp2040PioInstance1, true> Rp2040x4Pio1400KbpsInvertedMethod;
typedef Rp2040x4Pio1Ws2805InvertedMethod Rp2040x4Pio1Ws2814InvertedMethod;

// PIO 1 method is the default method, and still x4 instances 
typedef Rp2040x4Pio1Ws2812xMethod NeoWs2813Method;
typedef Rp2040x4Pio1Ws2812xMethod NeoWs2812xMethod;
typedef Rp2040x4Pio1800KbpsMethod NeoWs2812Method;
typedef Rp2040x4Pio1Ws2812xMethod NeoWs2811Method;
typedef Rp2040x4Pio1Ws2812xMethod NeoWs2816Method;
typedef Rp2040x4Pio1Ws2805Method NeoWs2805Method;
typedef Rp2040x4Pio1Ws2814Method NeoWs2814Method;
typedef Rp2040x4Pio1Sk6812Method NeoSk6812Method;
typedef Rp2040x4Pio1Tm1814Method NeoTm1814Method;
typedef Rp2040x4Pio1Tm1829Method NeoTm1829Method;
typedef Rp2040x4Pio1Tm1914Method NeoTm1914Method;
typedef Rp2040x4Pio1Sk6812Method NeoLc8812Method;
typedef Rp2040x4Pio1Apa106Method NeoApa106Method;
typedef Rp2040x4Pio1Tx1812Method NeoTx1812Method;
typedef Rp2040x4Pio1Gs1903Method NeoGs1903Method;

typedef Rp2040x4Pio1Ws2812xMethod Neo800KbpsMethod;
typedef Rp2040x4Pio1400KbpsMethod Neo400KbpsMethod;

typedef Rp2040x4Pio1Ws2812xInvertedMethod NeoWs2813InvertedMethod;
typedef Rp2040x4Pio1Ws2812xInvertedMethod NeoWs2812xInvertedMethod;
typedef Rp2040x4Pio1Ws2812xInvertedMethod NeoWs2811InvertedMethod;
typedef Rp2040x4Pio1800KbpsInvertedMethod NeoWs2812InvertedMethod;
typedef Rp2040x4Pio1Ws2812xInvertedMethod NeoWs2816InvertedMethod;
typedef Rp2040x4Pio1Ws2805InvertedMethod NeoWs2805InvertedMethod;
typedef Rp2040x4Pio1Ws2814InvertedMethod NeoWs2814InvertedMethod;
typedef Rp2040x4Pio1Sk6812InvertedMethod NeoSk6812InvertedMethod;
typedef Rp2040x4Pio1Tm1814InvertedMethod NeoTm1814InvertedMethod;
typedef Rp2040x4Pio1Tm1829InvertedMethod NeoTm1829InvertedMethod;
typedef Rp2040x4Pio1Tm1914InvertedMethod NeoTm1914InvertedMethod;
typedef Rp2040x4Pio1Sk6812InvertedMethod NeoLc8812InvertedMethod;
typedef Rp2040x4Pio1Apa106InvertedMethod NeoApa106InvertedMethod;
typedef Rp2040x4Pio1Tx1812InvertedMethod NeoTx1812InvertedMethod;
typedef Rp2040x4Pio1Gs1903InvertedMethod NeoGs1903InvertedMethod;

typedef Rp2040x4Pio1Ws2812xInvertedMethod Neo800KbpsInvertedMethod;
typedef Rp2040x4Pio1400KbpsInvertedMethod Neo400KbpsInvertedMethod;


#endif
