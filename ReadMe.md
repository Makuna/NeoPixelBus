# NeoPixelBus

[![Donate](http://img.shields.io/paypal/donate.png?color=yellow)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=6AA97KE54UJR4)

Arduino NeoPixel library

NOW SUPPORTS esp8266! 
This branch (UartDriven) only supports esp8266, and uses the hardware UART to send the data.  This model is required today if you want to use WiFi due to SDK changes that will cause the bitbang model to crash the SDK WiFi code.  Currently it is unknown if they will fix the SDK so that the bitbang model can ever be used. 
Thanks to stiliface and forkineye for this work.

There is a known issue with this branch that the esp8266 speed must be set to 160mhz to be able to use more than about 50 leds.  There seems to be a pipeline issue where the cpu can't maintain consistent data at 80mhz beyound about 50 pixels.


NEW Animation class provides more flexible animation definitions

Clone this into your Arduino\Library folder

This library is a modification of the Adafruit NeoPixel library.
The Api is similiar, but it removes the overal brightness feature and adds animation support.

## Requirements

If you use this library with Boards other than the Esp8266, you will be required to also have STL present.  Currently I don't know a way to get this and make it available to library in a general way.

While an attempt is being made with this project, the version is out of date for C++ 11 support that Arduino IDE now supports.

https://github.com/maniacbug/StandardCplusplus


## Installing This Library
Create a directory in your Arduino\Library folder named "NeoPixelBus"
Clone (Git) this project into that folder.  
It should now show up in the import list.

## Samples
### NeoPixelTest
this is simple example that sets four neopixels to red, green, blue, and then white in order; and then flashes them.  If the first pixel is green and the second is red, you need to pass the NEO_RGB flag into the NeoPixelBus constructor.
### NeoPixelFun
this is a more complex example, that includes code for three effects, and demonstrates animations.

## API Documentation


### RgbColor object
This represents a color as RGB model and exposes useful methods to manipulate colors.  This is the native color used and is the most effecient.
The color components are Red, Green, and Blue.

#### RgbColor(uint8_t r, uint8_t g, uint8_t b)
instantiates a RgbColor object with the given r, g, b values.

#### RgbColor(uint8_t brightness)
instantiates a RgbColor object with the given brightness. 0 is black, 128 is grey, 255 is white.

#### RgbColor(HslColor color)
instantiates a RgbColor object by converting the HslColor into RGB.

#### uint8_t CalculateBrightness()
returns the general brightness of the pixel, averaging of the color components.

#### void Darken(uint8_t delta)
this will darken the color by the given amount

#### void Lighten(uint8_t delta)
this will lighten the color by the given amount

#### static RgbColor LinearBlend(RgbColor left, RgbColor right, float progress)
this will return a color that is a blend between the given colors.  The amount to blend is given by the value of progress, 0.0 will return the left value, 1.0 will return the right value, 0.5 will return the value between them.

NOTE:  This is not an accurate "visible light" color blend but is fast and in most cases good enough.


### HslColor object
This represents a color as HSL model and exposes useful methods to manipulate colors.  While not the native color used, the extra overhead will be offset by intuitive color components and effects.
The color compoents are Hue, Saturation, and Lightness.
To darken the color, just reduce the L property and to brighten the color, just increase the L property.
To radomly pick a color, just randomly pick a Hue and leave Saturation and Lightness the same.


#### HslColor(uint8_t h, uint8_t s, uint8_t l)
instantiates a HslColor object with the given h, s, l values.  For most colors, a luminace value of 127 is full brightness.  If the color is monochromatic (grey), then set hue and saturdation to zero, and then set luminance between 0 and 255.

#### HslColor(RgbColor color)
instantiates a HslColor object by converter the RgbColor.

#### static HslColor LinearBlend(HslColor left, HslColor right, float progress)
this will return a color that is a blend between the given colors.  The amount to blend is given by the value of progress, 0.0 will return the left value, 1.0 will return the right value, 0.5 will return the value between them.


### NeoPixelBus object
This represents a single NeoPixel Bus that is connected by a single pin.  Please see Adafruit's documentation for details, but the differences are documented below.

#### NeoPixelBus(uint16_t n, uint8_t p, uint8_t t = NEO_GRB | NEO_KHZ800);
instantiates a NewoPixelBus object, with n number of pixels on the bus, over the p pin, using the defined NeoPixel type.
For the exp8266, only the TXD1 pin is supported due to the hardware uart restriction and this argument is ignored.
There are some NeoPixels that address the color values differently, so if you set the green color but it displays as red, use the NEO_RGB type flag.

```
NeoPixelBus strip = NeoPixelBus(4, 8, NEO_RGB | NEO_KHZ800);
```
It is rare, but some older NeoPixels require a slower communications speed, to include this support you must modify the NeoPixel.h file and uncomment the define line and then include the NEO_KHZ400 type flag to enable this slower speed.

```

// v1 NeoPixels aren't handled by default, include the following define before the 
// NeoPixelBus library include to support the slower bus speeds
#define INCLUDE_NEO_KHZ400_SUPPORT 
```
```

NeoPixelBus strip = NeoPixelBus(4, 8, NEO_RGB | NEO_KHZ400);
```

#### void SetPixelColor(uint16_t n, RgbColor c)
This allows setting a pixel on the bus to a color as defined by a color object.	If an animation is actively running on a pixel, the animation will reset the color on the next update.

#### RgbColor GetPixelColor(uint16_t n) const
this allows retrieving the current pixel color

#### void Begin()
this will initialize the NeoPixelBus for use

#### void Show()
this will attempt to update the connected pixels with the current state.

#### bool CanShow() const
this will return that enough time has passed since the last show that another show can be made.  The show will block by calling delay(0) until this is true.

#### void ClearTo(RgbColor color)
this will set all pixels to the given color

#### bool IsDirty(uint16_t n) const
returns if there are changes that need to be shown

#### void Dirty()
this will mark the current state as dirty so that Show() will think the pixel colors have changed.  This is useful when you modify the biffer directly.

#### void ResetDirty()
this will mark the current state as not dirty so that Show() will thing the pixels have the current colors.

#### uint8_t* Pixels() const
this will return the underlying buffer used to store the current pixel colors in its native format.  This is usefull for advanced effets that would be slower by using GetPixelColor and SetPixelColor.
Each pixel takes up 3 bytes, the order of the color components for each pixel is dependent on the physical NeoPixels you are using.

#### uint16_t PixelCount() const
this will return the number of pixels in the underlying buffer.


### NeoPixelAnimator object
This manages the animations for a single NeoPixelBus.  All time values are in milliseconds.
NOTE:  NeoPixelBus::Show() must still be called to push the color state to the physical NeoPixels.

#### bool IsAnimating() const
this method will return the current animation state.  It will return false if there are no active animations.

#### bool IsAnimating(uint16_t n) const
this method will return the current animation state for the given pixel.  It will return false if there are no active animations for the given pixel.

#### void StartAnimation(uint16_t n, uint16_t time, AnimUpdateCallback animUpdate)
this method will start an animation for the given pixel, over the given time, using the animUpdate function to provide the effect.  The animUpdate method will be called on short cyles given a progress value of zero to one that present the time progress from start to finish.

#### void StopAnimation(uint16_t n)
this method will stop the current running animation on the given pixel.  The pixel will be left in what ever state it was in last.

#### void UpdateAnimations(uint32_t maxDeltaMs = 1000)
this method will allow the animations to be processed and update the pixel color state. It should be called often within the Loop() function.
The argument maxDeltaMs is used to cap the calculations for animation time to this value.  Due to other code that may take large amounts of time, it could cause an animation to jump due to the time passage.  By changing this value to a lower number like 100ms, you will guarentee no animation step will be large but the timing of the animation may no longer match real time.

#### void IsPaused()
this method will return if the animations are paused.  See Pause() and Resume().  This state is distinct from IsAnimating().

#### void Pause()
this method will pause all animations, thus suspending the animations at their current state and no longer changing the colors. This will not effect other state, so IsAnimating will continue to return true if there are suspended animations.

#### void Resume()
this method will resume all animations, thus continuing all animations where they were at when Pause() was called.

#### void FadeTo(uint16_t time, RgbColor color)
this will setup an animation for all pixels to linear fade between the current color and the given color over the time given.  The time is in milliseconds.


