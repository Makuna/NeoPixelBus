#pragma once

#include <string.h>
#include <stdio.h>
#include "stdlib.h"

// UnitDecimalToFractionClks is used inside both c and c++ files, 
// so to make sure there are no duplicate definitions in c and c++ calling conventions
// we wrap it up
//
#ifdef __cplusplus
extern "C" {
#endif

void UnitDecimalToFractionClks(uint8_t* resultN,
    uint8_t* resultD,
    double unitDecimal,
    double accuracy);

#ifdef __cplusplus
}
#endif

