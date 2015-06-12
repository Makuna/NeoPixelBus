#include <NeoPixelBus.h>
#include <functional>

#define pixelCount 16 // make sure to set this to the number of pixels in your strip
#define pixelPin 8  // make sure to set this to the correct pin

NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin);
NeoPixelAnimator animations(&strip); // NeoPixel animation management object

uint16_t effectState = 0;  // general purpose varible used to store effect state


void setup()
{
    strip.Begin();
    strip.Show();
    SetRandomSeed();
}


void loop()
{
    // There are four fun functions that implement different effects
    // uncomment one at a time and upload to see the effect

    // LoopAround(192, 200); // very interesting on rings of NeoPixels
    PickRandom(128);
    // FadeInFadeOutRinseRepeat(192);
    // LoopFade(192, 10);

    // wait until no more animations are running
    while (animations.IsAnimating())
    {
        animations.UpdateAnimations();
        strip.Show();
        delay(31); // ~30hz change cycle
    }

}

void FadeInFadeOutRinseRepeat(uint8_t peak)
{
    if (effectState == 0)
    {
        for (uint8_t pixel = 0; pixel < pixelCount; pixel++)
        {
            uint16_t time = random(800, 1000);
            RgbColor originalColor = strip.GetPixelColor(pixel);
            RgbColor color = RgbColor(random(peak), random(peak), random(peak));

            // define the effect to apply, in this case linear blend
            AnimUpdateCallback animUpdate = [=](float progress)
            {
                // progress will start at 0.0 and end at 1.0
                RgbColor updatedColor = RgbColor::LinearBlend(originalColor, color, (uint8_t)(255 * progress));
                strip.SetPixelColor(pixel, updatedColor);
            };
            animations.StartAnimation(pixel, time, animUpdate);
        }
    }
    else if (effectState == 1)
    {
        for (uint8_t pixel = 0; pixel < pixelCount; pixel++)
        {
            uint16_t time = random(600, 700);
            RgbColor originalColor = strip.GetPixelColor(pixel);

            // define the effect to apply, in this case linear blend
            AnimUpdateCallback animUpdate = [=](float progress)
            {
                // progress will start at 0.0 and end at 1.0
                RgbColor updatedColor = RgbColor::LinearBlend(originalColor, RgbColor(0, 0, 0), (uint8_t)(255 * progress));
                strip.SetPixelColor(pixel, updatedColor);
            };
            // start the animation
            animations.StartAnimation(pixel, time, animUpdate);
        }
    }
    effectState = (effectState + 1) % 2; // next effectState and keep within the number of effectStates

}

void PickRandom(uint8_t peak)
{
    // pick random set of pixels to animate
    uint8_t count = random(pixelCount);
    while (count > 0)
    {
        uint16_t pixel = random(pixelCount);

        // configure the animations
        HslColor color = HslColor(random(255), peak, peak);
        uint16_t time = random(100, 400);
        HslColor originalColor = strip.GetPixelColor(pixel);

        // define the effect to apply, in this case linear blend
        AnimUpdateCallback animUpdate = [=](float progress)
        {
            // progress will start at 0.0 and end at 1.0
            HslColor updatedColor = HslColor::LinearBlend(originalColor, color, (uint8_t)(255 * progress));
            strip.SetPixelColor(pixel, updatedColor);
        };
        // start the animation
        animations.StartAnimation(pixel, time, animUpdate);

        count--;
    }
}

void LoopAround(uint8_t peak, uint16_t speed)
{
    // Looping around the ring sample
    uint16_t time = speed;


    // apply an animations to current pixel and previous 5 pixels
    for (uint16_t offset = 5; offset >= 0; offset--)
    {
        uint16_t pixel = (effectState + (pixelCount - offset)) % pixelCount;
        RgbColor originalColor = strip.GetPixelColor(pixel);
        RgbColor color;

        if (offset == 0)
        {
            // newest one fades up to a random color
            color = RgbColor(random(peak), random(peak), random(peak));
        }
        else if (offset == 5)
        {
            // oldest fades to black
            color = RgbColor(0, 0, 0);
        }
        else
        {
            // all others just dim
            color = originalColor;
            color.Darken(color.CalculateBrightness() / 2);
        }

        // define the effect to apply, in this case linear blend
        AnimUpdateCallback animUpdate = [=](float progress)
        {
            // progress will start at 0.0 and end at 1.0
            RgbColor updatedColor = RgbColor::LinearBlend(originalColor, color, (uint8_t)(255 * progress));
            strip.SetPixelColor(pixel, updatedColor);
        };

        // start the animation
        animations.StartAnimation(pixel, time, animUpdate);
    }

    // move to next pixel 
    effectState = (effectState + 1) % pixelCount;
}

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
