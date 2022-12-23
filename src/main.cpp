#include "globals.h"
#include <Arduino.h>
#include <ArduinoOTA.h>                         // For updating the flash over WiFi
#include "network.h"                            // For WiFi credentials
#include "drawing.h"

//
// Task Handles to our running threads
//
TaskHandle_t g_taskScreen = nullptr;
TaskHandle_t g_taskSync   = nullptr;
TaskHandle_t g_taskWeb    = nullptr;
TaskHandle_t g_taskDraw   = nullptr;
TaskHandle_t g_taskDebug  = nullptr;
TaskHandle_t g_taskAudio  = nullptr;
TaskHandle_t g_taskNet    = nullptr;
TaskHandle_t g_taskRemote = nullptr;
TaskHandle_t g_taskSocket = nullptr;

//
// Global Variables
//
DRAM_ATTR bool g_bUpdateStarted = false;            // Has an OTA update started?
DRAM_ATTR RemoteDebug Debug;                        // Instance of our telnet debug server
CRGB leds0[NUM_LEDS0];  // been
CRGB leds1[NUM_LEDS1];  // overig

// DebugLoopTaskEntry
//
// Entry point for the Debug task, pumps the Debug handler
void IRAM_ATTR DebugLoopTaskEntry(void *)
{    
    debugI(">> DebugLoopTaskEntry\n");
    debugV("Starting RemoteDebug server...\n");

    Debug.setResetCmdEnabled(true);                         // Enable the reset command
    Debug.showProfiler(false);                              // Profiler (Good to measure times, to optimize codes)
    Debug.showColors(false);                                // Colors
    Debug.setCallBackProjectCmds(&processRemoteDebugCmd);   // Func called to handle any debug externsions we add

    while (!WiFi.isConnected())                             // Wait for wifi, no point otherwise
        delay(100);

    Debug.begin(cszHostname, RemoteDebug::INFO);            // Initialize the WiFi debug server

    for (;;)                                                // Call Debug.handle() 20 times a second
    {
        #if ENABLE_WIFI
            EVERY_N_MILLIS(50)
            {
                Debug.handle();
            }
        #endif
        delay(10);        
    }    
}


void setup() {

    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_WARN);        // set all components to ERROR level  

    if (WiFi.isConnected() == false && ConnectToWiFi(10) == false)
    {
        Serial.printf("Not connected");
    }

    // Re-route debug output to the serial port
    Debug.setSerialEnabled(true);

    debugI("Starting DebugLoopTaskEntry");
    xTaskCreatePinnedToCore(DebugLoopTaskEntry, "Debug Loop", STACK_SIZE, nullptr, DEBUG_PRIORITY, &g_taskDebug, DEBUG_CORE);

    debugI("Adding %d LEDs to FastLED.", NUM_LEDS0);
    FastLED.addLeds<WS2812B, LED_PIN0, GRB>(leds0, NUM_LEDS0);  // been

    debugI("Adding %d LEDs to FastLED.", NUM_LEDS0);
    FastLED.addLeds<WS2812B, LED_PIN1, GRB>(leds1, NUM_LEDS1);  // overig
    FastLED.setBrightness(80);
    ColorFillEffect(CRGB::White, NUM_LEDS1, 1); // inital set to white
    TheMachineLogo(CRGB::OrangeRed);

    xTaskCreatePinnedToCore(DrawLoopTaskEntryOne, "Eyes", STACK_SIZE, nullptr, DRAWING_PRIORITY, &g_taskDraw, DRAWING_CORE);
    xTaskCreatePinnedToCore(DrawLoopTaskEntryTwo, "Heart", STACK_SIZE, nullptr, DRAWING_PRIORITY, &g_taskDraw, DRAWING_CORE);
    xTaskCreatePinnedToCore(DrawLoopTaskEntryThree, "Jackpot", STACK_SIZE, nullptr, DRAWING_PRIORITY, &g_taskDraw, DRAWING_CORE);
}

void loop() {
    while(true)
    {
        #if ENABLE_OTA
          if (WiFi.isConnected()) {    
              ArduinoOTA.handle();
          }
        #endif 

        EVERY_N_SECONDS(5)
        {
            debugI("IP: %s, Mem: %u LargestBlk: %u PSRAM Free: %u/%u LED FPS: %d",
                   WiFi.localIP().toString().c_str(),
                   ESP.getFreeHeap(),
                   ESP.getMaxAllocHeap(),
                   ESP.getFreePsram(), ESP.getPsramSize(),
                   FastLED.getFPS());
        }

        delay(10);        
    }
}

