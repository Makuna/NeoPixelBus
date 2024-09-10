// DotStarTest
// This example will cycle between showing four pixels as Red, Green, Blue, White
// and then showing those pixels as Black.
//
// There is serial output of the current state so you can confirm and follow along
//

#include <NeoPixelBus.h>

const uint16_t PixelCount = 4; // this example assumes 4 pixels, making it smaller will cause a failure

// make sure to set this to the correct pins
const uint8_t DotClockPin = 9;
const uint8_t DotDataPin = 8;  

uint16_t colorSaturation=32768;

// for software bit bang, with Rgb48Color
NeoPixelBus<Hd108RgbFeature, Hd108Method> strip(PixelCount, DotClockPin, DotDataPin);

// for hardware SPI (best performance but must use hardware pins)
//NeoPixelBus<Hd108RgbFeature, Hd108SpiMethod> strip(PixelCount);

// Rgbw64Color implementation
// NeoPixelBus<Hd108LrgbFeature, DotStarMethod> strip(PixelCount, DotClockPin, DotDataPin);

Rgb48Color red(colorSaturation, 0, 0);
Rgb48Color green(0, colorSaturation, 0);
Rgb48Color blue(0, 0, colorSaturation);
Rgb48Color white(colorSaturation);
Rgb48Color black(0);

// for use with RGB DotStars when using the luminance/brightness global value
// note that its range is only 0 - 31 (31 is full bright) and 
// also note that it is not useful for POV displays as it will cause more flicker
Rgbw64Color redL(colorSaturation, 0, 0, 31); // use white value to store luminance
Rgbw64Color greenL(0, colorSaturation, 0, 31); // use white value to store luminance
Rgbw64Color blueL(0, 0, colorSaturation, 31); // use white value to store luminance
Rgbw64Color whiteL(colorSaturation, colorSaturation, colorSaturation, colorSaturation / 8); // luminance is only 0-31

void setup()
{
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    // this resets all the neopixels to an off state
    strip.Begin();
    strip.ClearTo(black);
    strip.Show();

    Serial.println();
    Serial.println("Running...");
}

void loop()
{
    delay(5000);

    Serial.println("Colors R, G, B, W...");

    // set the colors, 
    strip.SetPixelColor(0, red);
    strip.SetPixelColor(1, green);
    strip.SetPixelColor(2, blue);
    strip.SetPixelColor(3, white);
    strip.Show();


    delay(5000);

    Serial.println("Off ...");

    // turn off the pixels
    strip.SetPixelColor(0, black);
    strip.SetPixelColor(1, black);
    strip.SetPixelColor(2, black);
    strip.SetPixelColor(3, black);
    strip.Show();

}