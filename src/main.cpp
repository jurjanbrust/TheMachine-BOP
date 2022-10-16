#include "globals.h"
#include <Arduino.h>
#include <ArduinoOTA.h>                         // For updating the flash over WiFi
#include "network.h"                            // For WiFi credentials

//
// Global Variables
//
DRAM_ATTR bool g_bUpdateStarted = false;            // Has an OTA update started?


void setup() {

  Serial.begin(115200);
  Serial.printf("Starting");
  if (WiFi.isConnected() == false && ConnectToWiFi(10) == false)
  {
      Serial.printf("Not connected");
  }
  Serial.println(WiFi.localIP());


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