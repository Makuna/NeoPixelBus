/*-------------------------------------------------------------------------
NeoPixel library helper functions for HD108 using PARLIO peripheral on compatible ESP32 chips

Written by Tom Magnier, adapted from DotStarEsp32DmaSpiMethod

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

// Available on ESP32-C5 / C6 / H2 / H4 / P4
#if defined(ARDUINO_ARCH_ESP32) && defined(SOC_PARLIO_SUPPORTED)

#pragma once

/*  General Reference documentation for the APIs used in this implementation
LOW LEVEL:  (what is actually used)
DOCS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-reference/peripherals/parlio/parlio_tx.html
*/

extern "C"
{
#include <driver/parlio_tx.h>
}

template<typename T_SPEED> class Hd108Esp32ParlioMethodBase
{
public:
    typedef typename T_SPEED::SettingsObject SettingsObject;

    Hd108Esp32ParlioMethodBase(uint8_t clockPin, uint8_t dataPin, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _clockPin((gpio_num_t)clockPin),
        _dataPin((gpio_num_t)dataPin),
        _sizeStartFrame(16), // 128 bits (16 bytes)
        _sizePixelData(pixelCount * elementSize + settingsSize),
        _sizeEndFrame((pixelCount + 7) / 8 < 8 ? 8 : (pixelCount + 7) / 8) // one clock per bit, no less than 8 bytes
    {

        _bufferSize = _sizeStartFrame + _sizePixelData + _sizeEndFrame;

        // must have a 4 byte aligned buffer for DMA
        uint32_t alignment = _bufferSize % 4;
        if (alignment)
        {
            _bufferSize += 4 - alignment;
        }

        _data = static_cast<uint8_t*>(malloc(_bufferSize));
        _dmadata = static_cast<uint8_t*>(heap_caps_malloc(_bufferSize, MALLOC_CAP_DMA));

        // data cleared later in Begin()
    }

    ~Hd108Esp32ParlioMethodBase()
    {
        if (_parlio_tx_handle)
        {
            deinitParlio();
        }
        free(_data);
        heap_caps_free(_dmadata);
    }

    bool IsReadyToUpdate() const
    {
        esp_err_t ret = parlio_tx_unit_wait_all_done(_parlio_tx_handle, 0);

        return (ret == ESP_OK);
    }

    void Initialize()
    {
        memset(_data, 0x00, _sizeStartFrame); //Init start frame
        memset(_data + _sizeStartFrame + _sizePixelData, 0x00, _bufferSize - (_sizeStartFrame + _sizePixelData)); //Init end frame

        initParlio();
    }

    void Update(bool)
    {        
        while(!IsReadyToUpdate()) 
            portYIELD();

        memcpy(_dmadata, _data, _bufferSize);

        parlio_transmit_config_t transmit_config = {
            .idle_value = 0x00, // All data lines are low in idle state
        };
        ESP_ERROR_CHECK(parlio_tx_unit_transmit(_parlio_tx_handle, _dmadata, _bufferSize * 8, &transmit_config));
    }

    bool AlwaysUpdate()
    {
        // this method requires update to be called only if changes to buffer
        return false;
    }

    bool SwapBuffers()
    {
        return false;
    }

    uint8_t* getData() const
    {
        return _data + _sizeStartFrame;
    };

    size_t getDataSize() const
    {
        return _sizePixelData;
    };

    void applySettings([[maybe_unused]] const SettingsObject& settings)
    {
        _speed.applySettings(settings);
        if (_parlio_tx_handle)
        {
            deinitParlio();
            initParlio();
        }    
    }

private:
    void initParlio()
    {
        parlio_tx_unit_config_t config = {
            .clk_src = PARLIO_CLK_SRC_DEFAULT,      // Select the default clock source
            .clk_in_gpio_num = (gpio_num_t)-1,      // Don't use external clock source
            .output_clk_freq_hz = _speed.Clock,     // Output clock frequency
            .data_width = 1,                        // Data width is 1 bit (single data pin)
            .data_gpio_nums = {
                _dataPin
            },
            .clk_out_gpio_num = _clockPin,          // Clock pin
            .valid_gpio_num = (gpio_num_t)-1,       // Don't use valid signal
            .trans_queue_depth = 8,                 // Transaction queue depth (max count of pending transactions)
            .max_transfer_size = _bufferSize,       // Maximum transfer size (number of bytes per transaction)
            .sample_edge = PARLIO_SAMPLE_EDGE_POS,  // Sample data on the rising edge of the clock
            .bit_pack_order = PARLIO_BIT_PACK_ORDER_MSB, //Data endianness : MSB first
            .flags = { 
                //.clk_gate_en = 1,                   // NOT SUPPORTED ON C6 - Disable clock when not transmitting
            }, 
        };
        ESP_LOGD("PARLIO", "Initializing PARLIO with clock freq %d Hz, source %d", config.output_clk_freq_hz, config.clk_src);

        // Create TX unit instance
        ESP_ERROR_CHECK(parlio_new_tx_unit(&config, &_parlio_tx_handle));
        // Enable TX unit
        ESP_ERROR_CHECK(parlio_tx_unit_enable(_parlio_tx_handle));
    }

    void deinitParlio()
    {
        while (!IsReadyToUpdate())
            portYIELD();

        ESP_ERROR_CHECK(parlio_tx_unit_disable(_parlio_tx_handle));
        ESP_ERROR_CHECK(parlio_del_tx_unit(_parlio_tx_handle));
        _parlio_tx_handle = NULL;
    }

    const size_t             _sizeStartFrame;
    const size_t             _sizePixelData;   // Size of '_data' buffer below, minus (_sizeStartFrame + _sizeEndFrame)
    const size_t             _sizeEndFrame;

    size_t                  _bufferSize;
    uint8_t*                _data;       // Holds start/end frames and LED color values
    uint8_t*                _dmadata;    // Holds start/end frames and LED color values
    
    parlio_tx_unit_handle_t _parlio_tx_handle = NULL;   
    T_SPEED _speed;
    gpio_num_t _clockPin;
    gpio_num_t _dataPin;
};


typedef Hd108Esp32ParlioMethodBase<SpiSpeed40Mhz> Hd108Esp32Parlio40MhzMethod;
typedef Hd108Esp32ParlioMethodBase<SpiSpeed20Mhz> Hd108Esp32Parlio20MhzMethod;
typedef Hd108Esp32ParlioMethodBase<SpiSpeed10Mhz> Hd108Esp32Parlio10MhzMethod;
typedef Hd108Esp32ParlioMethodBase<SpiSpeed5Mhz> Hd108Esp32Parlio5MhzMethod;
typedef Hd108Esp32ParlioMethodBase<SpiSpeed2Mhz> Hd108Esp32Parlio2MhzMethod;
typedef Hd108Esp32ParlioMethodBase<SpiSpeed1Mhz> Hd108Esp32Parlio1MhzMethod;
typedef Hd108Esp32ParlioMethodBase<SpiSpeed500Khz> Hd108Esp32Parlio500KhzMethod;
typedef Hd108Esp32ParlioMethodBase<SpiSpeedHz> Hd108Esp32ParlioHzMethod;

typedef Hd108Esp32Parlio10MhzMethod Hd108Esp32ParlioMethod;

#endif // defined(ARDUINO_ARCH_ESP32) && defined(SOC_PARLIO_SUPPORTED)