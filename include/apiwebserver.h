#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi
#include "globals.h"
#include "drawing.h"
#include "ledrenderer.h"
#include "network.h"

using namespace fs;

class ApiWebServer 
{
  private:

    AsyncWebServer _server;

    void sendResponse(AsyncWebServerRequest * request,
                      int status,
                      const char * contentType,
                      const String & content)
    {
        AsyncWebServerResponse * response =
            request->beginResponse(status, contentType, content);
        response->addHeader("Access-Control-Allow-Origin", "*");
        request->send(response);
    }

    bool parseUnsignedParam(AsyncWebServerRequest * request,
                            const char * name,
                            uint32_t maximum,
                            uint32_t & value)
    {
        if (!request->hasParam(name, false, false))
            return false;

        const String text =
            request->getParam(name, false, false)->value();
        if (text.length() == 0)
            return false;

        char * end = nullptr;
        const unsigned long parsed = strtoul(text.c_str(), &end, 10);
        if (end == text.c_str() || *end != '\0' || parsed > maximum)
            return false;

        value = static_cast<uint32_t>(parsed);
        return true;
    }

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
        _server.on("/status",          HTTP_GET, [this](AsyncWebServerRequest * pRequest) { this->status(pRequest); });

        _server.begin();
        debugI("HTTP server started");
    }

    void setLed(AsyncWebServerRequest * pRequest)
    {
        uint32_t index = 0;
        if (!parseUnsignedParam(pRequest, "index", NUM_LEDS1 - 1, index))
        {
            sendResponse(pRequest, 400, "application/json",
                         "{\"error\":\"index must be between 0 and 120\"}");
            return;
        }

        RunSingleLedTest(static_cast<uint8_t>(index));
        sendResponse(pRequest, 202, "application/json",
                     "{\"accepted\":true}");
    }

    void setBrightness(AsyncWebServerRequest * pRequest)
    {
        uint32_t value = 0;
        if (!parseUnsignedParam(pRequest, "value", 255, value))
        {
            sendResponse(pRequest, 400, "application/json",
                         "{\"error\":\"value must be between 0 and 255\"}");
            return;
        }

        SetLedBrightness(static_cast<uint8_t>(value));
        SaveBrightness(static_cast<uint8_t>(value));
        sendResponse(pRequest, 200, "application/json",
                     "{\"updated\":true}");
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
        uint32_t dir = 0;
        if (!parseUnsignedParam(pRequest, "dir", 8, dir))
        {
            sendResponse(pRequest, 400, "application/json",
                         "{\"error\":\"dir must be between 0 and 8\"}");
            return;
        }
        debugI("Sweep triggered via API: dir=%u",
               static_cast<unsigned>(dir));
        RunSweep(static_cast<uint8_t>(dir));
        AsyncWebServerResponse * pResponse = pRequest->beginResponse(200);
        pResponse->addHeader("Access-Control-Allow-Origin", "*");
        pRequest->send(pResponse);
    }

    void status(AsyncWebServerRequest * pRequest)
    {
        String json;
        json.reserve(320);
        json += "{\"uptimeMs\":";
        json += millis();
        json += ",\"animationsStopped\":";
        json += AreAnimationsStopped() ? "true" : "false";
        json += ",\"cancellationPending\":";
        json += IsSceneCancellationPending() ? "true" : "false";
        json += ",\"startupOpening\":";
        json += GetStartupOpeningSelection();
        json += ",\"schedulerRemainingMs\":";
        json += GetSchedulerRemainingMs();
        json += ",\"brightness\":";
        json += GetLedBrightness();
        json += ",\"publishedFrames\":";
        json += GetPublishedFrameCount();
        json += ",\"powerLimitMilliamps\":";
        json += GetLedPowerLimitMilliamps();
        json += ",\"outputEnabled\":";
        json += IsLedOutputEnabled() ? "true" : "false";
        json += ",\"wifiConnected\":";
        json += WiFi.isConnected() ? "true" : "false";
        json += ",\"wifiRssi\":";
        json += WiFi.isConnected() ? WiFi.RSSI() : 0;
        json += ",\"networkServicesStarted\":";
        json += NetworkServicesStarted() ? "true" : "false";
        json += ",\"networkReconnects\":";
        json += GetNetworkReconnectCount();
        json += ",\"freeHeap\":";
        json += ESP.getFreeHeap();
        json += "}";
        sendResponse(pRequest, 200, "application/json", json);
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