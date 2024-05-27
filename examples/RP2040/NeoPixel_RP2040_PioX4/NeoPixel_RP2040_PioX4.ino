//
// NeoPixel_RP2040_PioX4 - 
// This sketch demonstrates the use of the PIO method allowing upto 4 instances
// Per one of the two PIO channels
// 
// This example only works on the RP2040
// 
// The key part of the method name is Rp2040Pio1X4, 
//    Rp2040 (platform specific method),
//    PIO Channel 1 (most commonly available), 
//    X4 (4 instances allowed)
//
// In this example, it demonstrates different ColorFeatures, Method specification, and count per strip
//
#include <NeoPixelBus.h>

// Demonstrating the use of the four channels
NeoPixelBus<NeoBgrFeature, NeoRp2040Pio1X4Ws2811Method> strip1(120, 15); // note: older WS2811 and longer strip
NeoPixelBus<NeoGrbFeature, NeoRp2040Pio1X4Ws2812xMethod> strip2(100, 2); // note: modern WS2812 with letter like WS2812b
NeoPixelBus<NeoGrbFeature, NeoRp2040Pio1X4Ws2812xInvertedMethod> strip3(100, 4); // note: inverted
NeoPixelBus<NeoGrbwFeature, NeoRp2040Pio1X4Sk6812Method> strip4(50, 16); // note: RGBW and Sk6812 and smaller strip

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
    // each strip will start sending on their show call
    strip1.Show();
    strip2.Show();
    strip3.Show();
    strip4.Show();
}
