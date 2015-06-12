#include <NeoPixelBus.h>

#define pixelCount 4 // this example assumes 4 pixels, making it smaller will cause a failure
#define pixelPin 8  // make sure to set this to the correct pin

#define colorSaturation 128

NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin);
// NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin, NEO_RGB);
//
// some pixels require the color components to be in a different order
// using the flag NEO_GRB will use the order; green, red, then blue.
//


RgbColor red = RgbColor(colorSaturation, 0, 0);
RgbColor green = RgbColor(0, colorSaturation, 0);
RgbColor blue = RgbColor(0, 0, colorSaturation);
RgbColor white = RgbColor(colorSaturation);
RgbColor black = RgbColor(0);


HslColor hslRed( red );
HslColor hslGreen( green );
HslColor hslBlue( blue );
HslColor hslWhite( white );
HslColor hslBlack( black );
    
void setup()
{
    // this resets all the neopixels to an off state
    strip.Begin();
    strip.Show();
}


void loop()
{
    delay(1000);

    // set the colors, 
    // if they don't match in order, you may need to use NEO_GRB flag
    strip.SetPixelColor(0, red);
    strip.SetPixelColor(1, green);
    strip.SetPixelColor(2, blue);
    strip.SetPixelColor(3, white);
    strip.Show();

    
    delay(3000);

    // turn off the pixels
    strip.SetPixelColor(0, black);
    strip.SetPixelColor(1, black);
    strip.SetPixelColor(2, black);
    strip.SetPixelColor(3, black);
    strip.Show();
    
        delay(1000);

    // set the colors, 
    // if they don't match in order, you may need to use NEO_GRB flag
    strip.SetPixelColor(0, hslRed);
    strip.SetPixelColor(1, hslGreen);
    strip.SetPixelColor(2, hslBlue);
    strip.SetPixelColor(3, hslWhite);
    strip.Show();

    
    delay(3000);

    // turn off the pixels
    strip.SetPixelColor(0, hslBlack);
    strip.SetPixelColor(1, hslBlack);
    strip.SetPixelColor(2, hslBlack);
    strip.SetPixelColor(3, hslBlack);
    strip.Show();
}


