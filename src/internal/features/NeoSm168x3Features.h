/*-------------------------------------------------------------------------
NeoSm168x3Features provides feature classes to describe color order and
color depth for NeoPixelBus template class specific to the SM168x3 chips/leds

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
3 channel RGB
SM16803P  1.8~60mA << need spec sheet to get accurate implementation
SM16813PB 1.8~19mA 
SM16823E 60~350mA
*/

class NeoSm168x3SettingsBase : public NeoRgbCurrentSettings
{
public:
    NeoSm168x3SettingsBase(uint8_t redGain, 
            uint8_t greenGain, 
            uint8_t blueGain,
            uint16_t redCurrent, 
            uint16_t greenCurrent,
            uint16_t blueCurrent) :
        NeoRgbCurrentSettings(redCurrent, greenCurrent, blueCurrent),
        RedGain(redGain & 0x0f),
        GreenGain(greenGain & 0x0f),
        BlueGain(blueGain & 0x0f) {}

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
        default:
            return BlueGain;
        }
    }

    const uint8_t RedGain : 4;
    const uint8_t GreenGain : 4;
    const uint8_t BlueGain : 4;
};

template <uint8_t V_IC_1, uint8_t V_IC_2, uint8_t V_IC_3>
class NeoSm16803pbSettings : public NeoSm168x3SettingsBase
{
public:
    NeoSm16803pbSettings(uint8_t redGain, uint8_t greenGain, uint8_t blueGain) :
        NeoSm168x3SettingsBase(redGain, 
            greenGain, 
            blueGain,
            CurrentLookup[redGain],
            CurrentLookup[greenGain],
            CurrentLookup[blueGain])
    {
    }

    void Encode(uint8_t* encoded) const
    {
        // 0RGB 4 bits each
        *encoded++ = operator[](V_IC_1);
        *encoded = operator[](V_IC_2) << 4 | operator[](V_IC_3);
    }

protected:
    static constexpr uint8_t CurrentLookup[16] = {
            18, 30, 41, 53, 64, 76, 87, 99, 
            110, 133, 145, 156, 168, 179, 190};
};

template <uint8_t V_IC_1, uint8_t V_IC_2, uint8_t V_IC_3>
class NeoSm16823eSettings : public NeoSm168x3SettingsBase
{
public:
    NeoSm16823eSettings(uint8_t redGain, uint8_t greenGain, uint8_t blueGain, uint16_t resisterOhms) :
        NeoSm168x3SettingsBase(redGain,
            greenGain,
            blueGain,
            calcCurrent(resisterOhms, redGain),
            calcCurrent(resisterOhms, greenGain),
            calcCurrent(resisterOhms, blueGain)),
        extROhms(resisterOhms)
    {
    }

    void Encode(uint8_t* encoded) const
    {
        // RGB0 4 bits each
        *encoded++ = operator[](V_IC_1) << 4 | operator[](V_IC_2);
        *encoded = operator[](V_IC_3) << 4;
    }

protected:
    const uint16_t extROhms;

    static uint16_t calcCurrent(const uint16_t ohms, const uint8_t gain)
    {
        uint16_t mA = (967 * (240 + (gain * 32)) / ohms); // from spec sheet, gain 0-15 instead
        return mA * 10; // return tenths of mA
    }
};

// CAUTION:  Make sure ColorIndex order for Neo3ByteFeature matches T_SETTINGS
template<typename T_SETTINGS> class NeoRgbSm168x3Elements : 
    public Neo3ByteFeature<ColorIndexR, ColorIndexG, ColorIndexB>
{
public:
    typedef T_SETTINGS SettingsObject;
    static const size_t SettingsSize = 2;

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

typedef NeoRgbSm168x3Elements<NeoSm16803pbSettings<ColorIndexR, ColorIndexG, ColorIndexB>> NeoRgbSm16803pbFeature;
typedef NeoRgbSm168x3Elements<NeoSm16823eSettings<ColorIndexR, ColorIndexG, ColorIndexB>> NeoRgbSm16823eFeature;



