#include <Arduino.h>
#include "NeoPixelBus.h"
#include "NeoEsp32I2sXMethod.h"

uint32_t NeoEsp32I2sX8BusZero::s_i2sBufferSize = 0;
uint32_t* NeoEsp32I2sX8BusZero::s_i2sBuffer = nullptr;

size_t NeoEsp32I2sX8BusZero::s_MaxBusDataSize = 0;
uint8_t NeoEsp32I2sX8BusZero::s_UpdateMap = 0;
uint8_t NeoEsp32I2sX8BusZero::s_UpdateMapMask = 0;
uint8_t NeoEsp32I2sX8BusZero::s_BusCount = 0;

