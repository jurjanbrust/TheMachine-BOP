#include "globals.h"
#include "drawing.h"
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi
#include <cstring>

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.  

extern uint32_t           g_FPS;
extern bool               g_bUpdateStarted;

namespace
{
    constexpr uint16_t kGlobalHeartIntervalSeconds = 300;   // 5 minutes
    constexpr uint32_t kGlobalHeartDurationMs      = 15000;  // run heartbeat for 15s
    constexpr uint32_t kMachineModeDurationMs      = 60000;  // rotate every minute
    constexpr uint8_t  kMachineLedCount            = theMachineLastLed - theMachineFirstLed + 1;
    constexpr uint8_t  kJackpotSegments            = 8;
    constexpr uint8_t  kJackpotLedsPerSegment      = 6;
    constexpr uint8_t  kJackpotLedCount            = kJackpotSegments * kJackpotLedsPerSegment;
    constexpr uint8_t  kPlanetCount                = 5;
    constexpr uint16_t kPlanetSparkleIntervalMs    = 150;
    constexpr uint8_t  kPlanetSparkleDecay         = 220;
    constexpr uint8_t  kShuttleFirstLed            = 55;
    constexpr uint8_t  kShuttleLedCount            = 3;
    constexpr uint32_t kShuttleModeDurationMs      = 15000;
    constexpr uint8_t  kStreetLedCount             = 5;
    constexpr uint32_t kStreetModeDurationMs       = 12000;
    constexpr uint32_t kStreetRunnerIntervalMs     = 120;
    constexpr uint8_t  kStreetSparkleDecay         = 210;
    constexpr uint32_t kShowcaseDimDurationMs      = 2500;
    constexpr uint32_t kShowcaseRampDurationMs     = 2000;
    constexpr uint32_t kShowcaseHoldDurationMs     = 1000;

    constexpr uint8_t kPlanetIndices[kPlanetCount] = {
        moonTopLeft,
        bigBluePlanetLeftSide,
        bigBluePlanetRightSide,
        jupiterUpper,
        jupiterLower
    };

    const CRGB kPlanetBaseColors[kPlanetCount] = {
        CRGB::AntiqueWhite,
        CRGB::DeepSkyBlue,
        CRGB::DeepSkyBlue,
        CRGB::OrangeRed,
        CRGB::OrangeRed
    };

    constexpr uint8_t kStreetIndices[kStreetLedCount] = {
        people,
        carright1,
        carright2,
        carleft1,
        carleft2
    };

    CRGB g_planetSparkleLayer[kPlanetCount] = {};
    bool g_planetHighlightActive = false;
    uint32_t g_frontheadPulseStart = 0;
    CRGB g_streetSparkleLayer[kStreetLedCount] = {};
    CRGB g_jackpotDimBuffer[kJackpotLedCount] = {};
    bool g_globalHeartActive = false;

    void UpdatePlanetSparkles()
    {
        for (uint8_t i = 0; i < kPlanetCount; ++i)
        {
            g_planetSparkleLayer[i].fadeToBlackBy(kPlanetSparkleDecay);
            leds1[kPlanetIndices[i]] += g_planetSparkleLayer[i];
        }

        const uint8_t sparkleIdx = random8(kPlanetCount);
        g_planetSparkleLayer[sparkleIdx] += CRGB::White;
    }

    void UpdateFrontheadAccent()
    {
        if (!g_planetHighlightActive)
            return;

        const uint8_t pulse = beatsin8(30, 40, 220);
        CRGB accent = CRGB::White;
        accent.nscale8_video(pulse);
        leds1[fronthead] = accent;
    }

    void RenderStreetPulse()
    {
        const uint8_t wave = beatsin8(24, 60, 255);
        for (uint8_t i = 0; i < kStreetLedCount; ++i)
        {
            CRGB color = CRGB::White;
            color.nscale8_video(wave);
            leds1[kStreetIndices[i]] = color;
        }
    }

    void RenderStreetSparkle()
    {
        for (uint8_t i = 0; i < kStreetLedCount; ++i)
        {
            g_streetSparkleLayer[i].fadeToBlackBy(kStreetSparkleDecay);
            leds1[kStreetIndices[i]] += g_streetSparkleLayer[i];
        }

        const uint8_t sparkleIdx = random8(kStreetLedCount);
        g_streetSparkleLayer[sparkleIdx] += CHSV(random8(), 200, 255);
    }

    enum class MachineMode : uint8_t
    {
        Rainbow = 0,
        Pulse,
        Sparkle,
        Scanner,
        Showcase,
        Count
    };

    enum class JackpotMode : uint8_t
    {
        Classic = 0,
        AlternatingFill,
        DualChase,
        Meteor,
        RainbowSweep,
        Sparkle,
        Pulse,
        Count
    };

    enum class ShuttleMode : uint8_t
    {
        Flicker = 0,
        Wave,
        Boost,
        Count
    };

    enum class StreetMode : uint8_t
    {
        Pulse = 0,
        Runner,
        Sparkle,
        Count
    };

    static const uint8_t kHeartbeatTable[] = {
        25,  61, 105, 153, 197, 233, 253, 255,
        252, 243, 230, 213, 194, 149, 101, 105,
        153, 197, 216, 233, 244, 253, 255, 255,
        252, 249, 243, 237, 230, 223, 213, 206,
        194, 184, 174, 162, 149, 138, 126, 112,
        101,  91,  78,  69,  62,  58,  51,  47,
         43,  39,  37,  35,  29,  25,  22,  20,
         19,  15,  12,   9,   8,   6,   5,   3
    };

    constexpr uint8_t HeartbeatTableSize()
    {
        return static_cast<uint8_t>(sizeof(kHeartbeatTable) / sizeof(kHeartbeatTable[0]));
    }

    uint8_t GetHeartbeatBrightness(uint8_t bpm = 35)
    {
        const uint8_t steps = HeartbeatTableSize();
        const uint8_t hbIndex = lerp8by8(0, steps, beat8(bpm));
        return lerp8by8(0, 255, kHeartbeatTable[hbIndex]);
    }

    void FillMachineRange(const CRGB & color)
    {
        for (uint8_t i = 0; i < kMachineLedCount; ++i)
        {
            leds1[theMachineFirstLed + i] = color;
        }
    }

    void RenderMachineRainbow()
    {
        const uint8_t baseHue = beat8(12);
        CHSV hsv(baseHue, 240, 255);
        for (uint8_t i = 0; i < kMachineLedCount; ++i)
        {
            leds1[theMachineFirstLed + i] = hsv;
            hsv.hue += 10;
        }
        FastLED.show();
    }

    void RenderMachinePulse()
    {
        CRGB color = CRGB::DeepPink;
        color.nscale8_video(GetHeartbeatBrightness(30));
        FillMachineRange(color);
        FastLED.show();
    }

    void RenderMachineSparkle()
    {
        fadeToBlackBy(&leds1[theMachineFirstLed], kMachineLedCount, 40);
        const uint8_t idx = random8(kMachineLedCount);
        leds1[theMachineFirstLed + idx] = CRGB::White;
        FastLED.show();
        delay(30);
    }

    void RenderMachineScanner()
    {
        static int8_t direction = 1;
        static uint8_t position = 0;

        FillMachineRange(CRGB::Black);
        const uint8_t ledIndex = theMachineFirstLed + position;
        leds1[ledIndex] = CRGB::Red;
        FastLED.show();

        if (position == 0)
            direction = 1;
        else if (position == kMachineLedCount - 1)
            direction = -1;

        position = static_cast<uint8_t>(position + direction);
        delay(40);
    }

    struct ShowcaseState
    {
        bool initialized = false;
        uint8_t stage = 0;
        uint32_t stageStart = 0;
    };

    ShowcaseState g_showcaseState;

    void ResetShowcaseState()
    {
        g_showcaseState = ShowcaseState{};
    }

    uint8_t ShowcaseIntensity(uint32_t elapsed)
    {
        if (elapsed >= kShowcaseRampDurationMs)
            return 255;
        return static_cast<uint8_t>((elapsed * 255UL) / kShowcaseRampDurationMs);
    }

    void RenderMachineShowcase()
    {
        const uint32_t now = millis();
        if (!g_showcaseState.initialized)
        {
            g_showcaseState.initialized = true;
            g_showcaseState.stage = 0;
            g_showcaseState.stageStart = now;
        }

        auto advanceStage = [&](uint8_t nextStage) {
            g_showcaseState.stage = nextStage;
            g_showcaseState.stageStart = millis();
        };

        switch (g_showcaseState.stage)
        {
            case 0:
            {
                g_planetHighlightActive = false;
                fadeToBlackBy(leds0, NUM_LEDS0, 20);
                fadeToBlackBy(leds1, NUM_LEDS1, 20);
                FastLED.show();
                if (now - g_showcaseState.stageStart >= kShowcaseDimDurationMs)
                {
                    advanceStage(1);
                }
                break;
            }
            case 1:
            {
                g_planetHighlightActive = false;
                const uint32_t elapsed = now - g_showcaseState.stageStart;
                CRGB machineColor = CRGB(246, 200, 160);
                machineColor.nscale8_video(ShowcaseIntensity(elapsed));
                FillMachineRange(machineColor);
                FastLED.show();
                if (elapsed >= kShowcaseRampDurationMs + kShowcaseHoldDurationMs)
                {
                    advanceStage(2);
                }
                break;
            }
            case 2:
            {
                if (!g_planetHighlightActive)
                    g_frontheadPulseStart = now;
                g_planetHighlightActive = true;
                const uint32_t elapsed = now - g_showcaseState.stageStart;
                const uint8_t intensity = ShowcaseIntensity(elapsed);
                for (uint8_t i = 0; i < kPlanetCount; ++i)
                {
                    CRGB color = kPlanetBaseColors[i];
                    color.nscale8_video(intensity);
                    leds1[kPlanetIndices[i]] = color;
                }
                UpdateFrontheadAccent();
                FastLED.show();
                if (elapsed >= kShowcaseRampDurationMs + kShowcaseHoldDurationMs)
                {
                    advanceStage(3);
                }
                break;
            }
            case 3:
            {
                g_planetHighlightActive = false;
                const uint32_t elapsed = now - g_showcaseState.stageStart;
                CRGB foreheadColor = CRGB::White;
                foreheadColor.nscale8_video(ShowcaseIntensity(elapsed));
                leds1[fronthead] = foreheadColor;
                FastLED.show();
                if (elapsed >= kShowcaseRampDurationMs + kShowcaseHoldDurationMs)
                {
                    advanceStage(0);
                }
                break;
            }
            default:
                g_planetHighlightActive = false;
                advanceStage(0);
                break;
        }
    }

    void RunMachineMode(MachineMode mode)
    {
        if (mode != MachineMode::Showcase)
        {
            ResetShowcaseState();
        }

        switch (mode)
        {
            case MachineMode::Rainbow:
                RenderMachineRainbow();
                break;
            case MachineMode::Pulse:
                RenderMachinePulse();
                break;
            case MachineMode::Sparkle:
                RenderMachineSparkle();
                break;
            case MachineMode::Scanner:
                RenderMachineScanner();
                break;
            case MachineMode::Showcase:
                RenderMachineShowcase();
                break;
            default:
                break;
        }
    }

    MachineMode NextMachineMode(MachineMode mode)
    {
        auto next = static_cast<uint8_t>(mode) + 1;
        const auto maxModes = static_cast<uint8_t>(MachineMode::Count);
        if (next >= maxModes)
            next = 0;
        return static_cast<MachineMode>(next);
    }

    void RunGlobalHeartMode()
    {
        g_globalHeartActive = true;
        const uint32_t start = millis();
        while (millis() - start < kGlobalHeartDurationMs)
        {
            const uint8_t brightness = GetHeartbeatBrightness();
            CRGB strip0 = CRGB::Red;
            strip0.nscale8_video(brightness);
            CRGB strip1 = CRGB::BlueViolet;
            strip1.nscale8_video(brightness);

            fill_solid(leds0, NUM_LEDS0, strip0);
            fill_solid(leds1, NUM_LEDS1, strip1);
            FastLED.show();
            delay(30);
        }
        g_globalHeartActive = false;
    }

    void ShowJackpotDimmed()
    {
        memcpy(g_jackpotDimBuffer, leds0, sizeof(g_jackpotDimBuffer));
        for (uint8_t i = 0; i < kJackpotLedCount; ++i)
        {
            CRGB scaled = g_jackpotDimBuffer[i];
            scaled.nscale8_video(18);
            leds0[i] = scaled;
        }
        FastLED.show();
        memcpy(leds0, g_jackpotDimBuffer, sizeof(g_jackpotDimBuffer));
    }

    void ClearJackpotRange()
    {
        for (uint8_t i = 0; i < kJackpotLedCount; ++i)
        {
            leds0[i] = CRGB::Black;
        }
    }

    void RunJackpotClassic()
    {
        for (uint8_t current = 0; current < kJackpotLedCount; current += kJackpotLedsPerSegment)
        {
            for (uint8_t i = 0; i < kJackpotLedsPerSegment; ++i)
            {
                leds0[current + i] = CRGB::Red;
            }
            ShowJackpotDimmed();
            delay(300);

            for (uint8_t i = 0; i < kJackpotLedsPerSegment; ++i)
            {
                leds0[current + i] = CRGB::Black;
            }
        }

        for (int current = kJackpotLedCount - kJackpotLedsPerSegment; current >= 0; current -= kJackpotLedsPerSegment)
        {
            for (uint8_t i = 0; i < kJackpotLedsPerSegment; ++i)
            {
                leds0[current + i] = CRGB::Red;
            }
            ShowJackpotDimmed();
            delay(300);

            for (uint8_t i = 0; i < kJackpotLedsPerSegment; ++i)
            {
                leds0[current + i] = CRGB::Black;
            }
        }
    }

    void RunJackpotAlternatingFill()
    {
        static const CRGB palette[] = { CRGB::DarkOrange, CRGB::Gold, CRGB::Red };
        constexpr size_t paletteSize = sizeof(palette) / sizeof(palette[0]);

        ClearJackpotRange();

        for (size_t colorIdx = 0; colorIdx < paletteSize; ++colorIdx)
        {
            for (uint8_t segment = 0; segment < kJackpotSegments; ++segment)
            {
                for (uint8_t led = 0; led < kJackpotLedsPerSegment; ++led)
                {
                    leds0[segment * kJackpotLedsPerSegment + led] = palette[colorIdx];
                }
                ShowJackpotDimmed();
                delay(120);
            }
        }

        ClearJackpotRange();
    }

    void RunJackpotDualChase()
    {
        ClearJackpotRange();
        int left = 0;
        int right = kJackpotLedCount - 1;

        while (left <= right)
        {
            leds0[left] = CRGB::Cyan;
            leds0[right] = CRGB::Magenta;
            ShowJackpotDimmed();
            delay(90);
            leds0[left] = CRGB::Black;
            leds0[right] = CRGB::Black;
            ++left;
            --right;
        }
    }

    void RunJackpotMeteor()
    {
        ClearJackpotRange();
        const CRGB meteorColor = CRGB::DeepSkyBlue;
        constexpr uint8_t meteorSize = 5;
        constexpr uint8_t trailDecay = 70;
        const int totalSteps = kJackpotLedCount + kJackpotLedsPerSegment;

        for (int step = 0; step < totalSteps; ++step)
        {
            for (uint8_t i = 0; i < kJackpotLedCount; ++i)
            {
                leds0[i].fadeToBlackBy(trailDecay);
            }

            for (uint8_t i = 0; i < meteorSize; ++i)
            {
                int idx = step - i;
                if (idx >= 0 && idx < kJackpotLedCount)
                {
                    leds0[idx] = meteorColor;
                }
            }

            ShowJackpotDimmed();
            delay(40);
        }
    }

    void RunJackpotRainbowSweep()
    {
        static uint8_t hueBase = 0;
        for (uint8_t i = 0; i < kJackpotLedCount; ++i)
        {
            leds0[i] = CHSV(hueBase + i * 4, 240, 255);
        }
        ShowJackpotDimmed();
        hueBase += 3;
        delay(80);
    }

    void RunJackpotSparkle()
    {
        fadeToBlackBy(leds0, kJackpotLedCount, 40);
        constexpr uint8_t sparkleCount = 5;
        for (uint8_t i = 0; i < sparkleCount; ++i)
        {
            leds0[random8(kJackpotLedCount)] += CHSV(random8(), 200, 255);
        }
        ShowJackpotDimmed();
        delay(60);
    }

    void RunJackpotPulse()
    {
        CRGB color = CRGB::Gold;
        color.nscale8_video(GetHeartbeatBrightness(28));
        fill_solid(leds0, kJackpotLedCount, color);
        ShowJackpotDimmed();
        delay(50);
    }

    void RunJackpotMode(JackpotMode mode)
    {
        switch (mode)
        {
            case JackpotMode::Classic:
                RunJackpotClassic();
                break;
            case JackpotMode::AlternatingFill:
                RunJackpotAlternatingFill();
                break;
            case JackpotMode::DualChase:
                RunJackpotDualChase();
                break;
            case JackpotMode::Meteor:
                RunJackpotMeteor();
                break;
            case JackpotMode::RainbowSweep:
                RunJackpotRainbowSweep();
                break;
            case JackpotMode::Sparkle:
                RunJackpotSparkle();
                break;
            case JackpotMode::Pulse:
                RunJackpotPulse();
                break;
            default:
                break;
        }
    }

    JackpotMode NextJackpotMode(JackpotMode mode)
    {
        auto next = static_cast<uint8_t>(mode) + 1;
        const auto maxModes = static_cast<uint8_t>(JackpotMode::Count);
        if (next >= maxModes)
            next = 0;
        return static_cast<JackpotMode>(next);
    }

    CRGB * GetShuttleSegment()
    {
        return &leds1[kShuttleFirstLed];
    }

    void RenderShuttleFlicker()
    {
        CRGB * segment = GetShuttleSegment();
        for (uint8_t i = 0; i < kShuttleLedCount; ++i)
        {
            const uint8_t heat = random8(160, 255);
            segment[i] = CHSV(10 + random8(8), 255, heat);
        }
        FastLED.show();
        delay(35);
    }

    void RenderShuttleWave()
    {
        static uint8_t offset = 0;
        CRGB * segment = GetShuttleSegment();
        for (uint8_t i = 0; i < kShuttleLedCount; ++i)
        {
            const uint8_t wave = sin8(offset + i * 32);
            segment[i] = CHSV(5 + wave / 6, 220, 150 + (wave >> 2));
        }
        offset += 6;
        FastLED.show();
        delay(45);
    }

    void RenderShuttleBoost()
    {
        const uint8_t pulse = beatsin8(18, 150, 255);
        CRGB * segment = GetShuttleSegment();
        for (uint8_t i = 0; i < kShuttleLedCount; ++i)
        {
            const uint8_t blendAmount = static_cast<uint8_t>((i * 255) / kShuttleLedCount);
            CRGB heat = CRGB::Orange;
            heat.nscale8_video(pulse);
            segment[i] = blend(CRGB::White, heat, blendAmount);
        }
        FastLED.show();
        delay(30);
    }

    void RunShuttleMode(ShuttleMode mode)
    {
        switch (mode)
        {
            case ShuttleMode::Flicker:
                RenderShuttleFlicker();
                break;
            case ShuttleMode::Wave:
                RenderShuttleWave();
                break;
            case ShuttleMode::Boost:
                RenderShuttleBoost();
                break;
            default:
                break;
        }
    }

    ShuttleMode NextShuttleMode(ShuttleMode mode)
    {
        auto next = static_cast<uint8_t>(mode) + 1;
        const auto maxModes = static_cast<uint8_t>(ShuttleMode::Count);
        if (next >= maxModes)
            next = 0;
        return static_cast<ShuttleMode>(next);
    }

    void RunStreetMode(StreetMode mode)
    {
        switch (mode)
        {
            case StreetMode::Pulse:
                RenderStreetPulse();
                break;
            case StreetMode::Sparkle:
                RenderStreetSparkle();
                break;
            default:
                break;
        }
    }

    StreetMode NextStreetMode(StreetMode mode)
    {
        auto next = static_cast<uint8_t>(mode) + 1;
        const auto maxModes = static_cast<uint8_t>(StreetMode::Count);
        if (next >= maxModes)
            next = 0;
        return static_cast<StreetMode>(next);
    }
}

void PostDrawHandler()
{
    // Once an OTA flash update has started, we don't want to hog the CPU or it goes quite slowly, 
    // so we'll pause to share the CPU a bit once the update has begun
    if (g_bUpdateStarted)
        delay(1000);
    
    // If we didn't draw anything, we near-busy-wait so that we are continually checking the clock for an packet
    // whose time has come
    delay(5);
}

// used for pinbot jackpot
void DrawWalkingDot()
{
    static JackpotMode currentMode = JackpotMode::Classic;
    RunJackpotMode(currentMode);
    currentMode = NextJackpotMode(currentMode);
}

void JackPotDefaultColors()
{
    // jackpot = 8 vakjes met 6 leds per stuk
    // eerste 4 = geel
    // laatste 4 = rood
    for(int vakje = 0; vakje < 4; vakje++) {
        for(int ledje = 0; ledje < 6; ledje++) {
            leds0[vakje*6+ledje] = CRGB::DarkOrange;
        }
    }

    for(int vakje = 4; vakje < 8; vakje++) {
        for(int ledje = 0; ledje < 6; ledje++) {
            leds0[vakje*6+ledje] = CRGB::Red;
        }
    }
    ShowJackpotDimmed();
}

void TheMachineLogo(CRGB color = CRGB(246,200,160))
{
        int start = 8;
        for (int i = start; i < start+12; i++) {
			leds1[i] = color;
        }
        FastLED.show();
}

void TheBride(CRGB color = CRGB(246,200,160))
{
        uint leds[] = {3,5,6,62,63,64,65,66,67,68,69,79,88,94,95,96,97,98,103,104,105,106,107,108,109,110,111,112,113,115,116,117,118};

        for (uint i = 0; i < 33 ; i++) {
			leds1[leds[i]] = color;
        }
        FastLED.show();
}

void SingleLed(int index, CRGB color = CRGB(246,200,160))
{
    leds1[index] = color;
    FastLED.show();
}

void ColorFillEffect(CRGB color = CRGB(246,200,160), int nrOfLeds = 10, int everyNth = 10)
{
		for (int i = 0; i < nrOfLeds; i+= everyNth) {
			leds1[i] = color;
        }

        FastLED.show();
}

void Heartbeat(int channel)
{
    const uint8_t brightness = GetHeartbeatBrightness();
  if(channel == 0) {
    leds0[NUM_LEDS0 -1] = CRGB::Red;
    leds0[NUM_LEDS0 -1].fadeLightBy(brightness);
  } else if (channel == 1) {
    leds0[NUM_LEDS0 -2] = CRGB::BlueViolet;
    leds0[NUM_LEDS0 -2].fadeLightBy(brightness);
    leds0[NUM_LEDS0 -3] = CRGB::BlueViolet;
    leds0[NUM_LEDS0 -3].fadeLightBy(brightness);
    leds0[NUM_LEDS0 -4] = CRGB::BlueViolet;
    leds0[NUM_LEDS0 -4].fadeLightBy(brightness);
    leds0[NUM_LEDS0 -5] = CRGB::BlueViolet;
    leds0[NUM_LEDS0 -5].fadeLightBy(brightness);
  }
  FastLED.show();
  //FastLED.setBrightness( lerp8by8( 0, 255, brightness ) ); // interpolate to max MAX_BRIGHTNESS
}

void Eyes(CRGB color = CRGB(246,200,160))
{
    leds0[NUM_LEDS0 -2] = color;   // oog links
    leds0[NUM_LEDS0 -3] = color;   // oog 2e links
    leds0[NUM_LEDS0 -4] = color;   // oog 2e rechts
    leds0[NUM_LEDS0 -5] = color;   // oog rechts

    FastLED.show();
}

// shuttle flames
void IRAM_ATTR DrawLoopTaskEntryOne(void *)
{
    ShuttleMode currentMode = ShuttleMode::Flicker;
    uint32_t lastModeChange = millis();
    uint32_t lastSparkleUpdate = millis();
    StreetMode currentStreetMode = StreetMode::Pulse;
    uint32_t lastStreetModeChange = millis();

    for (;;)
    {
        if (g_globalHeartActive)
        {
            PostDrawHandler();
            continue;
        }

        const uint32_t now = millis();
        if (now - lastModeChange >= kShuttleModeDurationMs)
        {
            currentMode = NextShuttleMode(currentMode);
            lastModeChange = now;
        }

        if (now - lastStreetModeChange >= kStreetModeDurationMs)
        {
            currentStreetMode = NextStreetMode(currentStreetMode);
            lastStreetModeChange = now;
        }

        if (now - lastSparkleUpdate >= kPlanetSparkleIntervalMs)
        {
            UpdatePlanetSparkles();
            lastSparkleUpdate = now;
        }

        RunStreetMode(currentStreetMode);
        RunShuttleMode(currentMode);
        PostDrawHandler();
    }
}

// heart
void IRAM_ATTR DrawLoopTaskEntryTwo(void *) 
{
    for (;;)
    {
        Heartbeat(0);
        EVERY_N_SECONDS(kGlobalHeartIntervalSeconds)
        {
            RunGlobalHeartMode();
        }
        PostDrawHandler();
    }
}

// jackpot (been)
void IRAM_ATTR DrawLoopTaskEntryThree(void *)
{
    for (;;)
    {
        if (g_globalHeartActive)
        {
            PostDrawHandler();
            continue;
        }

        for(int i=0; i<5; i++) {
            DrawWalkingDot();
            delay(1000*1);
            if (g_globalHeartActive)
                break;
        }
        if (g_globalHeartActive)
        {
            PostDrawHandler();
            continue;
        }
        JackPotDefaultColors();
        delay(1000*10);
        PostDrawHandler();
    }
}

// the machine logo
void IRAM_ATTR DrawLoopTaskEntryFour(void *)
{
    MachineMode currentMode = MachineMode::Rainbow;
    uint32_t lastModeChange = millis();

    for (;;)
    {
        if (g_globalHeartActive)
        {
            PostDrawHandler();
            continue;
        }

        const uint32_t now = millis();
        if (now - lastModeChange >= kMachineModeDurationMs)
        {
            currentMode = NextMachineMode(currentMode);
            lastModeChange = now;
            debugI("Switching The Machine mode to %u", static_cast<unsigned>(currentMode));
        }

        RunMachineMode(currentMode);
        PostDrawHandler();
    }
}
