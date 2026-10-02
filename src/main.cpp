#include "globals.h"
#include <Arduino.h>
#include <ArduinoOTA.h>                         // For updating the flash over WiFi
#include <Preferences.h>
#include "network.h"                            // For WiFi credentials
#include "drawing.h"
#include "apiwebserver.h"
#include "ledrenderer.h"

//
// Task Handles to our running threads
//
TaskHandle_t g_taskScreen = nullptr;
TaskHandle_t g_taskSync   = nullptr;
TaskHandle_t g_taskWeb    = nullptr;
TaskHandle_t g_taskDraw   = nullptr;
TaskHandle_t g_taskHeart  = nullptr;
TaskHandle_t g_taskJackpot = nullptr;
TaskHandle_t g_taskMachine = nullptr;
TaskHandle_t g_taskSerial = nullptr;
TaskHandle_t g_taskLedRender = nullptr;
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

#if ENABLE_WEBSERVER
    DRAM_ATTR ApiWebServer g_WebServer;
#endif

CRGB leds0[NUM_LEDS0];  // been
CRGB leds1[NUM_LEDS1];  // overig

constexpr const char * kPrefsNamespace = "pinbot";
constexpr const char * kBrightnessKey = "brightness";

uint8_t LoadSavedBrightness()
{
    Preferences prefs;
    uint8_t brightness = kDefaultBrightness;

    if (prefs.begin(kPrefsNamespace, true))
    {
        brightness = prefs.getUChar(kBrightnessKey, kDefaultBrightness);
        prefs.end();
    }

    return brightness;
}

void SaveBrightness(uint8_t value)
{
    Preferences prefs;

    if (prefs.begin(kPrefsNamespace, false))
    {
        prefs.putUChar(kBrightnessKey, value);
        prefs.end();
    }
}

void ProcessSerialSceneCommand()
{
    static String command;

    while (Serial.available() > 0)
    {
        const char input = static_cast<char>(Serial.read());
        if (input != '\n' && input != '\r')
        {
            command += input;
            continue;
        }

        command.trim();
        if (command.length() == 0)
            continue;

        bool accepted = true;
        if (command == "1" || command.equalsIgnoreCase("vortex"))
            RunQuantumVortex();
        else if (command == "2" || command.equalsIgnoreCase("lightning"))
            RunLightningStorm();
        else if (command == "3" || command.equalsIgnoreCase("neonrings"))
            RunNeonRings();
        else if (command == "4" || command.equalsIgnoreCase("artworkstory"))
            RunArtworkStory();
        else if (command == "5" || command.equalsIgnoreCase("fireworks"))
            RunFireworks();
        else if (command == "6" || command.equalsIgnoreCase("lasergrid"))
            RunLaserMatrix();
        else if (command == "7" || command.equalsIgnoreCase("ghostbride"))
            RunGhostBride();
        else if (command == "8" || command.equalsIgnoreCase("multiball"))
            RunMultiball();
        else if (command == "9" || command.equalsIgnoreCase("eclipse"))
            RunSolarEclipse();
        else if (command == "10" || command.equalsIgnoreCase("prismshatter"))
            RunPrismShatter();
        else if (command == "22" || command.equalsIgnoreCase("crimsontakeover"))
            RunCrimsonTakeover();
        else if (command == "11" || command.equalsIgnoreCase("opening-showcase"))
            RunOpeningShowcase();
        else if (command == "12" || command.equalsIgnoreCase("opening-cosmic"))
            RunCosmicOpening();
        else if (command == "13" || command.equalsIgnoreCase("opening-bride"))
            RunBrideAssemblyOpening();
        else if (command == "14" || command.equalsIgnoreCase("opening-launch"))
            RunLaunchControlOpening();
        else if (command == "15" || command.equalsIgnoreCase("opening-city"))
            RunCityAwakeningOpening();
        else if (command == "16" || command.equalsIgnoreCase("opening-diagnostics"))
            RunDiagnosticsOpening();
        else if (command == "17" || command.equalsIgnoreCase("opening-transmission"))
            RunStellarTransmissionOpening();
        else if (command == "18" || command.equalsIgnoreCase("opening-pulse"))
            RunPulseOfLifeOpening();
        else if (command == "19" || command.equalsIgnoreCase("opening-moonlight"))
            RunMoonlightRevealOpening();
        else if (command.equalsIgnoreCase("jackpot"))
            TriggerJackpotCelebration();
        else if (command.equalsIgnoreCase("stop"))
            SetAllStopped(true);
        else if (command.equalsIgnoreCase("resume"))
            SetAllStopped(false);
        else if (command.equalsIgnoreCase("status"))
        {
            Serial.printf(
                "[STATUS] stopped=%u output=%u cancel=%u opening=%u brightness=%u "
                "frames=%lu wifi=%u reconnects=%lu heap=%u\n",
                AreAnimationsStopped(),
                IsLedOutputEnabled(),
                IsSceneCancellationPending(),
                GetStartupOpeningSelection(),
                GetLedBrightness(),
                static_cast<unsigned long>(GetPublishedFrameCount()),
                WiFi.isConnected(),
                static_cast<unsigned long>(GetNetworkReconnectCount()),
                ESP.getFreeHeap());
        }
        else
        {
            accepted = false;
            Serial.printf("[SERIAL] unknown command: %s\n", command.c_str());
        }

        if (accepted)
            Serial.printf("[SERIAL] command accepted: %s\n", command.c_str());
        command = "";
    }
}

void SerialSceneTaskEntry(void *)
{
    for (;;)
    {
        ProcessSerialSceneCommand();
        delay(10);
    }
}

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
    Serial.printf("[BOOT] setup started at %lu ms\n", millis());

    // Re-route debug output to the serial port
    Debug.setSerialEnabled(true);

    const uint8_t startupBrightness = LoadSavedBrightness();
    Serial.printf("[BOOT] registering LED strips\n");
    InitializeLedRenderer(startupBrightness);
    Serial.printf("[BOOT] brightness=%u, power limit=%umA\n",
                  startupBrightness, kLedPowerLimitMilliamps);

    // Start dark; one of the nine theatrical openings owns both strips.
    fill_solid(leds0, NUM_LEDS0, CRGB::Black);
    fill_solid(leds1, NUM_LEDS1, CRGB::Black);
    PublishLedFrame();
    const BaseType_t ledRenderResult = xTaskCreatePinnedToCore(
        LedRenderTaskEntry, "LED Render", STACK_SIZE, nullptr,
        LED_RENDER_PRIORITY, &g_taskLedRender, DRAWING_CORE);
    Serial.printf("[BOOT] LED render task=%ld (pdPASS=%ld)\n",
                  static_cast<long>(ledRenderResult),
                  static_cast<long>(pdPASS));
    const uint8_t startupOpening = PrepareRandomStartupOpening();
    Serial.printf("[BOOT] randomly selected opening %u of 9\n", startupOpening);

    const BaseType_t shuttleResult = xTaskCreatePinnedToCore(
        DrawLoopTaskEntryOne, "Shuttle", STACK_SIZE, nullptr, DRAWING_PRIORITY, &g_taskDraw, DRAWING_CORE);
    const BaseType_t heartResult = xTaskCreatePinnedToCore(
        DrawLoopTaskEntryTwo, "Heart", STACK_SIZE, nullptr, DRAWING_PRIORITY, &g_taskHeart, DRAWING_CORE);
    const BaseType_t jackpotResult = xTaskCreatePinnedToCore(
        DrawLoopTaskEntryThree, "Jackpot", STACK_SIZE, nullptr, DRAWING_PRIORITY, &g_taskJackpot, DRAWING_CORE);
    const BaseType_t machineResult = xTaskCreatePinnedToCore(
        DrawLoopTaskEntryFour, "TheMachine", STACK_SIZE, nullptr, DRAWING_PRIORITY, &g_taskMachine, DRAWING_CORE);
    Serial.printf(
        "[BOOT] drawing tasks shuttle=%ld heart=%ld jackpot=%ld machine=%ld (pdPASS=%ld)\n",
        static_cast<long>(shuttleResult),
        static_cast<long>(heartResult),
        static_cast<long>(jackpotResult),
        static_cast<long>(machineResult),
        static_cast<long>(pdPASS));

    const BaseType_t serialResult = xTaskCreatePinnedToCore(
        SerialSceneTaskEntry, "Serial Scenes", STACK_SIZE, nullptr,
        REMOTE_PRIORITY, &g_taskSerial, NET_CORE);
    Serial.printf("[BOOT] serial scene task=%ld (pdPASS=%ld)\n",
                  static_cast<long>(serialResult), static_cast<long>(pdPASS));

    const BaseType_t debugResult = xTaskCreatePinnedToCore(
        DebugLoopTaskEntry, "Debug Loop", STACK_SIZE, nullptr, DEBUG_PRIORITY, &g_taskDebug, DEBUG_CORE);
    Serial.printf("[BOOT] debug task=%ld (pdPASS=%ld)\n",
                  static_cast<long>(debugResult), static_cast<long>(pdPASS));

    const BaseType_t networkResult = xTaskCreatePinnedToCore(
        NetworkLoopTaskEntry, "Network", STACK_SIZE, nullptr,
        NET_PRIORITY, &g_taskNet, NET_CORE);
    Serial.printf(
        "[BOOT] background network task=%ld (pdPASS=%ld); setup complete\n",
        static_cast<long>(networkResult), static_cast<long>(pdPASS));
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