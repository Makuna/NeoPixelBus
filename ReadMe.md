# NeoPixelBus

[![Donate](http://img.shields.io/paypal/donate.png?color=yellow)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=6AA97KE54UJR4)

Arduino NeoPixel library

ESP8266 CUSTOMERS PLEASE READ:  While this branch does work with the esp8266, due to the latest SDK releases it will not function reliably when WiFi is being used.  Therefore I suggest you use the DmaDriven or UartDriven branches, which both include solutions that will work with WiFi on.  Further they contains enhancements that just can't be supported on AVR platform.  Including HslColor object and an enhanced animator manager.

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Makuna/NeoPixelBus?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

Clone this into your Arduino\Library folder

This library is a modification of the Adafruit NeoPixel library.
The Api is similiar, but it removes the overal brightness feature and adds animation support.

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
This represents a color and exposes useful methods to manipulate colors.

#### RgbColor(uint8_t r, uint8_t g, uint8_t b)
instantiates a RgbColor object with the given r, g, b values.

#### RgbColor(uint8_t brightness)
instantiates a RgbColor object with the given brightness. 0 is black, 128 is grey, 255 is white.

#### uint8_t CalculateBrightness()
returns the general brightness of the pixe, averaging color.

#### void Darken(uint8_t delta)
this will darken the color by the given amount, blending toward black.  This method is destructive in that you can't expect to then call lighten and return to the original color.

#### void Lighten(uint8_t delta)
this will lighten the color by the given amount, blending toward white.  This method is destructive in that you can't expect to then call darken and return to the original color.

#### static RgbColor LinearBlend(RgbColor left, RgbColor right, uint8_t progress)
this will return a color that is a blend between the given colors.  The amount to blend is given by the value of progress, 0 will return the left value, 255 will return the right value, 128 will return the value between them.

NOTE:  This is not an accurate "visible light" color blend but is fast and in most cases good enough.

### NeoPixelBus object
This represents a single NeoPixel Bus that is connected by a single pin.  Please see Adafruit's documentation for details, but the differences are documented below.

#### NeoPixelBus(uint16_t n, uint8_t p = 6, uint8_t t = NEO_GRB | NEO_KHZ800);
instantiates a NewoPixelBus object, with n number of pixels on the bus, over the p pin, using the defined NeoPixel type.
There are some NeoPixels that address the color values differently, so if you set the green color but it displays as red, use the NEO_RGB type flag.

```
NeoPixelBus strip = NeoPixelBus(4, 8, NEO_RGB | NEO_KHZ800);
```
It is rare, but some older NeoPixels require a slower communications speed, to include this support you must include the following define before the NeoPixelBus library include and then include the NEO_KHZ400 type flag to enable this slower speed.

```
#define INCLUDE_NEO_KHZ400_SUPPORT 
#include <NeoPixelBus.h>

NeoPixelBus strip = NeoPixelBus(4, 8, NEO_RGB | NEO_KHZ400);
```

#### void SetPixelColor(uint16_t n, RgbColor c)
This allows setting a pixel on the bus to a color as defined by a color object.	If an animation is actively running on a pixel, it will be stopped.

#### RgbColor GetPixelColor(uint16_t n) const
this allows retrieving the current pixel color

#### void LinearFadePixelColor(uint16_t time, uint16_t n, RgbColor color)
this will setup an animation for a pixel to linear fade between the current color and the given color over the time given.  The time is in milliseconds.

#### void StartAnimating()
this method will initialize the animation state.  This should be called only if there are no active animations and new animations are started.  

#### void UpdateAnimations()
this method will allow the animations to be processed and update the pixel color state. 

NOTE:  Show must still be called to push the color state to the physical NeoPixels.

#### bool IsAnimating() const
this method will return the current animation state.  It will return false if there are no active animations.
