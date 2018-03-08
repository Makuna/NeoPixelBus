/*-------------------------------------------------------------------------
  NeoPixel library base on RMT for Esp32

  Written by Sven Fischer.
  RMT code copied from Neil Kolban
  Original from https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/WS2812.cpp

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

#if defined(ARDUINO_ARCH_ESP32)

#include <stdint.h>
#include <driver/rmt.h>
#include <set>

namespace detail {

    enum led_types {
        // LED_WS2812_V1,
        // LED_WS2812B_V1,
        // LED_WS2812B_V2,
        // LED_WS2812B_V3,
        // LED_WS2813_V1,
        // LED_WS2813_V2,
        LED_WS2813_V3,
        // LED_SK6812_V1,
        // LED_SK6812W_V1,
    };
    typedef struct {
        uint32_t T0H;
        uint32_t T1H;
        uint32_t T0L;
        uint32_t T1L;
    } ledParams_t;

    const ledParams_t ledParamsAll[] = {  // Still must match order of `led_types`
        // [LED_WS2812_V1]  = { .bytesPerPixel = 3, .T0H = 350, .T1H = 700, .T0L = 800, .T1L = 600, .TRS =  50000},
        // [LED_WS2812B_V1] = { .bytesPerPixel = 3, .T0H = 350, .T1H = 900, .T0L = 900, .T1L = 350, .TRS =  50000}, // Older datasheet
        // [LED_WS2812B_V2] = { .bytesPerPixel = 3, .T0H = 400, .T1H = 850, .T0L = 850, .T1L = 400, .TRS =  50000}, // 2016 datasheet
        // [LED_WS2812B_V3] = { .bytesPerPixel = 3, .T0H = 450, .T1H = 850, .T0L = 850, .T1L = 450, .TRS =  50000}, // cplcpu test
        // [LED_WS2813_V1]  = { .bytesPerPixel = 3, .T0H = 350, .T1H = 800, .T0L = 350, .T1L = 350, .TRS = 300000}, // Older datasheet
        // [LED_WS2813_V2]  = { .bytesPerPixel = 3, .T0H = 270, .T1H = 800, .T0L = 800, .T1L = 270, .TRS = 300000}, // 2016 datasheet
        [LED_WS2813_V3]  = { .T0H = 4, .T1H = 10, .T0L = 8, .T1L = 6 }, // 2017-05 WS datasheet
        // [LED_SK6812_V1]  = { .bytesPerPixel = 3, .T0H = 300, .T1H = 600, .T0L = 900, .T1L = 600, .TRS =  80000},
        // [LED_SK6812W_V1] = { .bytesPerPixel = 4, .T0H = 300, .T1H = 600, .T0L = 900, .T1L = 600, .TRS =  80000},
    };
}


class NeoEspRmtMethodImpl
{
public:

    NeoEspRmtMethodImpl(uint8_t pin, uint16_t pixelCount, size_t elementSize, int ledType)
        : _gpioNum(pin)
        , _sizePixels(pixelCount * elementSize)
        , _pixels(nullptr)
        , _ledType(ledType)
        {
            pinMode(pin, OUTPUT);

            _sizePixels = pixelCount * elementSize;
            _pixels = (uint8_t*)malloc(_sizePixels);
            memset(_pixels, 0, _sizePixels);

            // Find a free channel
            for (int i = RMT_CHANNEL_0; i < RMT_CHANNEL_MAX; ++i)
            {
                auto c = static_cast<rmt_channel_t>(i);

                if (s_channels.find(c) == s_channels.end())
                {
                    s_channels.insert(c);
                    _rmtChannel = c;
                    break;
                }
            }
        }

    ~NeoEspRmtMethodImpl()
        {
            pinMode(_gpioNum, INPUT);

            free(_pixels);
            s_channels.erase(s_channels.find(_rmtChannel));
        }

    bool IsReadyToUpdate() const
        {
            return true;
        }

    void Initialize();

    void Update();

    uint8_t* getPixels() const
        {
            return _pixels;
        };

    size_t getPixelsSize() const
        {
            return _sizePixels;
        };


private:
    size_t    _sizePixels;   // Size of '_pixels' buffer below
    uint8_t*  _pixels;        // Holds LED color values

    rmt_channel_t _rmtChannel;
    rmt_item32_t *_items{nullptr};
    int       _gpioNum;
    int       _ledType;

    static std::set<rmt_channel_t> s_channels;
};


template<int T_LEDTYPE> class NeoEspRmtMethodBase
{
public:

    NeoEspRmtMethodBase(uint8_t pin, uint16_t pixelCount, size_t elementSize) :
        _impl(pin, pixelCount, elementSize, T_LEDTYPE)
        {
        }

    ~NeoEspRmtMethodBase()
        {
        }

    bool IsReadyToUpdate() const
        {
            return _impl.IsReadyToUpdate();
        }

    void Initialize()
        {
            _impl.Initialize();
        }

    void Update()
        {
            _impl.Update();
        }

    uint8_t* getPixels() const
        {
            return _impl.getPixels();
        };

    size_t getPixelsSize() const
        {
            return _impl.getPixelsSize();
        };

 private:
    NeoEspRmtMethodImpl _impl;
};


// typedef NeoEspRmtMethodBase<NeoEspRmtSpeed400Kbps> NeoEsp32Rmt400KbpsMethod;
// typedef NeoEspRmtMethodBase<detail::LED_WS2812_V1> NeoEsp32RmtWS2812_V1Method;
// typedef NeoEspRmtMethodBase<detail::LED_WS2812B_V1> NeoEsp32RmtWS2812B_V1Method;
// typedef NeoEspRmtMethodBase<detail::LED_WS2812B_V2> NeoEsp32RmtWS2812B_V2Method;
// typedef NeoEspRmtMethodBase<detail::LED_WS2812B_V3> NeoEsp32RmtWS2812B_V3Method;
// typedef NeoEspRmtMethodBase<detail::LED_WS2813_V1> NeoEsp32RmtWS2813_V1Method;
// typedef NeoEspRmtMethodBase<detail::LED_WS2813_V2> NeoEsp32RmtWS2813_V2Method;
typedef NeoEspRmtMethodBase<detail::LED_WS2813_V3> NeoEsp32RmtWS2813_V3Method;
// typedef NeoEspRmtMethodBase<detail::LED_SK6812_V1> NeoEsp32RmtSK6812_V1Method;
// typedef NeoEspRmtMethodBase<detail::LED_SK6812W_V1> NeoEsp32RmtSK6812W_V1Method;

// // Bitbang method is the default method for Esp32
// typedef NeoEsp32RmtWs2813Method NeoWs2813Method;
// typedef NeoEsp32Rmt800KbpsMethod Neo800KbpsMethod;
// typedef NeoEsp32Rmt400KbpsMethod Neo400KbpsMethod;

#endif
