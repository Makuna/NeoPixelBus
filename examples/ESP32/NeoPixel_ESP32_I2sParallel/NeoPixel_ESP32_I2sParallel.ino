
// NeoPixel_ESP32_I2sParallel - This example only works on the ESP32
// This sketch demonstrates the use of the I2S Parallel method allowing upto 8 hardware updated channels
// WARNING:  Currently underdevelopement and this sketch is used for testing/demonstration; do not consider it a best practices sketch.

#include <NeoPixelBus.h>

const uint16_t PixelCount = 32; // this example assumes 4 pixels, making it smaller will cause a failure
const uint8_t DebugPin = 5; // used for logic anaylyser trigger capture 

// moved construction into Setup() so we can capture log output into Serial
//
NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method>* strip0;
NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method>* strip1;
NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method>* strip2;
NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method>* strip3;

// focusing on the first four channels for now
//NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method> strip4(PixelCount, PixelPin);
//NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method> strip5(PixelCount, PixelPin);
//NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method> strip6(PixelCount, PixelPin);
//NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method> strip7(PixelCount, PixelPin);

void setup() {
    Serial.begin(115200);
    while (!Serial); // wait for serial attach
    
    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    strip0 = new NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method>(PixelCount, 15);
    strip1 = new NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method>(PixelCount, 2);
    strip2 = new NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method>(PixelCount, 4);
    strip3 = new NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1X8Ws2812Method>(PixelCount, 16);
      
    strip0->Begin();
    Serial.println(" 1");
    strip1->Begin();
    Serial.println(" 2");
    strip2->Begin();
    Serial.println(" 3");
    strip3->Begin();
    Serial.println(" 4");

    pinMode(DebugPin, OUTPUT);
    digitalWrite(DebugPin, LOW);
  
    Serial.println();
    Serial.println("Running...");
}

void loop() {
    delay(5000);

    Serial.println("On ...");
    strip0->SetPixelColor(0, 255);
    strip1->SetPixelColor(0, 127);
    strip2->SetPixelColor(0, 63);
    strip3->SetPixelColor(0, 31);

    digitalWrite(DebugPin, HIGH);
    
    strip0->Show();
    strip1->Show();
    strip2->Show();
    strip3->Show();
    
    digitalWrite(DebugPin, LOW);

    delay(5000);

    Serial.println("Off ...");
    strip0->SetPixelColor(0, 0);
    strip1->SetPixelColor(0, 0);
    strip2->SetPixelColor(0, 0);
    strip3->SetPixelColor(0, 0);

    strip0->Show();
    strip1->Show();
    strip2->Show();
    strip3->Show();
}
