// PixieTest
// This example will cycle between showing Pixies as Red, Green, Blue, White
// and then showing those Pixies as Black.
//
// There is serial output of the current state so you can confirm and follow along
//

#include <NeoPixelBus.h>

const uint16_t PixelCount = 1; // Number of Pixies daisycahined

#define colorSaturation 128

// Pixie reads data in at 115.2k serial, 8N1.
// Byte order is R1, G1, B1, R2, G2, B2, ... where the first triplet is the
// color of the LED that's closest to the controller. 1ms of silence triggers
// latch. 2 seconds silence (or overheating) triggers LED off (for safety).

// Use either Software Serial or hardwar serial depending on the board

// #include "SoftwareSerial.h"
// #define PIXIEPIN  6 // Pin number for SoftwareSerial output
// SoftwareSerial pixieSerial(-1, PIXIEPIN);

//hardware Serial
#define pixieSerial Serial2

NeoPixelBus<NeoRgbFeature, PixieSerialMethod> strip(PixelCount, &pixieSerial);

RgbColor colors[5];
HslColor hslColors[5];


void setup()
{
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    pixieSerial.begin(115200); // Pixie requires 115200

    //setup array of colors (R,G,B,W,Black)
    colors[0] = RgbColor(colorSaturation, 0, 0);
    colors[1] = RgbColor(0, colorSaturation, 0);
    colors[2] = RgbColor(0, 0, colorSaturation);
    colors[3] = RgbColor(colorSaturation);
    colors[4] = RgbColor(0);

    hslColors[0] = HslColor(colors[0]);
    hslColors[1] = HslColor(colors[1]);
    hslColors[2] = HslColor(colors[2]);
    hslColors[3] = HslColor(colors[3]);
    hslColors[4] = HslColor(colors[4]);

    // this resets all the neopixels to an off state
    strip.Begin();
    strip.Show();

    Serial.println();
    Serial.println("Running...");
}


void loop()
{
    Serial.println("RGB Colors R, G, B, W, Off");
    //set all the Pixies to Red->Green->Blue->White->Black/Off
    for(int j=0; j<5; j++)
    {
      for(int i=0; i<PixelCount; i++)
      {
        strip.SetPixelColor(i, colors[j]);
      }
      strip.Show();
      delay(1000);
    }
    
    Serial.println("HSL Colors R, G, B, W, Off");
    //set all the Pixies to HSL Red->Green->Blue->White->Black/Off
    for(int j=0; j<5; j++)
    {
      for(int i=0; i<PixelCount; i++)
      {
        strip.SetPixelColor(i, hslColors[j]);
      }
      strip.Show();
      delay(1000);
    }

}