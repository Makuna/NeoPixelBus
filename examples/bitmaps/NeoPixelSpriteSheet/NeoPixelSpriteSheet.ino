// NeoPixelSpriteSheet
// This example will use a NeoVerticalSpriteSheet to animate a 
// moving Cylon Red Eye back and forth across the 
// the full collection of pixels on the strip. 
//


#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

// The actual image is contained in the data structure in the CylonRgb.h file
#include "CylonRgb.h"

const uint16_t PixelCount = myImageWidth; // the sample images are meant for 16 pixels, so we use the same for pixelcount
const uint16_t PixelPin = 2; // for esp8266 the pin is ignored in most methods
const uint16_t AnimCount = 1; // we only need one

NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

// sprite sheet stored in progmem using the same pixel feature as the NeoPixelBus
NeoVerticalSpriteSheet_P<RgbColor> spriteSheet(
    myImageWidth, // image width and sprite width since its vertical sprite sheet
    myImageHeight,  // image height
    1, // sprite is only one pixel high in this example
    myImage);

// animation state
uint16_t indexSprite;

void LoopAnimUpdate(const AnimationParam& param)
{
    // wait for this animation to complete,
    // we are using it as a timer
    if (param.state == AnimationState_Completed)
    {
        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);

        // draw the next frame in the sprite
        spriteSheet.Blt(strip, 0, indexSprite);
        indexSprite = (indexSprite + 1) % myImageHeight; // increment and wrap
    }
}

void setup()
{
    strip.Begin();
    strip.Show();

    indexSprite = 0;

    // we use the index 0 animation to time how often we rotate all the pixels
    animations.StartAnimation(0, 60, LoopAnimUpdate);
}


void loop()
{
    // this is all that is needed to keep it running
    // and avoiding using delay() is always a good thing for
    // any timing related solutions
    animations.UpdateAnimations();
    strip.Show();
}
