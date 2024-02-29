/*-------------------------------------------------------------------------
Tlc59711RgbFeatures provides feature classes to describe color order and
color depth for NeoPixelBus template class specific to the TLC59711 chip

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

// Four byte settings header per 24 bytes color data
//    12 channel, 16 bit, grouped RGB
// Write Command
// |      OUTTMG - 1 rising edge, 0 falling edge
// |      | EXTGCK - 0 internal clock, 1 SCKI clock
// |      | | TMGRST - 0/1 enable display timer reset mode
// |      | | | DSPRPT - 0/1 enabled auto display repeat
// |      | | | | BLANK - 0 enabled, 1 blank
// |      | | | | | BC (3x7) RGB Brightness control
// |      | | | | | |   red   green    blue
// |      | | | | | |     |       |       | color data (24 bytes) ->
// 100101 1 0 1 1 0 1111111 1111111 1111111 xxxxxxxx xxxxxxxx
// 765432 1 0 7 6 5 4321076 5432107 6543210 <- bit order within byte

enum Tlc69711_Control
{
    Tlc69711_Control_EmiFallingEdge = 0x00,
    Tlc69711_Control_EmiRisingEdge = 0x02,
    Tlc69711_Control_ExternalClock = 0x01,

    Tlc69711_Control_DisplayTimerReset = 0x80,
    Tlc69711_Control_AutoDisplayRepeat = 0x40,
    Tlc69711_Control_Blank = 0x20,

    Tlc69711_Control_Mask1 = 0x03,  // mask of this enum for first byte of encoded settings
    Tlc69711_Control_Mask2 = 0x0e   // mask of this enum for second byte of encoded settings

    Tlc69711_Control_Default = Tlc69711_Control_EmiRisingEdge | Tlc69711_Control_DisplayTimerReset | Tlc69711_Control_AutoDisplayRepeat;
};

class Tlc69711Settings
{
public:
    Tlc69711Settings(
            uint8_t groupsBrightness = 127,
            uint8_t control = Tlc69711_Control_Default) :
        RedGroupBrightness(groupsBrightness & 0x7f),
        GreenGroupBrightness(groupsBrightness & 0x7f),
        BlueGroupBrightness(groupsBrightness & 0x7f),
        Control(control)
    {
    }

    Tlc69711Settings(
            uint8_t redGroupBrightness, 
            uint8_t greenGroupBrightness,
            uint8_t blueGroupBrightness,
            uint8_t control = Tlc69711_Control_Default) :
        RedGroupBrightness(redGroupBrightness & 0x7f),
        GreenGroupBrightness(greenGroupBrightness & 0x7f),
        BlueGroupBrightness(blueGroupBrightness & 0x7f),
        Control(control)
    {
    }

    const uint8_t RedGroupBrightness;
    const uint8_t GreenGroupBrightness;
    const uint8_t BlueGroupBrightness;
    const uint8_t Control;
};

class Tlc59711ElementsSettings
{
public:
    typedef Tlc69711Settings SettingsObject;

    static constexpr size_t SettingsPerChipSize = 4;

    static constexpr size_t SettingsSize = 2 * SettingsPerChipSize; 

    void Encode(uint8_t* encoded, bool emiAlternate = false) const
    {
        uint8_t control = Control;

        if (emiAlternate)
        {
            if (control & Tlc69711_Control_EmiRisingEdge)
            {
                // clear the flag
                control &= ~Tlc69711_Control_EmiRisingEdge;
            }
            else
            {
                // set the flag
                control |= Tlc69711_Control_EmiRisingEdge;
            }
        }

        *encoded++ = 0b10010100 | (control & Tlc69711_Control_Mask1);
        *encoded++ = (control & Tlc69711_Control_Mask2) | RedGroupBrightness >> 2;
        *encoded++ = RedGroupBrightness << 5 | GreenGroupBrightness >> 1;
        *encoded = GreenGroupBrightness << 7 | BlueGroupBrightness;
    }

    static void applySettings([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData, [[maybe_unused]] const SettingsObject& settings)
    {
        // how the hell do we store the setting that doesn't go into the data stream'
        // Thought A:
        //   Add a new method to Methods, getSettingsData() that for most will just
        //   return the front of the data stream, but for this it will return the pointer
        //   to a member variable that is a SettingsObject, where this just copies it
        
        // Thought B:
        //   add a new method to Methods, setPixelSettings that all current methods 
        //   just have an empty set
        //   but the Tlc59711Method will store in a member variable
        //   BUT methods don't know anything about the feature settings 

        // Thought C: (Winner winner, chicken dinner)
        //   Leave all current work alone
        //   Set SettingsSize to 2 times SettingsPerChipSize
        //   Consider it at the front of the data buffer
        //   call encode twice, into pData and then pData + SettingsPerChipSize
        //   have our Tlc59711Method know about the two and send the 
        //   already interleaving settings and data

        // settings are at the front of the data stream
        uint8_t* pSet = pData;

        // encode two, for alternating use for EMI reduction 
        Encode(pSet, false);
        Encode(pSet + SettingsPerChipSize, true);
    }

    static uint8_t* pixels([[maybe_unused]] uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        // settings are at the front of the data stream
        return pData + SettingsSize;
    }

    static const uint8_t* pixels([[maybe_unused]] const uint8_t* pData, [[maybe_unused]] size_t sizeData)
    {
        // settings are at the front of the data stream
        return pData + SettingsSize;
    }
};

class Tlc59711RgbFeature :
    public Neo3WordFeature<ColorIndexR, ColorIndexG, ColorIndexB>,
    public Tlc59711ElementsSettings
{
};

class Tlc59711RgbwFeature :
    public Neo4WordFeature<ColorIndexR, ColorIndexG, ColorIndexB, ColorIndexW>,
    public Tlc59711ElementsSettings
{
};
