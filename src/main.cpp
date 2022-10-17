#include "globals.h"
#include <Arduino.h>
#include <ArduinoOTA.h>                         // For updating the flash over WiFi
#include "network.h"                            // For WiFi credentials


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
  if (WiFi.isConnected() == false && ConnectToWiFi(10) == false)
  {
      Serial.printf("Not connected");
  }

  debugI("Starting DebugLoopTaskEntry");
  xTaskCreatePinnedToCore(DebugLoopTaskEntry, "Debug Loop", STACK_SIZE, nullptr, DEBUG_PRIORITY, &g_taskDebug, DEBUG_CORE);
}

void loop() {
    while(true)
    {
        #if ENABLE_OTA
          if (WiFi.isConnected()) {    
              ArduinoOTA.handle();
          }
        #endif 

        delay(10);        
    }
}

