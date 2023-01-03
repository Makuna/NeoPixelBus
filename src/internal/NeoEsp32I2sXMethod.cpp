#include <Arduino.h>
#include "NeoPixelBus.h"
#include "NeoEsp32I2sXMethod.h"

#if defined(CONFIG_IDF_TARGET_ESP32S2)
	NeoEspI2sContext NeoEsp32I2s0Mux8Bus::s_context0 = NeoEspI2sContext(8);
#else
	NeoEspI2sContext NeoEsp32I2s0Mux16Bus::s_context0 = NeoEspI2sContext(16);
#endif
NeoEspI2sContext NeoEsp32I2s1Mux8Bus::s_context1 = NeoEspI2sContext(8);


