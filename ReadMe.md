# NeoPixelBus

[![Donate](http://img.shields.io/paypal/donate.png?color=yellow)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=6AA97KE54UJR4)

Arduino NeoPixel library

Please read this best practices link before connecting your NeoPixels, it will save you alot of time and effort. [AdaFruits NeoPixel Best Practices](https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices)

This version currently only supports AVR and Esp8266.  Other platforms can be supported by creating an issue on GitHub/Makuna/NeoPixelBus.  

NOW SUPPORTS RGBW! 
SK6812, WS2811, and WS2812 are usable with this library.

The new library supports a templatized model of defining which method gets used to send data and what order and size the pixel data is sent in.  This new design creates the smallest code for each definition of features used.  Please see examples to become familiar with the new design.

Using the StandTest example from AdaFruit_NeoPixel library targeting a Arduino Mega 2560 as a comparison, you can see the compilation results below.

AdaFruit_NeoPixel library
```
Sketch uses 4,236 bytes (1%) of program storage space. Maximum is 253,952 bytes.
Global variables use 40 bytes (0%) of dynamic memory, leaving 8,152 bytes for local variables. Maximum is 8,192 bytes.
```

NeoPixelBus library
```
Sketch uses 3,570 bytes (1%) of program storage space. Maximum is 253,952 bytes.
Global variables use 34 bytes (0%) of dynamic memory, leaving 8,158 bytes for local variables. Maximum is 8,192 bytes.
```
## Requirements

Due to the use of hardware or software features of each method, there are implications that you need to understand.  Please review the methods for more understanding of the tradeoffs.

## Installing This Library
Open the Library Manager and search for "NeoPixelBus by Makuna" and install

## Installing This Library From GitHub
Create a directory in your Arduino\Library folder named "NeoPixelBus"
Clone (Git) this project into that folder.  
It should now show up in the import list when you restart Arduino IDE.

## Samples
### NeoPixelTest
This is a good place to start and make sure your pixels work.
This is simple example that sets four pixels to red, green, blue, and white in order; and then flashes off and then on again.  If the first pixel is green and the second is red, you need to use the NeoRgbFeature with NeoPixelBus constructor.
### NeoPixelFunLoop
This example will move a trail of light around a series of pixels with a fading tail. A ring formation of pixels looks best.
### NeoPixelFunRandomChange
This example will randomly select a number pixels and then start an animation to blend them from their current color to randomly selected a color.
### NeoPixelFunFadeInOut
This example will randomly pick a color and fade all pixels to that color, then it will fade them to black and restart over.
### NeoPixelAnimation
This is a more complex example that demonstrates platform specific animation callbacks and the timescale support for long duration animations.

## Documentation
[See Wiki](https://github.com/Makuna/NeoPixelBus/wiki)



