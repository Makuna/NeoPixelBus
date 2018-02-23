// NeoPixelChangeDuration
// This example will demonstrate the NeoPixelAnimator::setDuration
// function to change the duration of a simple color fading
// application without the need to restart the animation from time 0. 
// Useful if you would like an animation to speed up/slow down based on 
// user input. 
// This example also showcases some gamma fading techniques that are provided
// by the library. 
// A small comment is made towards the end of this example regarding how to arbitrarily set the progress of an animation, using the NeoPixelAnimator::setProgress() function call

// See the NeoPixelAnimation.ino example for more information on
// how to use the base NeoPixelAnimator class effectively

// Tested on the adafruit circuit playground, but should be usable 
// on other platforms as well
//
// Written by Nigel Michki <nigeil@yahoo.com>


// =====Library includes=====
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <Arduino.h>


// =====Definitions=====
// define the number of pixels
#define N_PIXELS 10		// for original circuit playground; change as needed
#define NEOPIXEL_PIN 17 // for original circuit playground; change as needed
#define ANIMATION_COUNT 1 // only one animation

// A list of reasonable animation durations (here, in NEO_CENTISECONDS, based on the NeoPixelBus speed selected below)
const int durations[6] = {700, 1000, 1400, 2500, 3250, 4000};


// =====Important declarations=====
// The index we'll use to keep track of the currently selected duration
uint16_t duration_ind;

// make the neopixel bus/driver
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(N_PIXELS, NEOPIXEL_PIN);

// and the animation handler
NeoPixelAnimator animate_me(ANIMATION_COUNT, NEO_CENTISECONDS);

// We're going to use gamma correction here to fade the pixel
// colors in a 'truer' fashion
NeoGamma<NeoGammaTableMethod> color_gamma;

// The current time; we'll use this to decide when to update the animation
// duration in this example in an arbitrary/semi-random fashion
uint16_t current_time;

// =====Helper functions=====
// Define the animation update callback function
void RainbowGammaFadeAnimationUpdate(const AnimationParam& param) {
	RgbColor current_color; 
	float current_progress;

	// calculate the current progress based on a 'gamma' easing equation (not necessary, but looks the most natural for a gamma-corrected set of colors)
	current_progress = NeoEase::Gamma(param.progress);
	
	// calculate the current color based on a linear blend between the same HSL color (a light purple) around the longest possible distance
	current_color = RgbColor(HslColor::LinearBlend<NeoHueBlendLongestDistance>(HslColor(0.88f,1.0f,0.5f), HslColor(0.88f,1.0f,0.5f), current_progress));
	
	// and gamma correct this color (can be combined with the previous step)
	current_color = color_gamma.Correct(current_color);

	// update the pixel colors
  	for (int i=0; i<N_PIXELS; i++){
    	strip.SetPixelColor(i, current_color);
  	}
}


// =====Setup=====
void setup() {
	// Set the neopixel pin as an output
	pinMode(NEOPIXEL_PIN, OUTPUT);

	// sane starting duration index
	duration_ind = 0;

	// Start the neopixel animation up (animation 0)
	strip.Begin();
	animate_me.StartAnimation(0, durations[duration_ind], RainbowGammaFadeAnimationUpdate);

}


// =====Main program loop=====
void loop() {
	// get the 'current time'
	current_time = (millis() % 1000);

	// check if the animation has ended; restart if this is the case
	if (animate_me.IsAnimating() == false){
		animate_me.StopAnimation(0);
		animate_me.StartAnimation(0, durations[duration_ind], RainbowGammaFadeAnimationUpdate);
	}

	// this is an arbitrary update condition, but you should see changes to
	// the duration every time millis() % 1000 == 0
	// (recommended: hook up a push button and change this duration when 
	//  the button is pushed - much more interesting)
	if (current_time == 0) {
		duration_ind = (duration_ind + 1) % 6; // pick the next duration (6 options)
		
		// updates the animation duration without changing its current progress
		animate_me.setDuration(0, durations[duration_ind]); 
		
		// To instead change the progress of an animation directly, use the NeoPixelAnimator::setProgress(uint16_t animationIndex, float newProgress) function call, providing a newProgress value in the range [0,1].
	}

	animate_me.UpdateAnimations();
	strip.Show();
}
