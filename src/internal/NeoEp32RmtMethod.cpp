/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp32.

A BIG thanks to Andreas Merkle for the investigation and implementation of
a workaround to the GCC bug that drops method attributes from template methods

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

#include "NeoEsp32RmtMethod.h"

#ifdef ARDUINO_ARCH_ESP32


// these are required due to the linker error with ISRs
// dangerous relocation: l32r: literal placed after use
// https://stackoverflow.com/questions/19532826/what-does-a-dangerous-relocation-error-mean
//
void NeoEsp32RmtSpeedWs2811::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtSpeedWs2812x::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtSpeedSk6812::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtSpeedTm1814::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtSpeed800Kbps::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtSpeed400Kbps::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtSpeedApa106::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtInvertedSpeedWs2811::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtInvertedSpeedWs2812x::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtInvertedSpeedSk6812::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtInvertedSpeedTm1814::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtInvertedSpeed800Kbps::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtInvertedSpeed400Kbps::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}

void NeoEsp32RmtInvertedSpeedApa106::Translate(const void* src,
    rmt_item32_t* dest,
    size_t src_size,
    size_t wanted_num,
    size_t* translated_size,
    size_t* item_num)
{
    _translate(src, dest, src_size, wanted_num, translated_size, item_num,
        RmtBit0, RmtBit1, RmtDurationReset);
}
#endif