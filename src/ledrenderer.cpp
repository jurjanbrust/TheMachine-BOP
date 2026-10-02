#include "globals.h"
#include "ledrenderer.h"

#include <cstring>

namespace
{
    constexpr TickType_t kMinimumFrameInterval = pdMS_TO_TICKS(16);
    constexpr uint8_t kJackpotLedCount = 48;
    constexpr uint8_t kJackpotBlendAmount = 36;

    CRGB g_outputLeds0[NUM_LEDS0];
    CRGB g_outputLeds1[NUM_LEDS1];
    CRGB g_pendingLeds0[NUM_LEDS0];
    CRGB g_pendingLeds1[NUM_LEDS1];

    portMUX_TYPE g_frameMux = portMUX_INITIALIZER_UNLOCKED;
    TaskHandle_t g_renderTask = nullptr;
    volatile uint8_t g_brightness = kDefaultBrightness;
    volatile uint32_t g_publishedFrameCount = 0;
    volatile bool g_outputEnabled = true;
}

void InitializeLedRenderer(uint8_t brightness)
{
    FastLED.addLeds<WS2812B, LED_PIN0, GRB>(g_outputLeds0, NUM_LEDS0)
        .setCorrection(TypicalLEDStrip);
    FastLED.addLeds<WS2812B, LED_PIN1, GRB>(g_outputLeds1, NUM_LEDS1)
        .setCorrection(TypicalLEDStrip);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, kLedPowerLimitMilliamps);
    FastLED.setBrightness(brightness);
    g_brightness = brightness;

    fill_solid(g_outputLeds0, NUM_LEDS0, CRGB::Black);
    fill_solid(g_outputLeds1, NUM_LEDS1, CRGB::Black);
    fill_solid(g_pendingLeds0, NUM_LEDS0, CRGB::Black);
    fill_solid(g_pendingLeds1, NUM_LEDS1, CRGB::Black);
}

void PublishLedFrame()
{
    portENTER_CRITICAL(&g_frameMux);
    ::memcpy(g_pendingLeds0, leds0, sizeof(g_pendingLeds0));
    ::memcpy(g_pendingLeds1, leds1, sizeof(g_pendingLeds1));
    ++g_publishedFrameCount;
    TaskHandle_t renderTask = g_renderTask;
    portEXIT_CRITICAL(&g_frameMux);

    if (renderTask != nullptr)
        xTaskNotifyGive(renderTask);
}

void SetLedBrightness(uint8_t brightness)
{
    portENTER_CRITICAL(&g_frameMux);
    g_brightness = brightness;
    portEXIT_CRITICAL(&g_frameMux);

    if (g_renderTask != nullptr)
        xTaskNotifyGive(g_renderTask);
}

uint8_t GetLedBrightness()
{
    return g_brightness;
}

uint32_t GetPublishedFrameCount()
{
    return g_publishedFrameCount;
}

uint16_t GetLedPowerLimitMilliamps()
{
    return kLedPowerLimitMilliamps;
}

void SetLedOutputEnabled(bool enabled)
{
    portENTER_CRITICAL(&g_frameMux);
    g_outputEnabled = enabled;
    portEXIT_CRITICAL(&g_frameMux);

    if (g_renderTask != nullptr)
        xTaskNotifyGive(g_renderTask);
}

bool IsLedOutputEnabled()
{
    return g_outputEnabled;
}

void LedRenderTaskEntry(void *)
{
    g_renderTask = xTaskGetCurrentTaskHandle();
    TickType_t lastFrameTick = 0;

    for (;;)
    {
        const TickType_t waitTime =
            g_outputEnabled ? kMinimumFrameInterval : portMAX_DELAY;
        ulTaskNotifyTake(pdTRUE, waitTime);

        const TickType_t now = xTaskGetTickCount();
        const TickType_t elapsed = now - lastFrameTick;
        if (lastFrameTick != 0 && elapsed < kMinimumFrameInterval)
            vTaskDelay(kMinimumFrameInterval - elapsed);

        uint8_t brightness;
        bool outputEnabled;
        CRGB jackpotTarget[kJackpotLedCount];
        portENTER_CRITICAL(&g_frameMux);
        ::memcpy(jackpotTarget, g_pendingLeds0, sizeof(jackpotTarget));
        ::memcpy(
            &g_outputLeds0[kJackpotLedCount],
            &g_pendingLeds0[kJackpotLedCount],
            sizeof(g_outputLeds0) - sizeof(jackpotTarget));
        ::memcpy(g_outputLeds1, g_pendingLeds1, sizeof(g_outputLeds1));
        brightness = g_brightness;
        outputEnabled = g_outputEnabled;
        portEXIT_CRITICAL(&g_frameMux);

        if (!outputEnabled)
        {
            fill_solid(g_outputLeds0, NUM_LEDS0, CRGB::Black);
            fill_solid(g_outputLeds1, NUM_LEDS1, CRGB::Black);
        }
        else
        {
            for (uint8_t i = 0; i < kJackpotLedCount; ++i)
            {
                g_outputLeds0[i] = blend(
                    g_outputLeds0[i], jackpotTarget[i],
                    kJackpotBlendAmount);
            }
        }
        FastLED.setBrightness(brightness);
        FastLED.show();
        lastFrameTick = xTaskGetTickCount();
    }
}
