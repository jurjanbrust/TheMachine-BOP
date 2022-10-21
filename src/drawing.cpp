#include "globals.h"
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.  

extern uint32_t           g_FPS;
extern bool               g_bUpdateStarted;

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
            // Move a single white led 
            for(int whiteLed = 0; whiteLed < NUM_LEDS; whiteLed++) {
                // Turn our current led on to white, then show the leds
                leds[whiteLed] = CRGB::White;

                // Show the leds (only one of which is set to white, from above)
                FastLED.show();

                // Wait a little bit
                delay(100);
                Serial.printf("Led %d", whiteLed);
                debugI("Led: %d", whiteLed);
                // Turn our current led back to black for the next loop around
                leds[whiteLed] = CRGB::Black;
            }

            // Move a single white led 
            for(int whiteLed = NUM_LEDS; whiteLed > 0; whiteLed--) {
                // Turn our current led on to white, then show the leds
                leds[whiteLed] = CRGB::White;

                // Show the leds (only one of which is set to white, from above)
                FastLED.show();

                // Wait a little bit
                delay(100);
                Serial.printf("Led %d", whiteLed);
                debugI("Led: %d", whiteLed);
                // Turn our current led back to black for the next loop around
                leds[whiteLed] = CRGB::Black;
            }
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