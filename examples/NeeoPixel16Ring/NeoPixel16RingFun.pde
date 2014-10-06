#include <NeoPixelBus.h>

NeoPixelBus strip = NeoPixelBus(16, 8);
uint16_t p = 0;



void setup()
{
  strip.Begin();
  strip.Show();
  randomSeed(analogRead(0));
  Serial.begin(9600);

}


void loop()
{
  Serial.println("next");
  

  
  // LoopAround(192, 200);
  PickRandom(128);
  // FadeInFadeOutRinseRepeat(192);
  
  // start animating
  strip.StartAnimating();
  
  // wait until no more animations are running
  while (strip.IsAnimating())
  {
    strip.UpdateAnimations();
    strip.Show();
    delay(31); // ~30hz change cycle
  }
 
}

void FadeInFadeOutRinseRepeat(uint8_t peak)
{
  if (p == 0)
  {
    for (uint8_t pixel = 0; pixel < 16; pixel++)
    {
      uint16_t time = random(800,1000);
      strip.LinearFadePixelColor(time, pixel, RgbColor(random(peak), random(peak), random(peak)));
    }
  }
  else if (p == 1)
  {
    for (uint8_t pixel = 0; pixel < 16; pixel++)
    {
      uint16_t time = random(600,700);
      strip.LinearFadePixelColor(time, pixel, RgbColor(0, 0, 0));
    }
  }
  p = (p + 1) % 2; // next procedure and keep within the number of procedures
  
}

void PickRandom(uint8_t peak)
{

  // pick random set of pixels to animate
  uint8_t count = random(16);
  while (count > 0)
  {
    uint8_t pixel = random(16);
    
    // configure the animations
    RgbColor color; // = strip.getPixelColor(pixel);

      color = RgbColor(random(peak), random(peak), random(peak));

    
    uint16_t time = random(100,400);
    strip.LinearFadePixelColor( time, pixel, color);
    
    count--;
  }
}

void LoopAround(uint8_t peak, uint16_t speed)
{
  // Looping around the ring sample
  uint16_t prev;
  RgbColor prevColor;
  
  // fade previous one dark
  prev = (p + 11) % 16; 
  strip.LinearFadePixelColor(speed, prev, RgbColor(0, 0, 0));
  
  // fade previous one dark
  prev = (p + 12) % 16; 
  prevColor = strip.GetPixelColor( prev );
  prevColor.Darken(prevColor.CalculateBrightness() / 2);
  strip.LinearFadePixelColor(speed, prev, prevColor);
  
  // fade previous one dark
  prev = (p + 13) % 16; 
  prevColor = strip.GetPixelColor( prev );
  prevColor.Darken(prevColor.CalculateBrightness() / 2);
  strip.LinearFadePixelColor(speed, prev, prevColor);
  
  // fade previous one dark
  prev = (p + 14) % 16; 
  prevColor = strip.GetPixelColor( prev );
  prevColor.Darken(prevColor.CalculateBrightness() / 2);
  strip.LinearFadePixelColor(speed, prev, prevColor);
  
  // fade previous one dark
  prev = (p + 15) % 16; 
  prevColor = strip.GetPixelColor( prev );
  prevColor.Darken(prevColor.CalculateBrightness() / 2);
  strip.LinearFadePixelColor(speed, prev, prevColor);
  
  // fade current one light
  strip.LinearFadePixelColor(speed, p, RgbColor(random(peak), random(peak), random(peak)));
  p = (p+1) % 16;
}

