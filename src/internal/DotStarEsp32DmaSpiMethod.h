/*-------------------------------------------------------------------------
NeoPixel library helper functions for DotStars using Esp32, DMA and SPI (APA102).

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

#include "driver/spi_master.h"

#define DMASPI_PARALLEL_BITS 1

class Esp32VspiBus
{
public:
    static const uint8_t spiBus = VSPI;
    static const spi_host_device_t spiHostDevice = VSPI_HOST;
    static const int dmaChannel = 1;        // // arbitrary assignment, but based on the fact there are only two DMA channels and two available SPI ports, we need to split them somehow
};

class Esp32HspiBus
{
public:
    static const uint8_t spiBus = HSPI;
    static const spi_host_device_t spiHostDevice = HSPI_HOST;
    static const int dmaChannel = 2;        // // arbitrary assignment, but based on the fact there are only two DMA channels and two available SPI ports, we need to split them somehow
};

template<typename T_SPISPEED, typename T_SPIBUS> class DotStarEsp32DmaSpiMethod
{
public:
    DotStarEsp32DmaSpiMethod(uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        _sizePixelData(pixelCount * elementSize + settingsSize),
        _sizeEndFrame((pixelCount + 15) / 16) // 16 = div 2 (bit for every two pixels) div 8 (bits to bytes)
    {
        _spiBufferSize = _sizeStartFrame + _sizePixelData + _sizeEndFrame;

        // must have a 4 byte aligned buffer for i2s
        uint32_t alignment = _spiBufferSize % 4;
        if (alignment)
        {
            _spiBufferSize += 4 - alignment;
        }

        _data = static_cast<uint8_t*>(malloc(_spiBufferSize));
        _dmadata = static_cast<uint8_t*>(heap_caps_malloc(_spiBufferSize, MALLOC_CAP_DMA));

        // data cleared later in NeoPixelBus::Begin()
    }

    // Support constructor specifying pins by ignoring pins
    DotStarEsp32DmaSpiMethod(uint8_t, uint8_t, uint16_t pixelCount, size_t elementSize, size_t settingsSize) :
        DotStarEsp32DmaSpiMethod(pixelCount, elementSize, settingsSize)
    {
    }

    ~DotStarEsp32DmaSpiMethod()
    {
        free(_data);
        free(_dmadata);
    }

    bool IsReadyToUpdate() const
    {
        spi_transaction_t t;
        spi_transaction_t * tptr = &t;
        esp_err_t ret = spi_device_get_trans_result(_spiHandle, &tptr, 0);

        // We know the previous transaction completed if we got ESP_OK, and there's no transactions queued if tptr is unmodified
        return (ret==ESP_OK || tptr == &t);
    }

    void Initialize(int8_t sck, int8_t miso, int8_t mosi, int8_t ss)
    {
        memset(_data, 0x00, _sizeStartFrame);
        memset(_data + _sizeStartFrame + _sizePixelData, 0x00, _spiBufferSize - (_sizeStartFrame + _sizePixelData));

        // TODO: check values of arguments

        esp_err_t ret;
        spi_bus_config_t buscfg;
        memset(&buscfg, 0x00, sizeof(buscfg));

        buscfg.miso_io_num=miso;
        buscfg.mosi_io_num=mosi;
        buscfg.sclk_io_num=sck;
        buscfg.quadwp_io_num=-1;
        buscfg.quadhd_io_num=-1;
        buscfg.max_transfer_sz=_spiBufferSize;

        spi_device_interface_config_t devcfg;
        memset(&devcfg, 0x00, sizeof(devcfg));

        devcfg.clock_speed_hz=T_SPISPEED::Clock;
        devcfg.mode=0;                 //SPI mode 0
        devcfg.spics_io_num=ss;        //CS pin
        devcfg.queue_size=1;
#if (DMASPI_PARALLEL_BITS == 1)
        devcfg.flags=0;
#elif (DMASPI_PARALLEL_BITS == 2)
        devcfg.flags=SPI_DEVICE_HALFDUPLEX;
#endif

        //Initialize the SPI bus
        ret=spi_bus_initialize(T_SPIBUS::spiHostDevice, &buscfg, T_SPIBUS::dmaChannel);
        ESP_ERROR_CHECK(ret);

        //Allocate the LEDs on the SPI bus
        ret=spi_bus_add_device(T_SPIBUS::spiHostDevice, &devcfg, &_spiHandle);
        ESP_ERROR_CHECK(ret);
    }

    void Initialize()
    {
        // TODO: do we even want to handle default port?
        //_wire.begin();
    }

    void Update(bool)
    {
        while(!IsReadyToUpdate());

        memcpy(_dmadata, _data, _spiBufferSize);

        memset(&_spiTransaction, 0, sizeof(spi_transaction_t));
        _spiTransaction.length=(_spiBufferSize) * 8; // in bits not bytes!
#if (DMASPI_PARALLEL_BITS == 1)
        _spiTransaction.flags = 0;
#elif (DMASPI_PARALLEL_BITS == 2)
        _spiTransaction.flags = SPI_TRANS_MODE_DIO;
#endif
        _spiTransaction.tx_buffer = _dmadata;

        esp_err_t ret = spi_device_queue_trans(_spiHandle, &_spiTransaction, 0);  //Transmit!
        assert(ret==ESP_OK);            //Should have had no issues.
    }

    uint8_t* getData() const
    {
        return _data + _sizeStartFrame;
    };

    size_t getDataSize() const
    {
        return _sizePixelData;
    };

private:
    const size_t             _sizeStartFrame = 4;
    const size_t             _sizePixelData;   // Size of '_data' buffer below, minus (_sizeStartFrame + _sizeEndFrame)
    const size_t             _sizeEndFrame;

    size_t                  _spiBufferSize;
    uint8_t*                _data;       // Holds start/end frames and LED color values
    uint8_t*                _dmadata;    // Holds start/end frames and LED color values
    spi_device_handle_t     _spiHandle;
    spi_transaction_t       _spiTransaction;
};

typedef DotStarEsp32DmaSpiMethod<SpiSpeed10Mhz,Esp32VspiBus> DotStarEsp32DmaVspi10MhzMethod;
typedef DotStarEsp32DmaVspi10MhzMethod DotStarEsp32DmaVspiMethod;

typedef DotStarEsp32DmaSpiMethod<SpiSpeed10Mhz,Esp32HspiBus> DotStarEsp32DmaHspi10MhzMethod;
typedef DotStarEsp32DmaHspi10MhzMethod DotStarEsp32DmaHspiMethod;
