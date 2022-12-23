#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi
#include "globals.h"
#include "drawing.h"

using namespace fs;

class ApiWebServer 
{
  private:

    AsyncWebServer _server;

  public:

    ApiWebServer()
        : _server(80)
    {
    }

    void begin()
    {
        _server.on("/set",         HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->processRequest(pRequest); });
        _server.begin();

        debugI("HTTP server started");
    }

    void processRequest(AsyncWebServerRequest * pRequest)
    {
        ColorFillEffect(CRGB::Black, NUM_LEDS1, 1);

        const char * pszEffectIndex = "index";
        if (pRequest->hasParam(pszEffectIndex, false, false))
        {
          debugI("processRequest: param found");
          AsyncWebParameter * p = pRequest->getParam(pszEffectIndex, false, false);
          size_t index = strtoul(p->value().c_str(), NULL, 10); 
          debugI("index = %d", index);
          leds1[index] = CRGB::White;
          FastLED.show();
        } 
        else 
        {
            debugI("processRequest: param not found");
        }
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);      
    }
};