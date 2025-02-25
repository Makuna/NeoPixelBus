#pragma once
/*-------------------------------------------------------------------------
NeoPixel library helper functions for Methods.

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

// --------------------------------------------------------
class NeoBitsSpeedDmx512
{
public:
    const static uint16_t BitSendTimeNs = 4000; //5500;  //   4us, 250Kbps so 4000 * 11 / 8
    const static uint16_t ResetTimeUs = 500; // 6 x 32us equals 3 Words.
};

class NeoBitsSpeedWs2812x
{
public:
    const static uint16_t BitSendTimeNs = 1250;
    const static uint16_t ResetTimeUs = 300;
};

class NeoBitsSpeedWs2805
{
public:
    const static uint16_t BitSendTimeNs = 1125;
    const static uint16_t ResetTimeUs = 300; // spec is 280, intentionally longer for compatiblity use
};

class NeoBitsSpeedSk6812
{
public:
    const static uint16_t BitSendTimeNs = 1250;
    const static uint16_t ResetTimeUs = 80;
};

class NeoBitsSpeedTm1814
{
public:
    const static uint16_t BitSendTimeNs = 1250;
    const static uint16_t ResetTimeUs = 200;
};

class NeoBitsSpeedTm1914
{
public:
    const static uint16_t BitSendTimeNs = 1250;
    const static uint16_t ResetTimeUs = 200;
};

class NeoBitsSpeedTm1829
{
public:
    const static uint16_t BitSendTimeNs = 1250;
    const static uint16_t ResetTimeUs = 200;
};

class NeoBitsSpeed800Kbps
{
public:
    const static uint16_t BitSendTimeNs = 1250;
    const static uint16_t ResetTimeUs = 50;
};

class NeoBitsSpeed400Kbps
{
public:
    const static uint16_t BitSendTimeNs = 2500;
    const static uint16_t ResetTimeUs = 50;
};

class NeoBitsSpeedApa106
{
public:
    const static uint16_t BitSendTimeNs = 1710;
    const static uint16_t ResetTimeUs = 50;
};

//---------------------------------------------------------
class NeoBitsNotInverted
{
public:
    const static bool Inverted = false;
};

class NeoBitsInverted
{
public:
    const static bool Inverted = true;
};
