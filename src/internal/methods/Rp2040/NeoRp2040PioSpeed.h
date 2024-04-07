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

// Speeds
// --------------------------------------------------------
class NeoRp2040PioSpeedWs2811 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz = 800000.0f; // 300+950
    static constexpr uint32_t ResetTimeUs = 300;
};

class NeoRp2040PioSpeedWs2812x : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz = 800000.0f; // 400+850
    static constexpr uint32_t ResetTimeUs = 300;
};

class NeoRp2040PioSpeedWs2805 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz = 917431.19f; // 300+790
    static constexpr uint32_t ResetTimeUs = 300;  // spec is 280, intentionally longer for compatiblity use
};

class NeoRp2040PioSpeedSk6812 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz = 800000.0f; // 400+850
    static constexpr uint32_t ResetTimeUs = 80;
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1814 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz = 800000.00f; // 360+890
    static constexpr uint32_t ResetTimeUs = 200;
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1829 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz = 833333.33f; // 300+900
    static constexpr uint32_t ResetTimeUs = 200;
};

// normal is inverted signal
class NeoRp2040PioSpeedTm1914 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz = 800000.0f; // 360+890
    static constexpr uint32_t ResetTimeUs = 200;
};

class NeoRp2040PioSpeed800Kbps : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz = 800000.0f; // 400+850
    static constexpr uint32_t ResetTimeUs = 50;
};

class NeoRp2040PioSpeed400Kbps : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz = 400000.0f; // 800+1700
    static constexpr uint32_t ResetTimeUs = 50;
};

class NeoRp2040PioSpeedApa106 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz = 588235.29f; // 350+1350
    static constexpr uint32_t ResetTimeUs = 50;
};

class NeoRp2040PioSpeedTx1812 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono3Step>
{
public:
    static constexpr float BitRateHz = 1111111.11f; // 300+600
    static constexpr uint32_t ResetTimeUs = 80;
};

class NeoRp2040PioSpeedGs1903 : public NeoRp2040PioMonoProgram<NeoRp2040PioCadenceMono4Step>
{
public:
    static constexpr float BitRateHz = 833333.33f; // 300+900
    static constexpr uint32_t ResetTimeUs = 40;
};

#endif