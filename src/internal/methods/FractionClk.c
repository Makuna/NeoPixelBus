/*-------------------------------------------------------------------------
NeoPixel library helper functions.

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

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "stdlib.h"


// UnitDecimalToFractionClks
// Given unit decimal (floating point value less than 1.0), 
// return a fraction numerator and denominator that closely matches that 
//
// resultN - the address of the variable to place the numerator result
// resultD - the address of the variable to place the denominator result
// unitDecimal - the decimal value that is less than 1.0 and greater than/equal 0.0
// accuracy - the accuracy needed to match for a fractional (0.000001 is a good start)
//
void UnitDecimalToFractionClks(uint8_t* resultN,
    uint8_t* resultD,
    double unitDecimal,
    double accuracy)
{
    if (unitDecimal <= accuracy)
    {
        // no fractional 
        *resultN = 0;
        *resultD = 1;
        return;
    }
    else if (unitDecimal <= (1.0 / 63.0))
    {
        // lowest fractional 
        *resultN = 0;
        *resultD = 2;
        return;
    }
    else if (unitDecimal >= (62.0 / 63.0))
    {
        // highest fractional
        *resultN = 2;
        *resultD = 2;
        return;
    }

    //    printf("\nSearching for %f\n", unitDecimal);

    // The lower fraction is 0 / 1
    uint16_t lowerN = 0;
    uint16_t lowerD = 1;
    double lowerDelta = unitDecimal;

    // The upper fraction is 1 / 1
    uint16_t upperN = 1;
    uint16_t upperD = 1;
    double upperDelta = 1.0 - unitDecimal;

    uint16_t closestN = 0;
    uint16_t closestD = 1;
    double closestDelta = lowerDelta;

    for (;;)
    {
        // The middle fraction is 
        // (lowerN + upperN) / (lowerD + upperD)
        uint16_t middleN = lowerN + upperN;
        uint16_t middleD = lowerD + upperD;
        double middleUnit = (double)middleN / middleD;

        if (middleD > 63)
        {
            // exceeded our clock bits so break out
            // and use closest we found so far
            break;
        }

        if (middleD * (unitDecimal + accuracy) < middleN)
        {
            // middle is our new upper
            upperN = middleN;
            upperD = middleD;
            upperDelta = middleUnit - unitDecimal;
        }
        else if (middleN < (unitDecimal - accuracy) * middleD)
        {
            // middle is our new lower
            lowerN = middleN;
            lowerD = middleD;
            lowerDelta = unitDecimal - middleUnit;
        }
        else
        {
            // middle is our best fraction
            *resultN = middleN;
            *resultD = middleD;

            //            printf(" Match %d/%d = %f (%f)\n", middleN, middleD, middleUnit, unitDecimal - middleUnit);
            return;
        }

        // track the closest fraction so far (ONLY THE UPPER, so allow only slower Kbps)
        //
        //if (upperDelta < lowerDelta)
        {
            if (upperDelta < closestDelta)
            {
                closestN = upperN;
                closestD = upperD;
                closestDelta = upperDelta;

                //                printf(" Upper %d/%d = %f (%f)\n", closestN, closestD, middleUnit, closestDelta);
            }
        }
        /*
        else
        {
            if (lowerDelta < closestDelta)
            {
                closestN = lowerN;
                closestD = lowerD;
                closestDelta = lowerDelta;

                printf(" Lower %d/%d = %f (%f)\n", closestN, closestD, middleUnit, closestDelta);
            }
        }
        */
    }


    //    printf(" Closest %d/%d = %f (%f)\n\n", closestN, closestD, (double)closestN / closestD, closestDelta);
    // no perfect match, use the closest we found
    //
    *resultN = closestN;
    *resultD = closestD;
}

