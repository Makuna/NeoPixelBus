/*-------------------------------------------------------------------------
NeoPixel library helper functions for RP2040.

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

#ifdef ARDUINO_ARCH_RP2040

// PIO Programs (cadence)
// --------------------------------------------------------
// use https://wokwi.com/tools/pioasm
// copy relevant parts into NeoRp2040PioCadenceMono3Step and NeoRp2040PioCadenceMono4Step
// 
// 3 step program
// the pulse width is divided by 3, using 33% for each stage
// where 0 = 33% and 1 = 66%
/*
.program rgbic_mono
.side_set 1

; 0 bit
; TH0 TH1 TL1
; +++ ___ ___

; 1 bit
; TH0 TH1 TL1
; +++ +++ ___
.define public TH0 1 ; T1
.define public TH1 1 ; T2
.define public TL1 1 ; T3

.wrap_target
bitloop:
    out x, 1		side 0 [TL1 - 1] ; Side-set still takes place when instruction stalls
    jmp !x do_zero	side 1 [TH0 - 1] ; Branch on the bit we shifted out. Positive pulse
do_one:
    jmp bitloop		side 1 [TH1 - 1] ; Continue driving high, for a long pulse
do_zero:
    nop		side 0 [TH1 - 1] ; Or drive low, for a short pulse
.wrap
*/
//
class NeoRp2040PioCadenceMono3Step
{
protected:
    static constexpr uint8_t wrap_target = 0;
    static constexpr uint8_t wrap = 3;

    static constexpr uint8_t TH0 = 1;
    static constexpr uint8_t TH1 = 1;
    static constexpr uint8_t TL1 = 1;

    // changed from constexpr with initializtion due to 
    // that is only supported in c17+
    static const uint16_t program_instructions[];

public:
    // changed from constexpr with initializtion due to 
    // that is only supported in c17+
    static const struct pio_program program;

    static inline pio_sm_config get_default_config(uint offset)
    {
        pio_sm_config c = pio_get_default_sm_config();

        sm_config_set_wrap(&c, offset + wrap_target, offset + wrap);
        sm_config_set_sideset(&c, 1, false, false);
        return c;
    }

    static constexpr uint8_t bit_cycles = TH0 + TH1 + TL1;
};

// 4 step program
// the pulse width is divided by 4, using 25% for each stage
// where 0 = 25% and 1 = 75%
//
/*
.program rgbic_mono
.side_set 1

; 0 bit
; TH0 TH1 TL1
; +++ ___ ___

; 1 bit
; TH0 TH1 TL1
; +++ +++ ___
.define public TH0 1 ; T1
.define public TH1 2 ; T2
.define public TL1 1 ; T3

.wrap_target
bitloop:
    out x, 1		side 0 [TL1 - 1] ; Side-set still takes place when instruction stalls
    jmp !x do_zero	side 1 [TH0 - 1] ; Branch on the bit we shifted out. Positive pulse
do_one:
    jmp bitloop		side 1 [TH1 - 1] ; Continue driving high, for a long pulse
do_zero:
    nop		side 0 [TH1 - 1] ; Or drive low, for a short pulse
.wrap
*/
//
class NeoRp2040PioCadenceMono4Step
{
protected:
    static constexpr uint8_t wrap_target = 0;
    static constexpr uint8_t wrap = 3;

    static constexpr uint8_t TH0 = 1;
    static constexpr uint8_t TH1 = 2;
    static constexpr uint8_t TL1 = 1;

    // changed from constexpr with initializtion due to 
    // that is only supported in c17+
    static const uint16_t program_instructions[];

public:
    // changed from constexpr with initializtion due to 
    // that is only supported in c17+
    static const struct pio_program program;

    static inline pio_sm_config get_default_config(uint offset)
    {
        pio_sm_config c = pio_get_default_sm_config();

        sm_config_set_wrap(&c, offset + wrap_target, offset + wrap);
        sm_config_set_sideset(&c, 1, false, false);
        return c;
    }

    static constexpr uint8_t bit_cycles = TH0 + TH1 + TL1;
};

// Program Wrapper
// --------------------------------------------------------
constexpr uint c_ProgramNotLoaded = static_cast<uint>(-1);

template<typename T_CADENCE>
class NeoRp2040PioMonoProgram
{
public:
    static inline uint add(PIO pio_instance)
    {
        size_t index = 
#if NUM_PIOS == 2
                (pio_instance == pio0) ? 0 : 1;
#elif NUM_PIOS == 3
                (pio_instance == pio0) ? 0 : (pio_instance == pio1 ? 1 : 2);
#endif
        if (s_loadedOffset[index] == c_ProgramNotLoaded)
        {
            assert(pio_can_add_program(pio_instance, &T_CADENCE::program));
            s_loadedOffset[index] = pio_add_program(pio_instance, &T_CADENCE::program);
        }
        return s_loadedOffset[index];
    }

    static inline void init(PIO pio_instance, 
            uint sm, 
            uint offset, 
            uint pin, 
            float bitrate, 
            uint shiftBits)
    {
        float div = clock_get_hz(clk_sys) / (bitrate * T_CADENCE::bit_cycles);
        pio_sm_config c = T_CADENCE::get_default_config(offset);

        sm_config_set_sideset_pins(&c, pin);

        sm_config_set_out_shift(&c, false, true, shiftBits);
        sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
        sm_config_set_clkdiv(&c, div);

        // Set this pin's GPIO function (connect PIO to the pad)
        pio_gpio_init(pio_instance, pin);

        // Set the pin direction to output at the PIO
        pio_sm_set_consecutive_pindirs(pio_instance, sm, pin, 1, true);

        // Load our configuration, and jump to the start of the program
        pio_sm_init(pio_instance, sm, offset, &c);

        // Set the state machine running
        pio_sm_set_enabled(pio_instance, sm, true);
    }

private:
    static uint s_loadedOffset[NUM_PIOS]; // singlet instance of loaded program, one for each PIO hardware unit
};

template<typename T_CADENCE>
uint NeoRp2040PioMonoProgram<T_CADENCE>::s_loadedOffset[] = 
#if NUM_PIOS == 2
        {c_ProgramNotLoaded, c_ProgramNotLoaded};
#elif NUM_PIOS == 3
        {c_ProgramNotLoaded, c_ProgramNotLoaded, c_ProgramNotLoaded};
#endif


#endif
