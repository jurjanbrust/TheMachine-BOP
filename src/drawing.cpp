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
        for(int i =0; i<6; i++) 
            leds0[current+i] = CRGB::Red;

        FastLED.show();
        delay(300);

        for(int i =0; i<6; i++) 
            leds0[current+i] = CRGB::Black;
        FastLED.show();
    }

    // Move down
    for(int current = NUM_LEDS0-6; current > 0; current-=6) {
        for(int i =0; i<6; i++) 
            leds0[current-i] = CRGB::Red;

        FastLED.show();
        delay(300);

        for(int i =0; i<6; i++) 
            leds0[current-i] = CRGB::Black;
        FastLED.show();
    }
}

void TheMachineLogo(CRGB color = CRGB(246,200,160))
{
        int start = 8;
        for (int i = start; i < start+12; i++) {
			leds1[i] = color;
        }
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

void Eyes()
{
    leds0[NUM_LEDS0 -2] = CRGB::Yellow;   // oog links
    leds0[NUM_LEDS0 -3] = CRGB::Yellow;   // oog 2e links
    leds0[NUM_LEDS0 -4] = CRGB::Yellow;   // oog 2e rechts
    leds0[NUM_LEDS0 -5] = CRGB::Yellow;   // oog rechts
    FastLED.show();
}

// Starting point for the draw code loop
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

// hart
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
        DrawWalkingDot();
        delay(1000*1);
    }
}
