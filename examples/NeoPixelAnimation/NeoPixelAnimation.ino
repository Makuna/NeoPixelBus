#include <NeoPixelBus.h>
#include <functional>

#define pixelCount 4 // make sure to set this to the number of pixels in your strip
#define pixelPin 2  // make sure to set this to the correct pin, ignored for UartDriven branch

NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin);

// NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin, NEO_RGB);
//
// some pixels require the color components to be in a different order
// using the flag NEO_GRB will use the order; green, red, then blue.
// NEO_RGB, NEO_GRB. and NEO_BRG are supported
//

// NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin, NEO_KHZ400);
//
// some pixels require an alternate speed, mostly first generation leds only
// using the flag NEO_KHZ400 send out the data slower to support these; but you must also
// define the flag to turn on this extra support
//#define INCLUDE_NEO_KHZ400_SUPPORT 
//

// NeoPixel animation management object
NeoPixelAnimator animations(&strip, NEO_CENTISECONDS); 
// since the normal animation time range is only about 65 seconds, by passing timescale value
// to the NeoPixelAnimator constructor we can increase the time range, but we also increase
// the time between the animation updates.   
// NEO_CENTISECONDS will update the animations every 100th of a second rather than the default
// of a 1000th of a second, but the time range will now extend from about 65 seconds to about
// 10.9 minutes.  But you must remember that the values passed to StartAnimations are now 
// in centiseconds.
//
// Possible values from 1 to 32768, and there some helpful constants defined as...
// NEO_MILLISECONDS        1    // ~65 seconds max duration, ms updates
// NEO_CENTISECONDS       10    // ~10.9 minutes max duration, centisecond updates
// NEO_DECISECONDS       100    // ~1.8 hours max duration, decisecond updates
// NEO_SECONDS          1000    // ~18.2 hours max duration, second updates
// NEO_DECASECONDS     10000    // ~7.5 days, 10 second updates
//

void SetRandomSeed()
{
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    // Serial.println(seed);
    randomSeed(seed);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    strip.Begin();
    strip.Show();

    SetRandomSeed();

    // just pick some colors
    for (uint16_t pixel = 0; pixel < pixelCount; pixel++)
    {
        RgbColor color = RgbColor(random(255), random(255), random(255));
        strip.SetPixelColor(pixel, color);
    }

    Serial.println();
    Serial.println("Running...");
}

void SetupAnimationSet()
{
    // setup some animations
    for (uint16_t pixel = 0; pixel < pixelCount; pixel++)
    {
        const uint8_t peak = 128;

        // pick a random duration of the animation for this pixel
        // since values are centiseconds, the range is 8 - 10 seconds
        uint16_t time = random(800, 1000);

        // each animation starts with the color that was present
        RgbColor originalColor = strip.GetPixelColor(pixel);
        // and ends with a random color
        RgbColor color = RgbColor(random(peak), random(peak), random(peak));

        // we must supply a function that will define the animation, in this example
        // we are using "lambda expression" to define the function inline, which gives
        // us an easy way to "capture" the originalColor and color for the call back.
        //
        // this function will get called back when ever the animation needs to change
        // the state of the pixel, it will provide a animation progress value
        // from 0.0 (start of animation) to 1.0 (end of animation)
        //
        // we use this progress value to define how we want to animate in this case
        // we call RgbColor::LinearBlend which will return a color blended between
        // the values given, by the amount passed, hich is also a float value from 0.0-1.0.
        // then we set the color.
        //
        AnimUpdateCallback animUpdate = [=](float progress)
        {
            // progress will start at 0.0 and end at 1.0
            RgbColor updatedColor = RgbColor::LinearBlend(originalColor, color, progress);
            strip.SetPixelColor(pixel, updatedColor);

            // enable these lines to see the values on each update call  
            // note that doing this can take time to send the serial and may effect
            // timing, but using the longer period of centiseconds should allow this
            // without too much problems         
            /*

            Serial.print("[");
            Serial.print(pixel);
            Serial.print("] ");
            Serial.print("(");
            Serial.print(progress);
            Serial.print(") - (");
            Serial.print(updatedColor.R);
            Serial.print(",");
            Serial.print(updatedColor.G);
            Serial.print(",");
            Serial.print(updatedColor.B);
            Serial.println(")");

            */
        };

        // now use the animation properties we just calculated and start the animation
        // which will continue to run and call the update function until it completes
        animations.StartAnimation(pixel, time, animUpdate);
    }
}

void loop()
{
    if (animations.IsAnimating())
    {
        // the normal loop just needs these two to run the active animations
        animations.UpdateAnimations();
        strip.Show();
    }
    else
    {
        Serial.println();
        Serial.println("Setup Next Set...");
        // example function that sets up some animations
        SetupAnimationSet();
    }
}

