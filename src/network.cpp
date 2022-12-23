#include "globals.h"
#include "network.h"
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi

// processRemoteDebugCmd
// 
// Callback function that the debug library (which exposes a little console over telnet and serial) calls
// in order to allow us to add custom commands.  I've added a clock reset and stats command, for example.

void processRemoteDebugCmd() 
{
    String str = Debug.getLastCommand();
    if (str.equalsIgnoreCase("stats"))
    {
        debugI("Displaying statistics....");
    }
}

// ConnectToWiFi
//
// Connect to the pre-configured WiFi network.  
//
// BUGBUG I'm guessing this is exposed in all builds so anyone can call it and it just returns false if wifi
// isn't being used, but do we need that?  If no one really needs to call it put the whole thing in the ifdef
bool ConnectToWiFi(uint cRetries)
{
    #if !ENABLE_WIFI
        return false;
    #endif

    for (uint iPass = 0; iPass < cRetries; iPass++)
    {
        Serial.printf("Pass %u of %u: Connecting to Wifi SSID: %s - ESP32 Free Memory: %u, PSRAM:%u, PSRAM Free: %u\n",
            iPass, cRetries, cszSSID, ESP.getFreeHeap(), ESP.getPsramSize(), ESP.getFreePsram());

        //WiFi.disconnect();
        WiFi.begin(cszSSID, cszPassword);

        for (uint i = 0; i < 10; i++)
        {
            if (WiFi.isConnected())
            {
                Serial.printf("Connected to AP with BSSID: %s\n", WiFi.BSSIDstr().c_str());
                break;
            }
            else
            {
                delay(1000);
            }
        }

        if (WiFi.isConnected())
            break;
    }

    if (false == WiFi.isConnected())
    {
        Serial.printf("Giving up on WiFi\n");
        return false;
    }

    #if ENABLE_OTA
        Serial.printf("Publishing OTA...");
        SetupOTA(cszHostname);
    #endif

    Serial.printf("Received IP: %s", WiFi.localIP().toString().c_str());

    return true;
}

// SetupOTA
//
// Set up the over-the-air programming info so that we can be flashed over WiFi

void SetupOTA(const char *pszHostname)
{
    #if !ENABLE_OTA
        return;
    #endif

    ArduinoOTA.setRebootOnSuccess(true);

    if (nullptr == pszHostname)
        ArduinoOTA.setMdnsEnabled(false);
    else
        ArduinoOTA.setHostname(pszHostname);

    ArduinoOTA
        .onStart([]() {
            g_bUpdateStarted = true;

            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.printf("Stopping SPIFFS");
            #if ENABLE_WEBSEVER
            SPIFFS.end();
            #endif
            
            Serial.printf("Stopping IR remote");
            #if ENABLE_REMOTE            
            g_RemoteControl.end();
            #endif        
            
            Serial.printf("Start updating from OTA ");
            Serial.printf("%s", type.c_str());
        })
        .onEnd([]() {
            Serial.printf("\nEnd OTA");
        })
        .onProgress([](unsigned int progress, unsigned int total) 
        {
            static uint last_time = millis();
            if (millis() - last_time > 1000)
            {
                last_time = millis();
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            }
            else
            {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            }
            
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
            {
                Serial.printf("Auth Failed");
            }
            else if (error == OTA_BEGIN_ERROR)
            {
                Serial.printf("Begin Failed");
            }
            else if (error == OTA_CONNECT_ERROR)
            {
                Serial.printf("Connect Failed");
            }
            else if (error == OTA_RECEIVE_ERROR)
            {
                Serial.printf("Receive Failed");
            }
            else if (error == OTA_END_ERROR)
            {
                Serial.printf("End Failed");
            }
        });

    ArduinoOTA.begin();
}


