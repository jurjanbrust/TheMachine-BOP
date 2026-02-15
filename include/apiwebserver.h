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
        _server.on("/setled",         HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->setLed(pRequest); });
        _server.on("/setbrightness",         HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->setBrightness(pRequest); });
        _server.on("/jackpot",        HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->triggerJackpot(pRequest); });
        _server.on("/awakening",      HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->triggerAwakening(pRequest); });
        _server.on("/stop",           HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->stopAll(pRequest); });
        _server.on("/resume",         HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->resumeAll(pRequest); });
        _server.on("/sweep",          HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->sweep(pRequest); });
        _server.on("/radialpulse",    HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->radialPulse(pRequest); });
        _server.on("/plasma",          HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->plasma(pRequest); });
        _server.on("/rain",            HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->rain(pRequest); });
        _server.on("/breathinggrid",   HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->breathingGrid(pRequest); });
        _server.on("/spotlightcone",   HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->spotlightCone(pRequest); });
        _server.on("/spatialmeteor",   HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->spatialMeteor(pRequest); });

        _server.begin();
        debugI("HTTP server started");
    }

    void setLed(AsyncWebServerRequest * pRequest)
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

    void setBrightness(AsyncWebServerRequest * pRequest)
    {
        const char * pszEffectIndex = "value";
        if (pRequest->hasParam(pszEffectIndex, false, false))
        {
          debugI("processRequest: param found");
          AsyncWebParameter * p = pRequest->getParam(pszEffectIndex, false, false);
          size_t value = strtoul(p->value().c_str(), NULL, 10); 
          debugI("value = %d", value);
          uint8_t brightness = static_cast<uint8_t>(constrain(value, 0, 255));
          FastLED.setBrightness(brightness);
          SaveBrightness(brightness);
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

    void triggerJackpot(AsyncWebServerRequest * pRequest)
    {
        debugI("Jackpot celebration triggered via API");
        TriggerJackpotCelebration();
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void triggerAwakening(AsyncWebServerRequest * pRequest)
    {
        debugI("Awakening mode triggered via API");
        TriggerAwakening();
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void stopAll(AsyncWebServerRequest * pRequest)
    {
        debugI("Stop all modes triggered via API");
        SetAllStopped(true);
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void resumeAll(AsyncWebServerRequest * pRequest)
    {
        debugI("Resume all modes triggered via API");
        SetAllStopped(false);
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void sweep(AsyncWebServerRequest * pRequest)
    {
        // dir: 0=L→R, 1=R→L, 2=T→B, 3=B→T, 4=outer→inner, 5=inner→outer
        uint8_t dir = 0;
        if (pRequest->hasParam("dir", false, false))
        {
            AsyncWebParameter * p = pRequest->getParam("dir", false, false);
            dir = static_cast<uint8_t>(strtoul(p->value().c_str(), NULL, 10));
        }
        debugI("Sweep triggered via API: dir=%u", dir);
        RunSweep(dir);
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void radialPulse(AsyncWebServerRequest * pRequest)
    {
        debugI("Radial pulse triggered via API");
        RunRadialPulse();
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void plasma(AsyncWebServerRequest * pRequest)
    {
        debugI("Plasma triggered via API");
        RunPlasma();
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void rain(AsyncWebServerRequest * pRequest)
    {
        debugI("Rain triggered via API");
        RunRain();
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void breathingGrid(AsyncWebServerRequest * pRequest)
    {
        debugI("Breathing grid triggered via API");
        RunBreathingGrid();
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void spotlightCone(AsyncWebServerRequest * pRequest)
    {
        debugI("Spotlight cone triggered via API");
        RunSpotlightCone();
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void spatialMeteor(AsyncWebServerRequest * pRequest)
    {
        debugI("Spatial meteor triggered via API");
        RunSpatialMeteor();
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

};