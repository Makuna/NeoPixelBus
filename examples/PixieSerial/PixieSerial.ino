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

// Use either Software Serial or hardware serial depending on the board

// #include "SoftwareSerial.h"
// #define PIXIEPIN  6 // Pin number for SoftwareSerial output
// SoftwareSerial pixieSerial(-1, PIXIEPIN);

//hardware Serial
#define pixieSerial Serial2

NeoPixelBus<NeoRgbFeature, PixieStreamMethod> strip(PixelCount, &pixieSerial);

RgbColor colors[5];

void setup()
{
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    // Pixie requires 115200 buard rate and 8N1 (eight bits, no parity, 1 stop bit). 
    pixieSerial.begin(115200, SERIAL_8N1); 

    //setup array of colors (R,G,B,W,Black)
    colors[0] = RgbColor(colorSaturation, 0, 0);
    colors[1] = RgbColor(0, colorSaturation, 0);
    colors[2] = RgbColor(0, 0, colorSaturation);
    colors[3] = RgbColor(colorSaturation);
    colors[4] = RgbColor(0);

    // this resets all the Pixies to an off state
    strip.Begin();
    strip.Show();

    Serial.println();
    Serial.println("Running...");
}


void loop()
{
    Serial.println("RGB Colors R, G, B, W, Off");
    // Cycle all the Pixies through Red->Green->Blue->White->Black/Off from the colors array
    for(int colorIndex=0; colorIndex<5; colorIndex++)
    {
      for(int pixelIndex=0; pixelIndex<PixelCount; pixelIndex++)
      {
        strip.SetPixelColor(pixelIndex, colors[colorIndex]);
      }
      strip.Show(); 
      delay(1000);
      // As a safety feature, Pixie will shutoff if no data is received >2 seconds
      // Adafruit recomends sending data <1 second.  If needed, .Show() can be called 
      // again to resend the existing color value to reset this timer.
      
    }

}