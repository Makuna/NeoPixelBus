// NeoPixelParallel
// This example will demonstrate the use of the X parallel method 
// defined NeoPixelBus.
//
// There is serial output of the current state so you can confirm and 
// follow along.
//

#include <NeoPixelBus.h>

// define the NPB we want,
// using GRB color order, the normal one for a WS2812x LEDs
// x4 parallel channels method for WS2812x LEDs
//   not all platforms can support parallel channels
//   x8 and x16 maybe available depending on your platform
// 
// ESP32 - x4, x8, x16 (x4 aliased to x8)
// ESP32S2 - x4, x8, x16 (x4 aliased to x8)
// ESP32S3 - x4, x8, x16 (x4 aliased to x8)
// RP2040 - x4 only
//
typedef NeoPixelBus<NeoGrbFeature, X4Ws2812xMethod> NPB;

// make sure to set these to the correct pins and count for your setup
// while the x4 parallel was used, 
//   you can use less than all the defined channels
NPB strips[] = { {60, 15},
                 {60, 2},
                 {90, 4},
                 {30, 16} };

void setup()
{
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    // must call begin for each strip
    for (size_t strip = 0; strip < countof(strips); strip++)
    {
        strips[strip].Begin();
    }

    Serial.println();
    Serial.println("Running...");
}


void loop()
{
    delay(5000);

    //  show R,G,B,W in order on all strips
    //
    Serial.println("Colors R, G, B, W...");

    for (size_t strip = 0; strip < countof(strips); strip++)
    {
        strips[strip].SetPixelColor(0, { 255,0,0 }); // red;
        strips[strip].SetPixelColor(1, { 0,255,0 }); // green
        strips[strip].SetPixelColor(2, { 0,0,255 }); // blue
        strips[strip].SetPixelColor(3, { 255,255,255 }); // white

        // only after all the strips show() method is called will 
        // they actually get updated
        strips[strip].Show();  
    }


    delay(5000);

    // clear all strips to black
    //
    Serial.println("Colors off...");

    for (size_t strip = 0; strip < countof(strips); strip++)
    {
        strips[strip].ClearTo( 0 ); // black

        // only after all the strips show() method is called will 
        // they actually get updated
        strips[strip].Show();
    }
}

