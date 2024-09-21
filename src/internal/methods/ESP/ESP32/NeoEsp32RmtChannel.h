/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp32.

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

class NeoEsp32RmtChannel0
{
public:
    NeoEsp32RmtChannel0() {};
    rmt_channel_handle_t RmtChannelNumber = NULL;
};

class NeoEsp32RmtChannel1
{
public:
    NeoEsp32RmtChannel1() {};

    rmt_channel_handle_t RmtChannelNumber = NULL;
};

#if !defined(CONFIG_IDF_TARGET_ESP32C6) // C6 only 2 RMT channels ??
class NeoEsp32RmtChannel2
{
public:
    NeoEsp32RmtChannel2() {};

    rmt_channel_handle_t RmtChannelNumber = NULL;
};

class NeoEsp32RmtChannel3
{
public:
    NeoEsp32RmtChannel3() {};

protected:
    rmt_channel_handle_t RmtChannelNumber = NULL;
};
#endif // !defined(CONFIG_IDF_TARGET_ESP32C6)
#if !defined(CONFIG_IDF_TARGET_ESP32S2) && !defined(CONFIG_IDF_TARGET_ESP32C3) &&  !defined(CONFIG_IDF_TARGET_ESP32C6)

class NeoEsp32RmtChannel4
{
public:
    NeoEsp32RmtChannel4() {};

    rmt_channel_handle_t RmtChannelNumber = NULL;
};

class NeoEsp32RmtChannel5
{
public:
    NeoEsp32RmtChannel5() {};

    rmt_channel_handle_t RmtChannelNumber = NULL;
};

class NeoEsp32RmtChannel6
{
public:
    NeoEsp32RmtChannel6() {};

    rmt_channel_handle_t RmtChannelNumber = NULL;
};

class NeoEsp32RmtChannel7
{
public:
    NeoEsp32RmtChannel7() {};

    rmt_channel_handle_t RmtChannelNumber = NULL;
};

#endif

// dynamic channel support
class NeoEsp32RmtChannelN
{
public:
    NeoEsp32RmtChannelN(NeoBusChannel channel) :
        RmtChannelNumber(RmtChannelNumber)
    {
        RmtChannelNumber = NULL;
    };
    NeoEsp32RmtChannelN() = delete; // no default constructor
    rmt_channel_handle_t RmtChannelNumber = NULL;
};


