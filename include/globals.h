#pragma once

#define ENABLE_OTA 1
#define ENABLE_WIFI 1
#define INCOMING_WIFI_ENABLED 0
#define ENABLE_WEBSERVER 0
#define STACK_SIZE (ESP_TASK_MAIN_STACK) // Stack size for each new thread

#define NUM_CHANNELS 2

#define LED_PIN0 14 // been
#define NUM_LEDS0 8*6 + 4 + 1 // been + ogen + hart

#define LED_PIN1 12 // overig
#define NUM_LEDS1 121 // overig

//#define NUM_LEDS 505 // = ledstrip plafond

// Thread priorities
//
// We have a half-dozen workers and these are their relative priorities.  It might survive if all were set equal,
// but I think drawing should be lower than audio so that a bad or greedy effect doesn't starve the audio system.
#define DRAWING_PRIORITY        tskIDLE_PRIORITY+3      // Draw any available frames first
#define SOCKET_PRIORITY         tskIDLE_PRIORITY+4      // ...then process and decompress incoming frames
#define AUDIO_PRIORITY          tskIDLE_PRIORITY+2
#define SCREEN_PRIORITY         tskIDLE_PRIORITY+2
#define NET_PRIORITY            tskIDLE_PRIORITY+2
#define DEBUG_PRIORITY          tskIDLE_PRIORITY+1
#define REMOTE_PRIORITY         tskIDLE_PRIORITY+1

#define DRAWING_CORE            1
#define NET_CORE                0
#define AUDIO_CORE              1
#define SCREEN_CORE             1       
#define DEBUG_CORE              1
#define SOCKET_CORE             0
#define REMOTE_CORE             1


#include <FastLED.h>                // FastLED for the LED panels
#include "RemoteDebug.h"

extern RemoteDebug Debug;           // Let everyone in the project know about it.  If you don't have it, delete this
extern CRGB leds0[];    // been
extern CRGB leds1[];    // overig