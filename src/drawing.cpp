#include "globals.h"
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.  

extern uint32_t           g_FPS;
extern bool               g_bUpdateStarted;

void PostDrawHandler()
{
    // Once an OTA flash update has started, we don't want to hog the CPU or it goes quite slowly, 
    // so we'll pause to share the CPU a bit once the update has begun
    if (g_bUpdateStarted)
        delay(1000);
    
    // If we didn't draw anything, we near-busy-wait so that we are continually checking the clock for an packet
    // whose time has come
    delay(5);
}

// used for pinbot jackpot
void DrawWalkingDot()
{
    // move up
    for(int current = 0; current < NUM_LEDS0-6; current+=6) {
        for(int i=0; i<6; i++) 
            leds0[current+i] = CRGB::Red;

        FastLED.show();
        delay(300);

        for(int i=0; i<6; i++) 
            leds0[current+i] = CRGB::Black;

        FastLED.show();
    }

    // Move down
    for(int current = NUM_LEDS0-6; current > 0; current-=6) {
        for(int i=0; i<6; i++) 
            leds0[current-i] = CRGB::Red;

        FastLED.show();
        delay(300);

        for(int i=0; i<6; i++) 
            leds0[current-i] = CRGB::Black;

        FastLED.show();
    }
}

void JackPotDefaultColors()
{
    // jackpot = 8 vakjes met 6 leds per stuk
    // eerste 4 = geel
    // laatste 4 = rood
    for(int vakje = 0; vakje < 4; vakje++) {
        for(int ledje = 0; ledje < 6; ledje++) {
            leds0[vakje*6+ledje] = CRGB::DarkOrange;
        }
    }

    for(int vakje = 4; vakje < 8; vakje++) {
        for(int ledje = 0; ledje < 6; ledje++) {
            leds0[vakje*6+ledje] = CRGB::Red;
        }
    }
    FastLED.show();
}

void TheMachineLogo(CRGB color = CRGB(246,200,160))
{
        int start = 8;
        for (int i = start; i < start+12; i++) {
			leds1[i] = color;
        }
        FastLED.show();
}

void TheBride(CRGB color = CRGB(246,200,160))
{
        uint leds[] = {3,5,6,62,63,64,65,66,67,68,69,79,88,94,95,96,97,98,103,104,105,106,107,108,109,110,111,112,113,115,116,117,118};

        for (uint i = 0; i < 33 ; i++) {
			leds1[leds[i]] = color;
        }
        FastLED.show();
}

void SingleLed(int index, CRGB color = CRGB(246,200,160))
{
    leds1[index] = color;
    FastLED.show();
}

void ColorFillEffect(CRGB color = CRGB(246,200,160), int nrOfLeds = 10, int everyNth = 10)
{
		for (int i = 0; i < nrOfLeds; i+= everyNth) {
			leds1[i] = color;
        }

        FastLED.show();
}

void Heartbeat(int channel)
{
  const uint8_t hbTable[] = {
    25,
    61,
    105,
    153,
    197,
    233,
    253,
    255,
    252,
    243,
    230,
    213,
    194,
    149,
    101,
    105,
    153,
    197,
    216,
    233,
    244,
    253,
    255,
    255,
    252,
    249,
    243,
    237,
    230,
    223,
    213,
    206,
    194,
    184,
    174,
    162,
    149,
    138,
    126,
    112,
    101,
    91,
    78,
    69,
    62,
    58,
    51,
    47,
    43,
    39,
    37,
    35,
    29,
    25,
    22,
    20,
    19,
    15,
    12,
    9,
    8,
    6,
    5,
    3
  };

#define NUM_STEPS (sizeof(hbTable)/sizeof(uint8_t)) //array size
  //fill_solid(leds0, NUM_LEDS0, CRGB::Red);
  // beat8 generates index 0-255 (fract8) as per getBPM(). lerp8by8 interpolates that to array index:
  uint8_t hbIndex = lerp8by8( 0, NUM_STEPS, beat8( 35 )) ;
  uint8_t brightness = lerp8by8( 0, 255, hbTable[hbIndex] ) ;
  if(channel == 0) {
    leds0[NUM_LEDS0 -1] = CRGB::Red;
    leds0[NUM_LEDS0 -1].fadeLightBy(brightness);
  } else if (channel == 1) {
    leds0[NUM_LEDS0 -2] = CRGB::BlueViolet;
    leds0[NUM_LEDS0 -2].fadeLightBy(brightness);
    leds0[NUM_LEDS0 -3] = CRGB::BlueViolet;
    leds0[NUM_LEDS0 -3].fadeLightBy(brightness);
    leds0[NUM_LEDS0 -4] = CRGB::BlueViolet;
    leds0[NUM_LEDS0 -4].fadeLightBy(brightness);
    leds0[NUM_LEDS0 -5] = CRGB::BlueViolet;
    leds0[NUM_LEDS0 -5].fadeLightBy(brightness);
  }
  FastLED.show();
  //FastLED.setBrightness( lerp8by8( 0, 255, brightness ) ); // interpolate to max MAX_BRIGHTNESS
}

void Eyes(CRGB color = CRGB(246,200,160))
{
    leds0[NUM_LEDS0 -2] = color;   // oog links
    leds0[NUM_LEDS0 -3] = color;   // oog 2e links
    leds0[NUM_LEDS0 -4] = color;   // oog 2e rechts
    leds0[NUM_LEDS0 -5] = color;   // oog rechts

    FastLED.show();
}

// shuttle flames
void IRAM_ATTR DrawLoopTaskEntryOne(void *)
{
    int start = 55;
    int end = start + 2;
    int delayTime = 70;
    for (;;)
    {
        for(int i = start; i<=end; i++) {
            leds1[i] = CRGB::Red;
        }
        FastLED.show(); delay(delayTime);

         for(int i = start; i<=end; i++) {
            leds1[i] = CRGB::Brown;
        }
        FastLED.show(); delay(delayTime);

        PostDrawHandler();
    }
}

// heart
void IRAM_ATTR DrawLoopTaskEntryTwo(void *) 
{
    for (;;)
    {
        Heartbeat(0);
        PostDrawHandler();
    }
}

// jackpot (been)
void IRAM_ATTR DrawLoopTaskEntryThree(void *)
{
    for (;;)
    {
        for(int i=0; i<5; i++) {
            DrawWalkingDot();
            delay(1000*1);
        }
        JackPotDefaultColors();
        delay(1000*30);
        PostDrawHandler();
    }
}

// the machine logo
void IRAM_ATTR DrawLoopTaskEntryFour(void *)
{
    uint8_t deltahue = 10;
    for (;;)
    {
        uint8_t thisHue = beat8(10,255);                     // A simple rainbow march.
        CHSV hsv;
        hsv.hue = thisHue;
        hsv.val = 255;
        hsv.sat = 240;
        for( int i = 8; i < 20; ++i) {
            leds1[i] = hsv;
            hsv.hue += deltahue;
        }
            PostDrawHandler();
    }
}
