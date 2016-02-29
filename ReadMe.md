# DEPRICATED
This branch is no longer active.  Please use the Master branch as it has support for all platforms.  
This branch is being left around long enough for users to migrate away from it.

# NeoPixelBus

[![Donate](http://img.shields.io/paypal/donate.png?color=yellow)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=6AA97KE54UJR4)

Arduino NeoPixel library

A library to control one wire protocol RGB and RGBW leds like SK6812, WS2811, and WS2812 that are commonly refered to as NeoPixels.  
Supports most Arduino platforms.  
This is the most funtional library for the Esp8266 as it provides solutions for all Esp8266 module types even when WiFi is used.

Please read this best practices link before connecting your NeoPixels, it will save you alot of time and effort.  
[Adafruit NeoPixel Best Practices](https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices)

For quick questions jump on Gitter and ask away.  
[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Makuna/NeoPixelBus?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

For bugs, make sure there isn't an active issue and then create one.

This new library supports a templatized model of defining which method gets used to send data and what order and size the pixel data is sent in.  This new design creates the smallest code for each definition of features used.  Further it allows for picking which method to send the data on the Esp8266 in an easy to change way.  
Please see examples to become familiar with the new design.  
Due to this design you will often realize over 500 bytes of more program storage for your sketch.  Important for the smallest Arduinos project.  


## Installing This Library (prefered)
Open the Library Manager and search for "NeoPixelBus by Makuna" and install

## Installing This Library From GitHub
Create a directory in your Arduino\Library folder named "NeoPixelBus"
Clone (Git) this project into that folder.  
It should now show up in the import list when you restart Arduino IDE.

## Documentation
[See Wiki](https://github.com/Makuna/NeoPixelBus/wiki)



