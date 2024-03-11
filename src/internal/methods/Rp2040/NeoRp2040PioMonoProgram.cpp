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

#include <Arduino.h>
#include "../../NeoUtil.h"
#include "../../NeoBusChannel.h"
#include "../../NeoSettings.h"
#include "NeoRp2040x4Method.h"

#ifdef ARDUINO_ARCH_RP2040

// See NeoRp2040PioCadenceMono3Step, NeoRp2040PioCadenceMono4Step in header for details
// on changing the PIO programs
// 
// these are only required here in the .cpp due to current compiler version doesn't
// support doing this in the header file
// 
//

const uint16_t NeoRp2040PioCadenceMono3Step::program_instructions[] =
        {
        //     .wrap_target
        0x6021, //  0: out    x, 1            side 0     
        0x1023, //  1: jmp    !x, 3           side 1     
        0x1000, //  2: jmp    0               side 1     
        0xa042, //  3: nop                    side 0     
        //     .wrap
        };

const struct pio_program NeoRp2040PioCadenceMono3Step::program =
        {
        .instructions = NeoRp2040PioCadenceMono3Step::program_instructions,
        .length = 4,
        .origin = -1,
        };


const uint16_t NeoRp2040PioCadenceMono4Step::program_instructions[] =
        {
        //     .wrap_target
        0x6021, //  0: out    x, 1            side 0     
        0x1023, //  1: jmp    !x, 3           side 1     
        0x1100, //  2: jmp    0               side 1 [1] 
        0xa142, //  3: nop                    side 0 [1] 
        //     .wrap
        };

const struct pio_program NeoRp2040PioCadenceMono4Step::program =
        {
        .instructions = NeoRp2040PioCadenceMono4Step::program_instructions,
        .length = 4,
        .origin = -1,
        };
#endif