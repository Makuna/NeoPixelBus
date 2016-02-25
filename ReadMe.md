# NeoPixelBus

[![Donate](http://img.shields.io/paypal/donate.png?color=yellow)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=6AA97KE54UJR4)

Arduino NeoPixel library

A library to control one wire protocol RGB leds like SK6812, WS2811, and WS2812 that are commonly refered to as NeoPixels.

Please read this best practices link before connecting your NeoPixels, it will save you alot of time and effort.  
[AdaFruits NeoPixel Best Practices](https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices)

For quick questions jump on Gitter and ask away.  
[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Makuna/NeoPixelBus?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

For bugs, make sure there isn't an active issue and then create one.

This new library supports a templatized model of defining which method gets used to send data and what order and size the pixel data is sent in.  This new design creates the smallest code for each definition of features used.  Please see examples to become familiar with the new design.

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

## Installing This Library (prefered)
Open the Library Manager and search for "NeoPixelBus by Makuna" and install

## Installing This Library From GitHub
Create a directory in your Arduino\Library folder named "NeoPixelBus"
Clone (Git) this project into that folder.  
It should now show up in the import list when you restart Arduino IDE.

## Documentation
[See Wiki](https://github.com/Makuna/NeoPixelBus/wiki)



