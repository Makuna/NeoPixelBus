#include <Arduino.h>
#include "NeoEspRmtMethod.h"

#include <driver/rmt.h>
#include <driver/gpio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdexcept>

std::set<rmt_channel_t> NeoEspRmtMethodImpl::s_channels = std::set<rmt_channel_t>();

// /**
//  * Set two levels of RMT output to the Neopixel value for a "1".
//  * This is:
//  * a logic 1 for 0.35us
//  * a logic 0 for 0.8us
//  */

static void setItem1(rmt_item32_t *pItem, const detail::ledParams_t ledParams)
{
    assert(pItem != nullptr);
    pItem->level0    = 1;
    pItem->duration0 = 10;
//     pItem->duration0 = ledParams.T1H / (RMT_DURATION_NS * DIVIDER);
    pItem->level1    = 0;
    pItem->duration1 = 6;
//     pItem->duration1 = ledParams.T1L / (RMT_DURATION_NS * DIVIDER);
} // setItem1

/**
 * Set two levels of RMT output to the Neopixel value for a "0".
 * This is:
 * a logic 1 for 0.35us
 * a logic 0 for 0.8us
 */
static void setItem0(rmt_item32_t *pItem, const detail::ledParams_t ledParams)
{
    assert(pItem != nullptr);
    pItem->level0    = 1;
    pItem->duration0 = 4;
//     pItem->duration0 = ledParams.T0H / (RMT_DURATION_NS * DIVIDER);
    pItem->level1    = 0;
    pItem->duration1 = 8;
//     pItem->duration1 = ledParams.T0L / (RMT_DURATION_NS * DIVIDER);
} // setItem0

/**
 * Add an RMT terminator into the RMT data.
 */
static void setTerminator(rmt_item32_t *pItem, const detail::ledParams_t ledParams)
{
    assert(pItem != nullptr);
    pItem->level0    = 0;
    pItem->duration0 = 0;
    pItem->level1    = 0;
    pItem->duration1 = 0;
} // setTerminator

void NeoEspRmtMethodImpl::Initialize()
{
    this->_items      = new rmt_item32_t[_sizePixels * 8 + 1];

    rmt_config_t config;
    config.rmt_mode                  = RMT_MODE_TX;
    config.channel                   = this->_rmtChannel;
    config.gpio_num                  = static_cast<gpio_num_t>(_gpioNum);
    config.mem_block_num             = 8-this->_rmtChannel;
    config.clk_div                   = 8;
    config.tx_config.loop_en         = 0;
    config.tx_config.carrier_en      = 0;
    config.tx_config.idle_output_en  = 1;
    config.tx_config.idle_level      = (rmt_idle_level_t)0;
    config.tx_config.carrier_freq_hz = 10000;
    config.tx_config.carrier_level   = (rmt_carrier_level_t)1;
    config.tx_config.carrier_duty_percent = 50;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(this->_rmtChannel, 0, 0));

}

void NeoEspRmtMethodImpl::Update()
{
    auto pCurrentItem = this->_items;
    auto ledParams = detail::ledParamsAll[this->_ledType];

    for (auto i = 0; i < this->_sizePixels; ++i)
    {
        uint8_t currentPixel = this->_pixels[i];


        for (int j = 7; j >= 0; --j)
        {
            // We have 8 bits of data representing the red, green amd blue channels. The value of the
            // 8 bits to output is in the variable current_pixel.  We now need to stream this value
            // through RMT in most significant bit first.  To do this, we iterate through each of the 8
            // bits from MSB to LSB.
            if (currentPixel & (1<<j)) {
                setItem1(pCurrentItem, ledParams);
            } else {
                setItem0(pCurrentItem, ledParams);
            }
            pCurrentItem++;
        }
    }
    setTerminator(pCurrentItem, ledParams); // Write the RMT terminator.

    // Show the pixels.
    ESP_ERROR_CHECK(rmt_write_items(this->_rmtChannel, this->_items, this->_sizePixels * 8, 1 /* wait till done */));
}
