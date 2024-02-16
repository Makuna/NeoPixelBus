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

// Four byte settings header per 25 bytes color data
//    12 channel, 16 bit, grouped RGB
// Write Command
// |      OUTTMG - 1 rising edge, 0 falling edge
// |      | EXTGCK - 0 internal clock, 1 SCKI clock
// |      | | TMGRST - 0/1 enable display timer reset mode
// |      | | | DSPRPT - 0/1 enabled auto display repeat
// |      | | | | BLANK - 0 enabled, 1 blank
// |      | | | | | BC (3x7) RGB Brightness control
// |      | | | | | |   red   green    blue
// |      | | | | | |     |       |       | color data (25 bytes) ->
// 100101 1 0 1 1 0 1111111 1111111 1111111 xxxxxxxx xxxxxxxx
// 765432 1 0 7 6 5 4321076 5432107 6543210

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

    void Encode(uint8_t* encoded, bool emiAlternate = false) const
    {
        uint8_t control = Control;

        if (emiAlternate)
        {
            if (control & Tlc69711_Control_EmiRisingEdge)
            {
                control &= ~Tlc69711_Control_EmiRisingEdge;
            }
            else
            {
                control |= Tlc69711_Control_EmiRisingEdge;
            }
        }

        *encoded++ = 0b10010100 | (control & Tlc69711_Control_Mask1);
        *encoded++ = (control & Tlc69711_Control_Mask2) | RedGroupBrightness >> 2;
        *encoded++ = RedGroupBrightness << 5 | GreenGroupBrightness >> 1;
        *encoded = GreenGroupBrightness << 7 | BlueGroupBrightness;
    }

    constexpr uint8_t SizeEncodedSettings = 4;

    const uint8_t RedGroupBrightness;
    const uint8_t GreenGroupBrightness;
    const uint8_t BlueGroupBrightness;
    const uint8_t Control;
};