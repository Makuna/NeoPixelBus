/*-------------------------------------------------------------------------
NeoSm168x5Features provides feature classes to describe color order and
color depth for NeoPixelBus template class specific to the SM168x5 chips/leds

Written by Michael C. Miller.

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
#pragma once
/*
5 channel RGBWY
SM16825E 10.2~310.0mA
*/

class NeoSm168x5SettingsBase : public NeoRgbwwCurrentSettings
{
public:
    NeoSm168x5SettingsBase(uint8_t redGain, 
            uint8_t greenGain, 
            uint8_t blueGain,
            uint8_t whiteGain,
            uint8_t otherGain,
            uint16_t redCurrent, 
            uint16_t greenCurrent,
            uint16_t blueCurrent,
            uint16_t whiteCurrent,
            uint16_t otherCurrent) :
        NeoRgbwwCurrentSettings(redCurrent, greenCurrent, blueCurrent, whiteCurrent, otherCurrent),
        RedGain(redGain & 0x1f),
        GreenGain(greenGain & 0x1f),
        BlueGain(blueGain & 0x1f), 
        WhiteGain(whiteGain & 0x1f), 
        OtherGain(otherGain & 0x1f) {}

    // ------------------------------------------------------------------------
    // operator [] - readonly
    // access elements in order by index rather than member name
    // ------------------------------------------------------------------------
    uint8_t operator[](size_t idx) const
    {
        switch (idx)
        {
        case 0:
            return RedGain;
        case 1:
            return GreenGain;
        case 2:
            return BlueGain;
        case 3:
            return WhiteGain;
        default:
            return OtherGain;
        }
    }

    const uint8_t RedGain : 5;
    const uint8_t GreenGain : 5;
    const uint8_t BlueGain : 5;
    const uint8_t WhiteGain : 5;
    const uint8_t OtherGain : 5;
};



template <uint8_t V_IC_1, uint8_t V_IC_2, uint8_t V_IC_3, uint8_t V_IC_4, uint8_t V_IC_5>
class NeoSm16825eSettings : public NeoSm168x5SettingsBase
{
public:
    NeoSm16825eSettings(uint8_t redGain, uint8_t greenGain, uint8_t blueGain, uint8_t whiteGain, uint8_t otherGain ) :
        NeoSm168x5SettingsBase(redGain,
            greenGain,
            blueGain,
            whiteGain,
            otherGain,
            CurrentLookup[redGain],
            CurrentLookup[greenGain],
            CurrentLookup[blueGain],
            CurrentLookup[whiteGain],
            CurrentLookup[otherGain])
    {
    }

    void Encode(uint8_t* encoded) const
    {
        // RGBWY 5 bits each
        *encoded++ = operator[](V_IC_1) << 3 | operator[](V_IC_2) >> 2; // 0b11111222
        *encoded++ = operator[](V_IC_2) << 6 | operator[](V_IC_3) << 1 | operator[](V_IC_4) >> 4; // 0b22333334
        *encoded++ = operator[](V_IC_4) << 4 | operator[](V_IC_5) >> 1; // 0b44445555
        *encoded = operator[](V_IC_5) << 7 | 0b00011111; // 0b50011111 00 (action, not standby) 11111 (reserved)
    }

protected:
    static constexpr uint16_t CurrentLookup[32] = {
        102, 203, 304, 405, 506, 607, 708, 809,
        910, 1011, 1112, 1213, 1307, 1406, 1505, 1602, 
        1700, 1790, 1885, 1980, 2078, 2168, 2264, 2358, 
        2450, 2544, 2636, 2728, 2820, 2910, 3000, 3100}; // in tenth mA

    /* not to spec, switched to table
    constexpr uint16_t MinCmA =  1020; // 100th of a mA
    constexpr uint16_t MaxCmA = 31000;
    constexpr uint16_t DeltaCmA = MaxCmA - MinCmA;
    constexpr uint16_t IncCmA = DeltaCmA / 31;

    static uint16_t calcCurrent(const uint8_t gain)
    {
        uint16_t CmA = MinCmA + (gain * IncCmA);
        return CmA / 10; // return tenths of mA
    }
    */
};

// CAUTION:  Make sure ColorIndex order for Neo5ByteFeature matches T_SETTINGS
template<uint8_t V_IC_1, uint8_t V_IC_2, uint8_t V_IC_3, uint8_t V_IC_4, uint8_t V_IC_5>
class NeoRgbwcSm168x5Elements : 
    public Neo5WordFeature<V_IC_1, V_IC_2, V_IC_3, V_IC_4, V_IC_5>
{
public:
    typedef NeoSm16825eSettings<V_IC_1, V_IC_2, V_IC_3, V_IC_4, V_IC_5> SettingsObject;
    static const size_t SettingsSize = 4;

    static void applySettings([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData, [[maybe_unused]] const SettingsObject& settings)
    {
        // settings are at the end of the data stream
        uint8_t* pDest = pData + sizeData - SettingsSize;

        settings.Encode(pDest);
    }

    static uint8_t* pixels([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        return pData;
    }

    static const uint8_t* pixels([[maybe_unused]] const uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        return pData;
    }
};

typedef NeoRgbwcSm168x5Elements<ColorIndexR, ColorIndexG, ColorIndexB, ColorIndexWW, ColorIndexCW> NeoRgbwcSm16825eFeature;
typedef NeoRgbwcSm168x5Elements<ColorIndexR, ColorIndexG, ColorIndexB, ColorIndexCW, ColorIndexWW> NeoRgbcwSm16825eFeature;

