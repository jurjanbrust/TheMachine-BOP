#include "globals.h"
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.  

extern uint32_t           g_FPS;
extern bool               g_bUpdateStarted;

void DrawWalkingDot()
{
    // move up
    for(int whiteLed = 0; whiteLed < NUM_LEDS0; whiteLed+=6) {
        for(int i =0; i<6; i++) 
            leds0[whiteLed+i] = CRGB::Red;

        FastLED.show();
        delay(300);

        for(int i =0; i<6; i++) 
            leds0[whiteLed+i] = CRGB::Black;
        FastLED.show();
    }

    // Move down
    for(int whiteLed = NUM_LEDS0; whiteLed > 0; whiteLed-=6) {
        for(int i =0; i<6; i++) 
            leds0[whiteLed-i] = CRGB::Red;

        FastLED.show();
        delay(300);

        for(int i =0; i<6; i++) 
            leds0[whiteLed-i] = CRGB::Black;
        FastLED.show();
    }
}

void ColorFillEffect(CRGB color = CRGB(246,200,160), int nrOfLeds = 10, int everyNth = 10)
{
		for (int i = 0; i < nrOfLeds; i+= everyNth) {
			leds1[i] = color;
        }

        FastLED.show();
}

// DrawLoopTaskEntry
// 
// Starting point for the draw code loop
void IRAM_ATTR DrawLoopTaskEntry(void *)
{
   
    debugI(">> DrawLoopTaskEntry\n");

    for (;;)
    {
        // Loop through each of the channels and see if they have a current frame that needs to be drawn
        if (WiFi.isConnected())
        {
            ColorFillEffect(CRGB::White, NUM_LEDS1, 1);
            DrawWalkingDot();
            delay(1000*6);
            // ColorFillEffect(CRGB::Green, NUM_LEDS1, 1);
            // delay(1000*60);
            // ColorFillEffect(CRGB::Blue, NUM_LEDS1, 1);
            // delay(1000*60);
            // ColorFillEffect(CRGB::White, NUM_LEDS1, 1);
            // delay(1000*60);
            // ColorFillEffect(CRGB::Yellow, NUM_LEDS1, 1);
        }
        else
        {
            debugV("Not connected to WiFi so not checking for WiFi draw.");
        }

        // Once an OTA flash update has started, we don't want to hog the CPU or it goes quite slowly, 
        // so we'll pause to share the CPU a bit once the update has begun
        if (g_bUpdateStarted)
            delay(100);
        
        // If we didn't draw anything, we near-busy-wait so that we are continually checking the clock for an packet
        // whose time has come
        delay(5);
    }
}

