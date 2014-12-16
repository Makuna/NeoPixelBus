NeoPixelBus
====
Arduino NeoPixel library

Clone this into your Arduino\Library folder

This library is a modification of the Adafruit NeoPixel library.
The Api is similiar, but it removes the overal brightness feature and adds animation support.

Installing This Library
------------------------
Create a directory in your Arduino\Library folder named "NeoPixelBus"
Clone (Git) this project into that folder.  
It should now show up in the import list.

Samples
-------
NeoPixelTest - this is simple example that sets four neopixels to red, green, blue, and then white in order; and then flashes them.  If the first pixel is not green instead of read, you need to pass the NEO_RGB flag into the NeoPixelBus constructor.
NeoPixelFun - this is a more complex example, that includes code for three effects, and demonstrates animations.

API Documentation
-----------------

RgbColor object:
This represents a color and exposes useful methods to manipulate colors.
<pre><code>
RgbColor(uint8_t r, uint8_t g, uint8_t b)
</code></pre>
	instantiates a RgbColor object with the given r, g, b values.
<pre><code>
RgbColor(uint8_t brightness)
</code></pre>
	instantiates a RgbColor object with the given brightness. 0 is black, 128 is grey, 255 is white.
<pre><code>
uint8_t CalculateBrightness()
</code></pre>
	returns the general brightness of the pixe, averaging color.
<pre><code>
void Darken(uint8_t delta)
</code></pre>
	this will darken the color by the given amount
<pre><code>
void Lighten(uint8_t delta)
</code></pre>
	this will lighten the color by the given amount
<pre><code>
static RgbColor LinearBlend(RgbColor left, RgbColor right, uint8_t progress)
</code></pre>
	this will return a color that is a blend between the given colors.  The amount to blend is given by the value of progress, 0 will return the left value, 255 will return the right value, 128 will return the value between them.
	NOTE:  This is not an accurate "visible light" color blend but is fast and in most cases good enough.

NeoPixelBus object:
This represents a single NeoPixel Bus that is connected by a single pin.  Please see Adafruit's documentation for details, but the differences are documented below.

<pre><code>
NeoPixelBus(uint16_t n, uint8_t p = 6, uint8_t t = NEO_GRB | NEO_KHZ800);
<pre><code>
instantiates a NewoPixelBus object, with n number of pixels on the bus, over the p pin, using the defined NeoPixel type.
There are some NeoPixels that address the color values differently, so if you set the green color but it displays as red, use the NEO_RGB type flag.
<pre><code>
NeoPixelBus strip = NeoPixelBus(4, 8, NEO_GRB | NEO_KHZ800);
</code></pre>
It is rare, but some older NeoPixels require a slower communications speed, to include this support you must include the following define before the NeoPixelBus library include and then include the NEO_KHZ400 type flag to enable this slower speed.
<pre><code>
#define INCLUDE_NEO_KHZ400_SUPPORT 
#include &lt;NeoPixelBus.h&gt;

NeoPixelBus strip = NeoPixelBus(4, 8, NEO_GRB | NEO_KHZ400);
</code></pre>

<pre><code>
void SetPixelColor(uint16_t n, RgbColor c)
</code></pre>
	This allows setting a pixel on the bus to a color as defined by a color object.	If an animation is actively running on a pixel, it will be stopped.
<pre><code>
RgbColor GetPixelColor(uint16_t n) const
</code></pre>
	this allows retrieving the current pixel color
<pre><code>
void LinearFadePixelColor(uint16_t time, uint16_t n, RgbColor color)
</code></pre>
	this will setup an animation for a pixel to linear fade between the current color and the given color over the time given.  The time is in milliseconds.
<pre><code>
void StartAnimating()
</code></pre>
	this method will initialize the animation state.  This should be called only if there are no active animations and new animations are started.  
<pre><code>
void UpdateAnimations()
</code></pre>
	this method will allow the animations to processed and update the pixel color state. 
	NOTE:  Show must still be called to push the color state to the physical NeoPixels.
<pre><code>
bool IsAnimating() const
</code></pre>
	this method will return the current animation state.  It will return false if there are no active animations.