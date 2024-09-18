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

class NeoEsp32RmtSpeed
{
public:
    // 
    // ClkDiv of 2 provides for good resolution and plenty of reset resolution; but
    // a ClkDiv of 1 will provide enough space for the longest reset and does show
    // little better pulse accuracy
    const static uint8_t RmtClockDivider = 2;
    const static uint32_t RmtCpu = 80000000L; // 80 mhz RMT clock
    const static uint32_t NsPerSecond = 1000000000L;
    const static uint32_t RmtTicksPerSecond = (RmtCpu / RmtClockDivider);
    const static uint32_t NsPerRmtTick = (NsPerSecond / RmtTicksPerSecond); // about 25 

    inline constexpr static uint32_t FromNs(uint32_t ns)
    {
        return ns / NsPerRmtTick;
    }
    // this is used rather than the rmt_symbol_word_t as you can't correctly initialize
    // it as a static constexpr within the template
    inline constexpr static uint32_t Item32Val(uint16_t nsHigh, uint16_t nsLow)
    {
        return (FromNs(nsLow) << 16) | (1 << 15) | (FromNs(nsHigh));
    }


};

class NeoEsp32RmtSpeedBase : public NeoEsp32RmtSpeed
{
public:
    const static bool Inverted = false;
};

class NeoEsp32RmtInvertedSpeedBase : public NeoEsp32RmtSpeed
{
public:
    const static bool Inverted = true;
};

class NeoEsp32RmtSpeedWs2811 : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(300, 950);
    const static uint32_t RmtBit1 = Item32Val(900, 350);
    const static uint16_t RmtDurationReset = FromNs(300000); // 300us
};

class NeoEsp32RmtSpeedWs2812x : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(400, 850);
    const static uint32_t RmtBit1 = Item32Val(800, 450);
    const static uint16_t RmtDurationReset = FromNs(300000); // 300us
};

class NeoEsp32RmtSpeedSk6812 : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(400, 850);
    const static uint32_t RmtBit1 = Item32Val(800, 450);
    const static uint16_t RmtDurationReset = FromNs(80000); // 80us
};

// normal is inverted signal
class NeoEsp32RmtSpeedTm1814 : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(360, 890);
    const static uint32_t RmtBit1 = Item32Val(720, 530);
    const static uint16_t RmtDurationReset = FromNs(200000); // 200us
};

// normal is inverted signal
class NeoEsp32RmtSpeedTm1829 : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(300, 900);
    const static uint32_t RmtBit1 = Item32Val(800, 400);
    const static uint16_t RmtDurationReset = FromNs(200000); // 200us
};

// normal is inverted signal
class NeoEsp32RmtSpeedTm1914 : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(360, 890);
    const static uint32_t RmtBit1 = Item32Val(720, 530);
    const static uint16_t RmtDurationReset = FromNs(200000); // 200us
};

class NeoEsp32RmtSpeed800Kbps : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(400, 850);
    const static uint32_t RmtBit1 = Item32Val(800, 450);
    const static uint16_t RmtDurationReset = FromNs(50000); // 50us
};

class NeoEsp32RmtSpeed400Kbps : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(800, 1700);
    const static uint32_t RmtBit1 = Item32Val(1600, 900);
    const static uint16_t RmtDurationReset = FromNs(50000); // 50us
};

class NeoEsp32RmtSpeedApa106 : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(350, 1350);
    const static uint32_t RmtBit1 = Item32Val(1350, 350);
    const static uint16_t RmtDurationReset = FromNs(50000); // 50us
};

class NeoEsp32RmtSpeedTx1812 : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(300, 600);
    const static uint32_t RmtBit1 = Item32Val(600, 300);
    const static uint16_t RmtDurationReset = FromNs(80000); // 80us
};

class NeoEsp32RmtInvertedSpeedWs2811 : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(300, 950);
    const static uint32_t RmtBit1 = Item32Val(900, 350);
    const static uint16_t RmtDurationReset = FromNs(300000); // 300us
};

class NeoEsp32RmtInvertedSpeedWs2812x : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(400, 850);
    const static uint32_t RmtBit1 = Item32Val(800, 450);
    const static uint16_t RmtDurationReset = FromNs(300000); // 300us
};

class NeoEsp32RmtInvertedSpeedSk6812 : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(400, 850);
    const static uint32_t RmtBit1 = Item32Val(800, 450);
    const static uint16_t RmtDurationReset = FromNs(80000); // 80us
};

// normal is inverted signal
class NeoEsp32RmtInvertedSpeedTm1814 : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(360, 890);
    const static uint32_t RmtBit1 = Item32Val(720, 530);
    const static uint16_t RmtDurationReset = FromNs(200000); // 200us
};

// normal is inverted signal
class NeoEsp32RmtInvertedSpeedTm1829 : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(300, 900);
    const static uint32_t RmtBit1 = Item32Val(800, 400);
    const static uint16_t RmtDurationReset = FromNs(200000); // 200us
};

// normal is inverted signal
class NeoEsp32RmtInvertedSpeedTm1914 : public NeoEsp32RmtSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(360, 890);
    const static uint32_t RmtBit1 = Item32Val(720, 530);
    const static uint16_t RmtDurationReset = FromNs(200000); // 200us
};

class NeoEsp32RmtInvertedSpeed800Kbps : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(400, 850);
    const static uint32_t RmtBit1 = Item32Val(800, 450);
    const static uint16_t RmtDurationReset = FromNs(50000); // 50us
};

class NeoEsp32RmtInvertedSpeed400Kbps : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(800, 1700);
    const static uint32_t RmtBit1 = Item32Val(1600, 900);
    const static uint16_t RmtDurationReset = FromNs(50000); // 50us
};

class NeoEsp32RmtInvertedSpeedApa106 : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(350, 1350);
    const static uint32_t RmtBit1 = Item32Val(1350, 350);
    const static uint16_t RmtDurationReset = FromNs(50000); // 50us
};

class NeoEsp32RmtInvertedSpeedTx1812 : public NeoEsp32RmtInvertedSpeedBase
{
public:
    const static uint32_t RmtBit0 = Item32Val(300, 600);
    const static uint32_t RmtBit1 = Item32Val(600, 300);
    const static uint16_t RmtDurationReset = FromNs(80000); // 80us
};

