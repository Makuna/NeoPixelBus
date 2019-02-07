// NeoSegmentBus
// This example will demonstrate using the NeoSegmentBus which provides support for a
// seven segment LED digit driven by three WS2811; connected in series with other digits
//
// See https://shop.idlehandsdev.com/products/addressable-7-segment-display for a hardware example
//
// This example will print current seconds since start of the Arduino 
// with a digit animating a circling path for each second
//

#include <NeoPixelSegmentBus.h>
#include <NeoPixelAnimator.h>

const uint16_t DigitCount = 5; // Max Digits, not segments, not pixels
const uint8_t BusPin = 2;  // make sure to set this to the correct pin, ignored for Esp8266
const uint16_t CycleDigit = 0;
const uint16_t SecondsDigit = 1;

#define brightness 128

NeoPixelSegmentBus<SevenSegmentFeature, Neo800KbpsMethod> strip(DigitCount, BusPin);

enum Animation
{
    Animation_Cycle, // animation for the cycle indicator
    Animation_Fade, // animation for fade of seconds
    Animation_COUNT
};

NeoPixelAnimator animations(Animation_COUNT); 

void CycleAnimation(const AnimationParam& param)
{
    // calculate which segment should be on using the animation progress
    uint8_t bitfield = 1 << (uint8_t)(param.progress * LedSegment_F);
    // instant a digit with that segment on
    SevenSegDigit digit(bitfield, brightness);
    // apply it to the strip
    strip.SetPixelColor(CycleDigit, digit); 
}

SevenSegDigit StartingDigit;
SevenSegDigit EndingDigit;

void FadeAnimation(const AnimationParam& param)
{
    // create a digit that is a blend between the last second
    // value and the next second value using the animation progress
    SevenSegDigit digit = SevenSegDigit::LinearBlend(
            StartingDigit,
            EndingDigit,
            param.progress);
    // apply it to the strip
    strip.SetPixelColor(SecondsDigit, digit); 
}

uint32_t lastSeconds;

void setup()
{
    lastSeconds = millis() / 1000;

    strip.Begin();

    // format and display new complete value
    String display(lastSeconds);
    strip.SetString(SecondsDigit, display, brightness);

    strip.Show(); 
}

void loop()
{
    uint32_t seconds = millis() / 1000;
    
    // when the seconds change, start animations for the update
    //
    if (seconds != lastSeconds)
    {
        // set the seconds fade animation properties
        // first retain what is already present at the last digit as
        // a starting point for the animation
        StartingDigit = strip.GetPixelColor(SecondsDigit);
        
        // format and display new complete value
        String display(seconds);
        strip.SetString(SecondsDigit, display, brightness);

        // then get the last digit as what to animate to
        EndingDigit = strip.GetPixelColor(SecondsDigit); 

        // start the seconds fade animation
        animations.StartAnimation(Animation_Fade, 1000, FadeAnimation);

        // start the cycle animation for the next second
        animations.StartAnimation(Animation_Cycle, 1000, CycleAnimation);

        lastSeconds = seconds;
    }

    if (animations.IsAnimating())
    {
        // the normal loop just needs these two to run the active animations
        animations.UpdateAnimations();
        strip.Show();
    }
}

