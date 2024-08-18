//
// NeoPixel_ESP32_LcdParallel - 
// This sketch demonstrates the use of the LCD Parallel method allowing upto 8 or 16 hardware updated channels
// This example only works on the ESP32S3
// 
// The key part of the method name is Esp32LcdX8, 
//    E2p32 (platform specific method),
//    Lcd peripheral, 
//    X8 (8 parallel channel mode, x16 is also supported)
//
// In this example, it demonstrates different ColorFeatures, Method specification, and count per strip
// Note, the first instance of a NeoPixelBus will set the overall timing of all other instances
//
#include <NeoPixelBus.h>

// Demonstrating the use of the first four channels, but the method used allows for eight
NeoPixelBus<NeoBgrFeature, NeoEsp32LcdX8Ws2811Method> strip1(120, 15); // note: older WS2811 and longer strip
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xMethod> strip2(100, 2); // note: modern WS2812 with letter like WS2812b
NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812xInvertedMethod> strip3(100, 4); // note: inverted
NeoPixelBus<NeoGrbwFeature, NeoEsp32LcdX8Sk6812Method> strip4(50, 16); // note: RGBW and Sk6812 and smaller strip

void setup() {
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    // must call begin on all the strips
    strip1.Begin();
    strip2.Begin();
    strip3.Begin();
    strip4.Begin();

    Serial.println();
    Serial.println("Running...");
}

void loop() {
    delay(1000);

    // draw on the strips
    strip1.SetPixelColor(0, RgbColor(255, 0, 0));      // red
    strip2.SetPixelColor(0, RgbColor(0, 127, 0));      // green
    strip3.SetPixelColor(0, RgbColor(0, 0, 53));       // blue
    strip4.SetPixelColor(0, RgbwColor(0, 0, 128, 255)); // white channel with a little blue

    // show them, 
    // only on the last show, no matter the order, will the data be sent
    strip1.Show();
    strip2.Show();
    strip3.Show();
    strip4.Show();
}