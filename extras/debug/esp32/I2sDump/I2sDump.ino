#include <NeoPixelBus.h>

#if !defined(ARDUINO_ARCH_ESP32)
@error ESP32 Supported Only
#endif

void setup()
{
    Serial.begin(115200);
    while (!Serial); // wait for serial attach
}

void loop()
{
    Serial.println();
    Serial.println();
    Serial.println("I2S0: ");
    i2sDump(0);

    Serial.println();
    Serial.println();
    Serial.println("I2S1: ");
    i2sDump(1);

    delay(10000);
}
