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
        _server.on("/vortex",          HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->quantumVortex(pRequest); });
        _server.on("/lightning",       HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->lightningStorm(pRequest); });
        _server.on("/neonrings",       HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->neonRings(pRequest); });
        _server.on("/artworkstory",    HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->artworkStory(pRequest); });
        _server.on("/fireworks",       HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->fireworks(pRequest); });
        _server.on("/lasergrid",       HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->laserMatrix(pRequest); });
        _server.on("/ghostbride",      HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->ghostBride(pRequest); });
        _server.on("/multiball",       HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->multiball(pRequest); });
        _server.on("/eclipse",         HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->solarEclipse(pRequest); });
        _server.on("/prismshatter",    HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->prismShatter(pRequest); });
        _server.on("/crimsontakeover", HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->crimsonTakeover(pRequest); });
        _server.on("/opening-showcase", HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->openingShowcase(pRequest); });
        _server.on("/opening-cosmic",   HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->cosmicOpening(pRequest); });
        _server.on("/opening-bride",    HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->brideOpening(pRequest); });
        _server.on("/opening-launch",   HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->launchOpening(pRequest); });
        _server.on("/opening-city",     HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->cityOpening(pRequest); });
        _server.on("/opening-diagnostics", HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->diagnosticsOpening(pRequest); });
        _server.on("/opening-transmission", HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->transmissionOpening(pRequest); });
        _server.on("/opening-pulse",    HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->pulseOpening(pRequest); });
        _server.on("/opening-moonlight", HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->moonlightOpening(pRequest); });

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
          if (index < NUM_LEDS1)
          {
              leds1[index] = CRGB::White;
              FastLED.show();
          }
          else
          {
              debugW("setLed: index %u out of range (NUM_LEDS1=%u)", index, NUM_LEDS1);
          }
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

    void quantumVortex(AsyncWebServerRequest * pRequest)
    {
        debugI("Quantum vortex triggered via API");
        RunQuantumVortex();
        sendOk(pRequest);
    }

    void lightningStorm(AsyncWebServerRequest * pRequest)
    {
        debugI("Lightning storm triggered via API");
        RunLightningStorm();
        sendOk(pRequest);
    }

    void neonRings(AsyncWebServerRequest * pRequest)
    {
        debugI("Neon rings triggered via API");
        RunNeonRings();
        sendOk(pRequest);
    }

    void artworkStory(AsyncWebServerRequest * pRequest)
    {
        debugI("Artwork story triggered via API");
        RunArtworkStory();
        sendOk(pRequest);
    }

    void fireworks(AsyncWebServerRequest * pRequest)
    {
        debugI("Fireworks triggered via API");
        RunFireworks();
        sendOk(pRequest);
    }

    void laserMatrix(AsyncWebServerRequest * pRequest)
    {
        debugI("Laser matrix triggered via API");
        RunLaserMatrix();
        sendOk(pRequest);
    }

    void ghostBride(AsyncWebServerRequest * pRequest)
    {
        debugI("Ghost bride triggered via API");
        RunGhostBride();
        sendOk(pRequest);
    }

    void multiball(AsyncWebServerRequest * pRequest)
    {
        debugI("Multiball triggered via API");
        RunMultiball();
        sendOk(pRequest);
    }

    void solarEclipse(AsyncWebServerRequest * pRequest)
    {
        debugI("Solar eclipse triggered via API");
        RunSolarEclipse();
        sendOk(pRequest);
    }

    void prismShatter(AsyncWebServerRequest * pRequest)
    {
        debugI("Prism shatter triggered via API");
        RunPrismShatter();
        sendOk(pRequest);
    }

    void crimsonTakeover(AsyncWebServerRequest * pRequest)
    {
        RunCrimsonTakeover();
        sendOk(pRequest);
    }

    void openingShowcase(AsyncWebServerRequest * pRequest)
    {
        RunOpeningShowcase();
        sendOk(pRequest);
    }

    void cosmicOpening(AsyncWebServerRequest * pRequest)
    {
        RunCosmicOpening();
        sendOk(pRequest);
    }

    void brideOpening(AsyncWebServerRequest * pRequest)
    {
        RunBrideAssemblyOpening();
        sendOk(pRequest);
    }

    void launchOpening(AsyncWebServerRequest * pRequest)
    {
        RunLaunchControlOpening();
        sendOk(pRequest);
    }

    void cityOpening(AsyncWebServerRequest * pRequest)
    {
        RunCityAwakeningOpening();
        sendOk(pRequest);
    }

    void diagnosticsOpening(AsyncWebServerRequest * pRequest)
    {
        RunDiagnosticsOpening();
        sendOk(pRequest);
    }

    void transmissionOpening(AsyncWebServerRequest * pRequest)
    {
        RunStellarTransmissionOpening();
        sendOk(pRequest);
    }

    void pulseOpening(AsyncWebServerRequest * pRequest)
    {
        RunPulseOfLifeOpening();
        sendOk(pRequest);
    }

    void moonlightOpening(AsyncWebServerRequest * pRequest)
    {
        RunMoonlightRevealOpening();
        sendOk(pRequest);
    }

  private:
    void sendOk(AsyncWebServerRequest * pRequest)
    {
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

};