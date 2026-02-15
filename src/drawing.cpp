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
    constexpr uint32_t kAutoJackpotIntervalMs      = 600000; // 10 minutes
    constexpr uint32_t kMachineActiveDurationMs    = 30000;  // active mode: 30 seconds
    constexpr uint32_t kMachineIdleDurationMs      = 120000; // idle pause: 2 minutes
    constexpr uint8_t  kMachineLedCount            = theMachineLastLed - theMachineFirstLed + 1;
    constexpr uint8_t  kJackpotSegments            = 8;
    constexpr uint8_t  kJackpotLedsPerSegment      = 6;
    constexpr uint8_t  kJackpotLedCount            = kJackpotSegments * kJackpotLedsPerSegment;
    constexpr uint8_t  kJackpotDimScale            = 80;
    constexpr uint32_t kJackpotModeDurationMs      = 15000;
    constexpr uint32_t kJackpotDimmedDurationMs    = 60000;
    constexpr uint32_t kJackpotClassicIntervalMs   = 220;
    constexpr uint32_t kJackpotFillIntervalMs      = 180;
    constexpr uint32_t kJackpotChaseIntervalMs     = 140;
    constexpr uint32_t kJackpotMeteorIntervalMs    = 90;
    constexpr uint32_t kJackpotRainbowIntervalMs   = 120;
    constexpr uint32_t kJackpotSparkleIntervalMs   = 110;
    constexpr uint32_t kJackpotPulseIntervalMs     = 100;
    constexpr uint32_t kJackpotPlasmaIntervalMs    = 90;
    constexpr uint32_t kJackpotDimmedIntervalMs    = 1000;
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
    constexpr uint32_t kCarBlinkIntervalMs         = 800;
    constexpr uint8_t  kEyeBreathBpm              = 10;
    constexpr uint32_t kShuttleLaunchRampMs        = 5000;
    constexpr uint32_t kShuttleLaunchHoldMs        = 1500;
    constexpr uint32_t kShuttleLaunchFadeMs        = 2000;
    constexpr uint32_t kShuttleLaunchPauseMs       = 3000;
    constexpr uint32_t kShuttleLaunchTotalMs       = kShuttleLaunchRampMs + kShuttleLaunchHoldMs + kShuttleLaunchFadeMs + kShuttleLaunchPauseMs;
    constexpr uint32_t kShowcaseDimDurationMs      = 2500;
    constexpr uint32_t kShowcaseRampDurationMs     = 2000;
    constexpr uint32_t kShowcaseHoldDurationMs     = 1000;
    constexpr uint32_t kShowcaseFlickerDurationMs  = 10000;
    constexpr uint32_t kMoonPhaseDurationMs        = 15000;
    constexpr uint8_t  kMoonPhaseCount             = 7;
    constexpr uint8_t  kAppleGlowBpm               = 6;
    constexpr uint32_t kMeteorShowerIntervalMs     = 1200000; // 20 minutes
    constexpr uint8_t  kMeteorShowerLength          = 6;
    constexpr uint8_t  kMeteorShowerTrailDecay      = 64;
    constexpr uint32_t kBrideModeDurationMs         = 20000;
    constexpr uint8_t  kBrideLedCount               = 33;
    constexpr uint32_t kJackpotCelebrationDurationMs = 5000;
    constexpr uint32_t kSpotlightWashCycleMs        = 10000;
    constexpr uint32_t kAwakeningDurationMs          = 60000; // 1 minute total
    constexpr uint32_t kAutoAwakeningIntervalMs       = 1800000; // 30 minutes

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

    constexpr uint8_t kBrideIndices[kBrideLedCount] = {
        3,5,6,62,63,64,65,66,67,68,69,79,88,94,95,96,97,98,
        103,104,105,106,107,108,109,110,111,112,113,115,116,117,118
    };

    CRGB g_planetSparkleLayer[kPlanetCount] = {};
    bool g_planetHighlightActive = false;
    uint32_t g_frontheadPulseStart = 0;
    CRGB g_streetSparkleLayer[kStreetLedCount] = {};
    bool g_globalHeartActive = false;
    bool g_showcaseActive = false;
    volatile bool g_jackpotCelebrationRequested = false;
    bool g_jackpotCelebrationActive = false;
    volatile bool g_awakeningRequested = false;
    bool g_awakeningActive = false;
    volatile bool g_allStopped = false;
    const CRGB kSpotlightColor = CRGB::White;

    // Meteor shower state
    struct MeteorShower
    {
        bool active = false;
        int16_t position = 0;
        uint8_t hue = 0;
        uint32_t nextSpawn = 0;
    };
    MeteorShower g_meteorShower;
    CRGB g_meteorTrail[NUM_LEDS1] = {};

    // Bride animation modes
    enum class BrideMode : uint8_t
    {
        Aurora = 0,
        Starfield,
        Count
    };
    BrideMode g_brideMode = BrideMode::Aurora;
    uint32_t g_brideModeStart = 0;
    uint8_t g_brideStarfieldBrightness[kBrideLedCount] = {};


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

    void RenderCarHeadlights()
    {
        // Alternating left/right pairs like passing traffic
        const uint8_t phase = (millis() / kCarBlinkIntervalMs) % 2;
        // Warm yellow headlight color
        const CRGB headlight = CRGB(255, 200, 60);
        const CRGB dim       = CRGB(40, 30, 8);

        if (phase == 0)
        {
            // Left pair on, right pair dim
            leds1[carleft1]   = headlight;
            leds1[carleft2]   = headlight;
            leds1[carright1]  = dim;
            leds1[carright2]  = dim;
        }
        else
        {
            // Right pair on, left pair dim
            leds1[carright1]  = headlight;
            leds1[carright2]  = headlight;
            leds1[carleft1]   = dim;
            leds1[carleft2]   = dim;
        }
        // People LED pulses gently alongside
        const uint8_t peopleBrightness = beatsin8(12, 80, 200);
        CRGB peopleColor = CRGB::White;
        peopleColor.nscale8_video(peopleBrightness);
        leds1[people] = peopleColor;
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
        Comet,
        Showcase,
        Idle,
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
        Plasma,
        DimmedHold,
        Count
    };

    enum class ShuttleMode : uint8_t
    {
        Flicker = 0,
        Wave,
        Boost,
        Launch,
        Count
    };

    enum class StreetMode : uint8_t
    {
        Pulse = 0,
        Runner,
        Sparkle,
        CarHeadlights,
        Count
    };

    struct JackpotRuntime
    {
        JackpotMode mode = JackpotMode::Classic;
        uint32_t modeStart = 0;
        uint32_t nextFrame = 0;
        uint32_t frameInterval = kJackpotClassicIntervalMs;
        uint32_t modeDuration = kJackpotModeDurationMs;
        uint8_t step = 0;
        uint8_t secondary = 0;
        bool forward = true;
        uint8_t hueBase = 0;
        bool dimOutput = true;
    };

    JackpotRuntime g_jackpotRuntime;
    CRGB g_jackpotFrame[kJackpotLedCount];

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

    void SetSpotlights(const CRGB & color)
    {
        leds1[spotlights1] = color;
        leds1[spotlights2] = color;
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

    void RenderMachineComet()
    {
        // Fade the trail
        fadeToBlackBy(&leds1[theMachineFirstLed], kMachineLedCount, 80);

        // Comet head position sweeps back and forth
        static uint8_t cometPos = 0;
        static int8_t cometDir = 1;

        leds1[theMachineFirstLed + cometPos] = CRGB::White;
        // Slight warm glow behind the head
        if (cometPos >= 1)
        {
            CRGB trail = CRGB(246, 200, 160);
            trail.nscale8_video(180);
            leds1[theMachineFirstLed + cometPos - 1] += trail;
        }

        if (cometPos == 0)
            cometDir = 1;
        else if (cometPos >= kMachineLedCount - 1)
            cometDir = -1;
        cometPos = static_cast<uint8_t>(cometPos + cometDir);

        FastLED.show();
        delay(50);
    }

    // --- Moon Phase Animation ---
    void UpdateMoonPhases()
    {
        // Cycle: full (3 lit) → gibbous (2) → crescent (1) → new (0) → crescent (1) → gibbous (2) → full (3)
        const uint32_t phaseIndex = (millis() / kMoonPhaseDurationMs) % kMoonPhaseCount;
        // Map phase index to number of lit LEDs: 3,2,1,0,1,2,3
        static const uint8_t kPhaseLitCount[] = {3, 2, 1, 0, 1, 2, 3};
        const uint8_t litCount = kPhaseLitCount[phaseIndex];

        // Color shifts from cool white (full) to warm gold (crescent)
        const uint8_t warmth = (litCount == 3) ? 0 : (litCount == 2) ? 60 : (litCount == 1) ? 120 : 0;
        const CRGB moonColor = CRGB(255, 255 - warmth / 3, 255 - warmth);

        for (uint8_t i = 0; i < 3; ++i)
        {
            if (i < litCount)
                leds1[moonTopLeft + i] = moonColor;
            else
                leds1[moonTopLeft + i] = CRGB::Black;
        }
    }

    // --- Reactive Apple Glow ---
    void UpdateAppleGlow()
    {
        // Slow pulse between green and red
        const uint8_t blend = beatsin8(kAppleGlowBpm, 0, 255);
        leds1[apple] = CRGB(blend, 255 - blend, 0);
    }

    // --- Aurora / Northern Lights on The Bride ---
    void RenderBrideAurora()
    {
        const uint32_t now = millis();
        for (uint8_t i = 0; i < kBrideLedCount; ++i)
        {
            // Overlapping sine waves for organic movement
            const uint8_t waveA = sin8((now / 23) + i * 19);
            const uint8_t waveB = sin8((now / 37) + i * 31);
            const uint8_t combined = qadd8(waveA, waveB) / 2;

            // Aurora palette: greens, teals, purples, blues
            // Hue centered around green-teal (96) with purple shifts
            const uint8_t hue = 80 + (waveA / 4) + (waveB / 8);
            const uint8_t sat = qadd8(180, waveB / 4);
            const uint8_t val = scale8(combined, 200);

            leds1[kBrideIndices[i]] = CHSV(hue, sat, val);
        }
    }

    // --- Starfield on The Bride ---
    void RenderBrideStarfield()
    {
        for (uint8_t i = 0; i < kBrideLedCount; ++i)
        {
            // Slow gentle fade — scale8 with 235 gives a longer, softer decay
            if (g_brideStarfieldBrightness[i] > 2)
                g_brideStarfieldBrightness[i] = scale8(g_brideStarfieldBrightness[i], 235);
            else
                g_brideStarfieldBrightness[i] = 0;

            // Very dim base — barely visible starlight
            const uint8_t baseDim = 4;
            const uint8_t brightness = max(baseDim, g_brideStarfieldBrightness[i]);
            // Slight cool-white tint
            leds1[kBrideIndices[i]] = CRGB(brightness, brightness, brightness + (brightness >> 3));
        }

        // Sparse twinkle — ~12% chance per frame, peak at 140 (not full white)
        if (random8() < 30)
        {
            const uint8_t idx = random8(kBrideLedCount);
            g_brideStarfieldBrightness[idx] = 140;
        }
    }

    void UpdateBrideAnimation()
    {
        const uint32_t now = millis();
        if (g_brideModeStart == 0)
            g_brideModeStart = now;

        if (now - g_brideModeStart >= kBrideModeDurationMs)
        {
            g_brideMode = static_cast<BrideMode>(
                (static_cast<uint8_t>(g_brideMode) + 1) % static_cast<uint8_t>(BrideMode::Count));
            g_brideModeStart = now;
            memset(g_brideStarfieldBrightness, 0, sizeof(g_brideStarfieldBrightness));
        }

        switch (g_brideMode)
        {
            case BrideMode::Aurora:
                RenderBrideAurora();
                break;
            case BrideMode::Starfield:
                RenderBrideStarfield();
                break;
            default:
                break;
        }
    }

    // --- Meteor Shower across Strip 1 ---
    void UpdateMeteorShower()
    {
        const uint32_t now = millis();

        // Fade the trail layer
        for (uint16_t i = 0; i < NUM_LEDS1; ++i)
            g_meteorTrail[i].fadeToBlackBy(kMeteorShowerTrailDecay);

        if (!g_meteorShower.active)
        {
            if (now >= g_meteorShower.nextSpawn)
            {
                // Spawn a new meteor
                g_meteorShower.active = true;
                g_meteorShower.position = 0;
                g_meteorShower.hue = random8();
                g_meteorShower.nextSpawn = now + kMeteorShowerIntervalMs;
            }
        }
        else
        {
            // Draw the meteor head and trail
            for (uint8_t j = 0; j < kMeteorShowerLength; ++j)
            {
                int16_t pos = g_meteorShower.position - j;
                if (pos >= 0 && pos < NUM_LEDS1)
                {
                    const uint8_t brightness = 255 - (j * (200 / kMeteorShowerLength));
                    g_meteorTrail[pos] = CHSV(g_meteorShower.hue, 100, brightness);
                }
            }

            g_meteorShower.position += 2;  // Speed: 2 LEDs per frame
            if (g_meteorShower.position >= NUM_LEDS1 + kMeteorShowerLength)
            {
                g_meteorShower.active = false;
            }
        }

        // Blend meteor trail onto strip 1 (additive)
        for (uint16_t i = 0; i < NUM_LEDS1; ++i)
            leds1[i] += g_meteorTrail[i];
    }

    // --- Jackpot Win Celebration ---
    void RunJackpotCelebration()
    {
        g_jackpotCelebrationActive = true;
        const uint32_t start = millis();

        // Phase 1: Rapid rainbow flash (2 seconds)
        while (millis() - start < 2000)
        {
            const uint8_t hue = beat8(120);
            for (uint8_t i = 0; i < kJackpotLedCount; ++i)
            {
                leds0[i] = CHSV(hue + i * 5, 255, 255);
            }
            FastLED.show();
            delay(20);
        }

        // Phase 2: Golden cascade fill with sparkle (3 seconds)
        const uint32_t phase2Start = millis();
        uint8_t filledSegments = 0;
        while (millis() - phase2Start < 3000)
        {
            const uint32_t elapsed = millis() - phase2Start;
            const uint8_t targetSegments = static_cast<uint8_t>(
                min(static_cast<uint32_t>(kJackpotSegments), (elapsed * kJackpotSegments) / 2500));

            // Fill new segments with gold
            while (filledSegments < targetSegments)
            {
                const uint8_t base = filledSegments * kJackpotLedsPerSegment;
                for (uint8_t j = 0; j < kJackpotLedsPerSegment; ++j)
                    leds0[base + j] = CRGB::Gold;
                ++filledSegments;
            }

            // Random sparkle on filled LEDs
            if (filledSegments > 0)
            {
                const uint8_t sparkIdx = random8(filledSegments * kJackpotLedsPerSegment);
                leds0[sparkIdx] = CRGB::White;
            }

            FastLED.show();
            delay(30);

            // Fade sparkles back to gold
            for (uint8_t i = 0; i < filledSegments * kJackpotLedsPerSegment; ++i)
            {
                leds0[i] = blend(leds0[i], CRGB::Gold, 60);
            }
        }

        g_jackpotCelebrationActive = false;
    }

    // --- Spotlight Color Wash (during Showcase hold) ---
    CRGB GetSpotlightWashColor()
    {
        // Slow cycle: white → warm white → soft amber → back
        const uint8_t progress = beatsin8(6, 0, 255);
        // Interpolate from pure white to warm amber
        return CRGB(255, lerp8by8(255, 200, progress), lerp8by8(255, 140, progress));
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
        g_showcaseActive = false;
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
            g_showcaseActive = true;

            // Black out everything at the start
            fill_solid(leds0, NUM_LEDS0, CRGB::Black);
            fill_solid(leds1, NUM_LEDS1, CRGB::Black);
            FastLED.show();
        }

        auto advanceStage = [&](uint8_t nextStage) {
            g_showcaseState.stage = nextStage;
            g_showcaseState.stageStart = millis();
        };

        switch (g_showcaseState.stage)
        {
            case 0: // Fluorescent tube flicker for 10 seconds, everything else dark
            {
                g_planetHighlightActive = false;
                const uint32_t elapsed = now - g_showcaseState.stageStart;

                // Keep everything black except the spotlights
                fill_solid(leds0, NUM_LEDS0, CRGB::Black);
                fill_solid(leds1, NUM_LEDS1, CRGB::Black);

                // Fluorescent tube simulation: on-probability ramps up over time
                const uint8_t progress = static_cast<uint8_t>(
                    min(255UL, (elapsed * 255UL) / kShowcaseFlickerDurationMs));
                const uint8_t onChance = lerp8by8(40, 255, progress);
                // Occasional dark bursts early on
                const bool burstOff = (random8() < 20) && (progress < 200);
                const bool isOn = (random8() < onChance) && !burstOff;

                const CRGB spotColor = isOn ? kSpotlightColor : CRGB::Black;
                leds1[spotlights1] = spotColor;
                leds1[spotlights2] = spotColor;
                FastLED.show();
                delay(random8(30, 120));

                if (elapsed >= kShowcaseFlickerDurationMs)
                {
                    advanceStage(1);
                }
                break;
            }
            case 1: // Spotlights on solid, ramp up logo and planets
            {
                const uint32_t elapsed = now - g_showcaseState.stageStart;
                const uint8_t intensity = ShowcaseIntensity(elapsed);

                // Spotlights stay on permanently
                SetSpotlights(kSpotlightColor);

                // Ramp up The Machine logo
                CRGB machineColor = CRGB(246, 200, 160);
                machineColor.nscale8_video(intensity);
                FillMachineRange(machineColor);

                // Ramp up planets in their base colors
                for (uint8_t i = 0; i < kPlanetCount; ++i)
                {
                    CRGB color = kPlanetBaseColors[i];
                    color.nscale8_video(intensity);
                    leds1[kPlanetIndices[i]] = color;
                }

                FastLED.show();

                if (elapsed >= kShowcaseRampDurationMs + kShowcaseHoldDurationMs)
                {
                    g_showcaseActive = false;
                    advanceStage(2);
                }
                break;
            }
            case 2: // Hold — spotlights wash color, logo, and planets stay lit
            {
                SetSpotlights(GetSpotlightWashColor());
                FillMachineRange(CRGB(246, 200, 160));
                for (uint8_t i = 0; i < kPlanetCount; ++i)
                {
                    leds1[kPlanetIndices[i]] = kPlanetBaseColors[i];
                }
                FastLED.show();
                break;
            }
            default:
                g_showcaseActive = false;
                advanceStage(0);
                break;
        }
    }

    void RenderMachineIdle()
    {
        static const CRGB idleColor(246, 200, 160);
        FillMachineRange(idleColor);
        FastLED.show();
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
            case MachineMode::Comet:
                RenderMachineComet();
                break;
            case MachineMode::Showcase:
                RenderMachineShowcase();
                break;
            case MachineMode::Idle:
                RenderMachineIdle();
                break;
            default:
                break;
        }
    }

    MachineMode NextActiveMachineMode(MachineMode mode)
    {
        static const MachineMode kActiveModes[] = {
            MachineMode::Rainbow,
            MachineMode::Pulse,
            MachineMode::Sparkle,
            MachineMode::Scanner,
            MachineMode::Comet,
            MachineMode::Showcase
        };
        constexpr size_t kActiveCount = sizeof(kActiveModes) / sizeof(kActiveModes[0]);

        size_t idx = 0;
        for (; idx < kActiveCount; ++idx)
        {
            if (kActiveModes[idx] == mode)
                break;
        }
        if (idx >= kActiveCount)
        {
            idx = 0;
        }
        else
        {
            idx = (idx + 1) % kActiveCount;
        }
        return kActiveModes[idx];
    }

    void RunGlobalHeartMode()
    {
        g_globalHeartActive = true;

        // --- Transition: cross-fade current state to heart colors over 2 seconds ---
        // Snapshot the current LED state
        CRGB snapshot0[NUM_LEDS0];
        CRGB snapshot1[NUM_LEDS1];
        memcpy(snapshot0, leds0, sizeof(snapshot0));
        memcpy(snapshot1, leds1, sizeof(snapshot1));

        constexpr uint32_t kFadeInDurationMs = 2000;
        const uint32_t fadeStart = millis();
        while (millis() - fadeStart < kFadeInDurationMs)
        {
            const uint32_t elapsed = millis() - fadeStart;
            const uint8_t blendAmount = static_cast<uint8_t>(
                min(255UL, (elapsed * 255UL) / kFadeInDurationMs));

            // Target colors at modest brightness for the transition end
            const CRGB targetRed = CRGB(180, 0, 0);
            const CRGB targetViolet = CRGB(100, 0, 180);

            for (uint16_t i = 0; i < NUM_LEDS0; ++i)
                leds0[i] = blend(snapshot0[i], targetRed, blendAmount);
            for (uint16_t i = 0; i < NUM_LEDS1; ++i)
                leds1[i] = blend(snapshot1[i], targetViolet, blendAmount);

            FastLED.show();
            delay(20);
        }

        // --- Main heartbeat loop ---
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

    // ==================== Awakening Mode ====================
    // A 1-minute theatrical sequence: the bride comes alive.
    //
    // Timeline:
    //   0–10s : Eyes slowly open (fade from black to BlueViolet)
    //  10–20s : Heart LED starts beating, faint → strong
    //  20–30s : Moon fades in, bride outline starts dim aurora
    //  30–40s : Planets fade to base colors, apple glows, fronthead accent
    //  40–50s : Machine logo ramps warm white, shuttle flickers, jackpot warms up
    //  50–58s : Street scene lights up, spotlights flicker on
    //  58–60s : Hold at full, then release to normal animations

    void RunAwakeningMode()
    {
        g_awakeningActive = true;

        // Black out everything
        fill_solid(leds0, NUM_LEDS0, CRGB::Black);
        fill_solid(leds1, NUM_LEDS1, CRGB::Black);
        FastLED.show();
        delay(500); // Brief dramatic pause in total darkness

        const uint32_t startMs = millis();

        while (millis() - startMs < kAwakeningDurationMs)
        {
            const uint32_t elapsed = millis() - startMs;

            // Keep everything black as a base each frame, then paint active elements
            fill_solid(leds0, NUM_LEDS0, CRGB::Black);
            fill_solid(leds1, NUM_LEDS1, CRGB::Black);

            // ---- Phase 1 (0–10s): Eyes slowly open ----
            {
                // Ramp brightness 0 → 255 over 10 seconds, then hold steady
                const uint8_t eyeBright = (elapsed < 10000)
                    ? static_cast<uint8_t>((elapsed * 255UL) / 10000)
                    : 255;
                CRGB eyeColor = CHSV(200, 200, eyeBright);
                leds0[NUM_LEDS0 - 2] = eyeColor;
                leds0[NUM_LEDS0 - 3] = eyeColor;
                leds0[NUM_LEDS0 - 4] = eyeColor;
                leds0[NUM_LEDS0 - 5] = eyeColor;
            }

            // ---- Phase 2 (10–20s): Heart starts beating, growing stronger ----
            if (elapsed >= 10000)
            {
                uint8_t heartMax;
                if (elapsed < 20000)
                {
                    // Ramp max heartbeat amplitude from 30 → 255 over 10s
                    heartMax = static_cast<uint8_t>(30 + ((elapsed - 10000) * 225UL) / 10000);
                }
                else
                {
                    heartMax = 255;
                }
                const uint8_t hbRaw = GetHeartbeatBrightness();
                const uint8_t heartBright = scale8(hbRaw, heartMax);
                CRGB heartColor = CRGB::Red;
                heartColor.nscale8_video(heartBright);
                leds0[NUM_LEDS0 - 1] = heartColor;
            }

            // ---- Phase 3 (20–30s): Moon fades in + bride outline aurora ----
            if (elapsed >= 20000)
            {
                uint8_t phaseBlend;
                if (elapsed < 30000)
                    phaseBlend = static_cast<uint8_t>(((elapsed - 20000) * 255UL) / 10000);
                else
                    phaseBlend = 255;

                // Moon: fade in as crescent first (1 LED), expanding
                const uint8_t moonLit = (phaseBlend < 85) ? 1 : (phaseBlend < 170) ? 2 : 3;
                const CRGB moonColor = CRGB(
                    scale8(255, phaseBlend),
                    scale8(220, phaseBlend),
                    scale8(180, phaseBlend));
                for (uint8_t i = 0; i < moonLit; ++i)
                    leds1[moonTopLeft + i] = moonColor;

                // Bride aurora at scaled intensity
                const uint32_t now = millis();
                for (uint8_t i = 0; i < kBrideLedCount; ++i)
                {
                    const uint8_t waveA = sin8((now / 23) + i * 19);
                    const uint8_t waveB = sin8((now / 37) + i * 31);
                    const uint8_t combined = qadd8(waveA, waveB) / 2;
                    const uint8_t hue = 80 + (waveA / 4) + (waveB / 8);
                    const uint8_t sat = qadd8(180, waveB / 4);
                    const uint8_t val = scale8(scale8(combined, 200), phaseBlend);
                    leds1[kBrideIndices[i]] = CHSV(hue, sat, val);
                }

                // Fronthead accent fading in (purple)
                CRGB accent = CRGB::Purple;
                accent.nscale8_video(scale8(beatsin8(30, 40, 220), phaseBlend));
                leds1[fronthead] = accent;
            }

            // ---- Phase 4 (30–40s): Planets + apple awaken ----
            if (elapsed >= 30000)
            {
                uint8_t phaseBlend;
                if (elapsed < 40000)
                    phaseBlend = static_cast<uint8_t>(((elapsed - 30000) * 255UL) / 10000);
                else
                    phaseBlend = 255;

                // Planets fade in to their base colors
                for (uint8_t i = 0; i < kPlanetCount; ++i)
                {
                    CRGB pColor = kPlanetBaseColors[i];
                    pColor.nscale8_video(phaseBlend);
                    leds1[kPlanetIndices[i]] = pColor;
                }

                // Apple: blend green↔red pulse, scaled by fade-in
                const uint8_t appleBlend = beatsin8(kAppleGlowBpm, 0, 255);
                CRGB appleColor = CRGB(appleBlend, 255 - appleBlend, 0);
                appleColor.nscale8_video(phaseBlend);
                leds1[apple] = appleColor;

                // Fingers accent
                CRGB fingerColor = CRGB(246, 200, 160);
                fingerColor.nscale8_video(phaseBlend);
                leds1[fingersLeftCorner] = fingerColor;
            }

            // ---- Phase 5 (40–50s): Machine logo + shuttle + jackpot ----
            if (elapsed >= 40000)
            {
                uint8_t phaseBlend;
                if (elapsed < 50000)
                    phaseBlend = static_cast<uint8_t>(((elapsed - 40000) * 255UL) / 10000);
                else
                    phaseBlend = 255;

                // Machine logo: warm white ramp
                CRGB logoColor = CRGB(246, 200, 160);
                logoColor.nscale8_video(phaseBlend);
                for (uint8_t i = 0; i < kMachineLedCount; ++i)
                    leds1[theMachineFirstLed + i] = logoColor;

                // Shuttle: flickering flame fading in
                for (uint8_t i = 0; i < kShuttleLedCount; ++i)
                {
                    CRGB flame = CHSV(10 + random8(8), 220, random8(160, 255));
                    flame.nscale8_video(phaseBlend);
                    leds1[kShuttleFirstLed + i] = flame;
                }

                // Jackpot: warm amber sweeping in from center outward
                const uint8_t segmentsLit = 1 + (phaseBlend * 7) / 255;
                const uint8_t segBright = scale8(180, phaseBlend);
                for (uint8_t seg = 0; seg < segmentsLit; ++seg)
                {
                    // Light from center outward: segments 3,4 first, then 2,5, then 1,6, then 0,7
                    const uint8_t fromCenter = seg;
                    const uint8_t segA = 3 - min((uint8_t)3, fromCenter);
                    const uint8_t segB = 4 + min((uint8_t)3, fromCenter);

                    CRGB jColor = (seg % 2 == 0) ? CRGB::DarkOrange : CRGB::Red;
                    jColor.nscale8_video(segBright);

                    for (uint8_t led = 0; led < kJackpotLedsPerSegment; ++led)
                    {
                        leds0[segA * kJackpotLedsPerSegment + led] = jColor;
                        leds0[segB * kJackpotLedsPerSegment + led] = jColor;
                    }
                }
            }

            // ---- Phase 6 (50–58s): Street scene + spotlights ----
            if (elapsed >= 50000)
            {
                uint8_t phaseBlend;
                if (elapsed < 58000)
                    phaseBlend = static_cast<uint8_t>(((elapsed - 50000) * 255UL) / 8000);
                else
                    phaseBlend = 255;

                // Street LEDs fade in
                CRGB streetColor = CRGB::White;
                streetColor.nscale8_video(phaseBlend);
                for (uint8_t i = 0; i < kStreetLedCount; ++i)
                    leds1[kStreetIndices[i]] = streetColor;

                // Car headlights warm up
                CRGB headlight = CRGB(255, 200, 60);
                headlight.nscale8_video(phaseBlend);
                leds1[carright1] = headlight;
                leds1[carright2] = headlight;
                leds1[carleft1]  = headlight;
                leds1[carleft2]  = headlight;

                // Spotlights: fluorescent tube flicker for 8 seconds (50–58s)
                if (elapsed < 58000)
                {
                    // Probability of being on increases over the 8s window
                    const uint8_t flickerProgress = static_cast<uint8_t>(
                        ((elapsed - 50000) * 255UL) / 8000);
                    const uint8_t onChance = lerp8by8(80, 220, flickerProgress);
                    const bool on = (random8() < onChance);
                    CRGB spotColor = on ? CRGB::White : CRGB::Black;
                    leds1[spotlights1] = spotColor;
                    leds1[spotlights2] = spotColor;
                }
                else
                {
                    leds1[spotlights1] = CRGB::White;
                    leds1[spotlights2] = CRGB::White;
                }
            }

            FastLED.show();
            delay(25);
        }

        // Brief hold at full brightness
        delay(100);
        g_awakeningActive = false;
    }

    CRGB DimJackpotColor(CRGB color)
    {
        color.nscale8_video(kJackpotDimScale);
        return color;
    }

    void SetJackpotLed(uint8_t index, const CRGB & color)
    {
        g_jackpotFrame[index] = color;
    }

    void FillJackpotSegment(uint8_t segment, const CRGB & color)
    {
        const uint8_t base = segment * kJackpotLedsPerSegment;
        for (uint8_t i = 0; i < kJackpotLedsPerSegment; ++i)
        {
            SetJackpotLed(base + i, color);
        }
    }

    void ClearJackpotRange()
    {
        for (uint8_t i = 0; i < kJackpotLedCount; ++i)
        {
            g_jackpotFrame[i] = CRGB::Black;
        }
    }

    void ShowJackpotDimmed()
    {
        const bool shouldDim = g_planetHighlightActive || g_jackpotRuntime.dimOutput;
        for (uint8_t i = 0; i < kJackpotLedCount; ++i)
        {
            leds0[i] = shouldDim ? DimJackpotColor(g_jackpotFrame[i]) : g_jackpotFrame[i];
        }
        FastLED.show();
    }

    void ApplyJackpotDefaultColors()
    {
        for (uint8_t segment = 0; segment < kJackpotSegments; ++segment)
        {
            const CRGB color = (segment < (kJackpotSegments / 2)) ? CRGB::DarkOrange : CRGB::Red;
            FillJackpotSegment(segment, color);
        }
    }

    uint32_t JackpotIntervalForMode(JackpotMode mode)
    {
        switch (mode)
        {
            case JackpotMode::Classic:
                return kJackpotClassicIntervalMs;
            case JackpotMode::AlternatingFill:
                return kJackpotFillIntervalMs;
            case JackpotMode::DualChase:
                return kJackpotChaseIntervalMs;
            case JackpotMode::Meteor:
                return kJackpotMeteorIntervalMs;
            case JackpotMode::RainbowSweep:
                return kJackpotRainbowIntervalMs;
            case JackpotMode::Sparkle:
                return kJackpotSparkleIntervalMs;
            case JackpotMode::Pulse:
                return kJackpotPulseIntervalMs;
            case JackpotMode::Plasma:
                return kJackpotPlasmaIntervalMs;
            case JackpotMode::DimmedHold:
                return kJackpotDimmedIntervalMs;
            default:
                return kJackpotClassicIntervalMs;
        }
    }

    uint32_t JackpotDurationForMode(JackpotMode mode)
    {
        if (mode == JackpotMode::DimmedHold)
        {
            return kJackpotDimmedDurationMs;
        }
        return kJackpotModeDurationMs;
    }

    void ResetJackpotRuntime(JackpotMode mode, uint32_t now)
    {
        g_jackpotRuntime = JackpotRuntime{};
        g_jackpotRuntime.mode = mode;
        g_jackpotRuntime.modeStart = now;
        g_jackpotRuntime.nextFrame = now;
        g_jackpotRuntime.frameInterval = JackpotIntervalForMode(mode);
        g_jackpotRuntime.modeDuration = JackpotDurationForMode(mode);
        g_jackpotRuntime.dimOutput = true;

        bool shouldClear = true;
        switch (mode)
        {
            case JackpotMode::Classic:
                g_jackpotRuntime.step = 0;
                g_jackpotRuntime.secondary = kJackpotSegments;
                g_jackpotRuntime.forward = true;
                break;
            case JackpotMode::AlternatingFill:
                g_jackpotRuntime.step = 0;
                g_jackpotRuntime.secondary = 0;
                g_jackpotRuntime.dimOutput = false;
                break;
            case JackpotMode::DualChase:
                g_jackpotRuntime.step = 0;
                g_jackpotRuntime.secondary = kJackpotLedCount - 1;
                g_jackpotRuntime.dimOutput = false;
                break;
            case JackpotMode::Meteor:
                g_jackpotRuntime.step = 0;
                break;
            case JackpotMode::RainbowSweep:
                g_jackpotRuntime.hueBase = 0;
                break;
            case JackpotMode::Plasma:
                g_jackpotRuntime.step = 0;
                g_jackpotRuntime.hueBase = 0;
                break;
            case JackpotMode::DimmedHold:
                ApplyJackpotDefaultColors();
                g_jackpotRuntime.step = 0;
                shouldClear = false;
                break;
            case JackpotMode::Sparkle:
            case JackpotMode::Pulse:
            default:
                break;
        }

        if (shouldClear)
        {
            ClearJackpotRange();
        }
    }

    void StepJackpotClassic()
    {
        if (g_jackpotRuntime.secondary < kJackpotSegments && g_jackpotRuntime.secondary != g_jackpotRuntime.step)
        {
            FillJackpotSegment(g_jackpotRuntime.secondary, CRGB::Black);
        }
        FillJackpotSegment(g_jackpotRuntime.step, CRGB::Red);
        g_jackpotRuntime.secondary = g_jackpotRuntime.step;

        if (g_jackpotRuntime.forward)
        {
            if (g_jackpotRuntime.step >= kJackpotSegments - 1)
            {
                g_jackpotRuntime.forward = false;
                if (kJackpotSegments > 1)
                    g_jackpotRuntime.step = kJackpotSegments - 2;
            }
            else
            {
                ++g_jackpotRuntime.step;
            }
        }
        else
        {
            if (g_jackpotRuntime.step == 0 || kJackpotSegments == 1)
            {
                g_jackpotRuntime.forward = true;
                if (kJackpotSegments > 1)
                    g_jackpotRuntime.step = 1;
            }
            else
            {
                --g_jackpotRuntime.step;
            }
        }
    }

    void StepJackpotAlternatingFill()
    {
        static const CRGB palette[] = { CRGB::DarkOrange, CRGB::Gold, CRGB::Red };
        constexpr size_t paletteSize = sizeof(palette) / sizeof(palette[0]);

        FillJackpotSegment(g_jackpotRuntime.step, palette[g_jackpotRuntime.secondary]);
        ++g_jackpotRuntime.step;

        if (g_jackpotRuntime.step >= kJackpotSegments)
        {
            g_jackpotRuntime.step = 0;
            g_jackpotRuntime.secondary = static_cast<uint8_t>((g_jackpotRuntime.secondary + 1) % paletteSize);
            if (g_jackpotRuntime.secondary == 0)
            {
                ClearJackpotRange();
            }
        }
    }

    void StepJackpotDualChase()
    {
        ClearJackpotRange();
        uint8_t left = g_jackpotRuntime.step;
        uint8_t right = g_jackpotRuntime.secondary;
        if (left < kJackpotLedCount)
            SetJackpotLed(left, CRGB::Cyan);
        if (right < kJackpotLedCount)
            SetJackpotLed(right, CRGB::Magenta);

        if (left >= right || right == 0)
        {
            g_jackpotRuntime.step = 0;
            g_jackpotRuntime.secondary = kJackpotLedCount - 1;
        }
        else
        {
            ++g_jackpotRuntime.step;
            --g_jackpotRuntime.secondary;
        }
    }

    void StepJackpotMeteor()
    {
        constexpr uint8_t meteorSize = 5;
        constexpr uint8_t trailDecay = 70;
        const int totalSteps = kJackpotLedCount + kJackpotLedsPerSegment;

        fadeToBlackBy(g_jackpotFrame, kJackpotLedCount, trailDecay);
        for (uint8_t i = 0; i < meteorSize; ++i)
        {
            int idx = static_cast<int>(g_jackpotRuntime.step) - i;
            if (idx >= 0 && idx < kJackpotLedCount)
            {
                g_jackpotFrame[idx] = CRGB::DeepSkyBlue;
            }
        }
        ++g_jackpotRuntime.step;
        if (g_jackpotRuntime.step >= totalSteps)
        {
            g_jackpotRuntime.step = 0;
        }
    }

    void StepJackpotRainbowSweep()
    {
        for (uint8_t i = 0; i < kJackpotLedCount; ++i)
        {
            g_jackpotFrame[i] = CHSV(g_jackpotRuntime.hueBase + i * 4, 240, 255);
        }
        g_jackpotRuntime.hueBase += 3;
    }

    void StepJackpotSparkle()
    {
        constexpr uint8_t sparkleCount = 5;
        fadeToBlackBy(g_jackpotFrame, kJackpotLedCount, 40);
        for (uint8_t i = 0; i < sparkleCount; ++i)
        {
            g_jackpotFrame[random8(kJackpotLedCount)] += CHSV(random8(), 200, 255);
        }
    }

    void StepJackpotPulse()
    {
        CRGB color = CRGB::Gold;
        color.nscale8_video(GetHeartbeatBrightness(28));
        fill_solid(g_jackpotFrame, kJackpotLedCount, color);
    }

    void StepJackpotPlasma()
    {
        for (uint8_t i = 0; i < kJackpotLedCount; ++i)
        {
            const uint8_t waveA = sin8(g_jackpotRuntime.hueBase + i * 8);
            const uint8_t waveB = sin8(g_jackpotRuntime.step + i * 16);
            const uint8_t blend = qadd8(waveA, waveB) / 2;
            g_jackpotFrame[i] = CHSV(waveA + g_jackpotRuntime.hueBase, 200, blend);
        }

        g_jackpotRuntime.hueBase += 3;
        g_jackpotRuntime.step += 5;
    }

    void StepJackpotDimmedHold()
    {
        if (g_jackpotRuntime.step == 0)
        {
            ApplyJackpotDefaultColors();
            g_jackpotRuntime.step = 1;
        }
    }

    void StepCurrentJackpotMode()
    {
        switch (g_jackpotRuntime.mode)
        {
            case JackpotMode::Classic:
                StepJackpotClassic();
                break;
            case JackpotMode::AlternatingFill:
                StepJackpotAlternatingFill();
                break;
            case JackpotMode::DualChase:
                StepJackpotDualChase();
                break;
            case JackpotMode::Meteor:
                StepJackpotMeteor();
                break;
            case JackpotMode::RainbowSweep:
                StepJackpotRainbowSweep();
                break;
            // case JackpotMode::Sparkle:
            //     StepJackpotSparkle();
            //     break;
            case JackpotMode::Pulse:
                StepJackpotPulse();
                break;
            case JackpotMode::Plasma:
                StepJackpotPlasma();
                break;
            case JackpotMode::DimmedHold:
                StepJackpotDimmedHold();
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

    void UpdateJackpotAnimations()
    {
        const uint32_t now = millis();
        if (g_jackpotRuntime.modeStart == 0)
        {
            ResetJackpotRuntime(g_jackpotRuntime.mode, now);
        }

        if (now - g_jackpotRuntime.modeStart >= g_jackpotRuntime.modeDuration)
        {
            ResetJackpotRuntime(NextJackpotMode(g_jackpotRuntime.mode), now);
        }

        if (now < g_jackpotRuntime.nextFrame)
        {
            return;
        }

        StepCurrentJackpotMode();
        ShowJackpotDimmed();
        g_jackpotRuntime.nextFrame = now + g_jackpotRuntime.frameInterval;
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

    void RenderShuttleLaunch(uint32_t modeElapsed)
    {
        CRGB * segment = GetShuttleSegment();

        if (modeElapsed < kShuttleLaunchRampMs)
        {
            // Ignition ramp: dim red → orange → white-hot
            const uint8_t progress = static_cast<uint8_t>(
                min(255UL, (modeElapsed * 255UL) / kShuttleLaunchRampMs));

            // Hue shifts from deep red (0) to orange (28) to yellow-white
            const uint8_t hue = lerp8by8(0, 28, progress);
            // Saturation drops from full color to near-white at peak
            const uint8_t sat = lerp8by8(255, 80, progress);
            // Brightness ramps up
            const uint8_t val = lerp8by8(40, 255, progress);

            for (uint8_t i = 0; i < kShuttleLedCount; ++i)
            {
                // Slight per-LED variation for organic feel
                segment[i] = CHSV(hue + random8(4), sat, val - random8(15));
            }
        }
        else if (modeElapsed < kShuttleLaunchRampMs + kShuttleLaunchHoldMs)
        {
            // Peak white-hot hold with slight flicker
            for (uint8_t i = 0; i < kShuttleLedCount; ++i)
            {
                const uint8_t flicker = random8(230, 255);
                segment[i] = CRGB(flicker, flicker, lerp8by8(200, flicker, random8()));
            }
        }
        else if (modeElapsed < kShuttleLaunchRampMs + kShuttleLaunchHoldMs + kShuttleLaunchFadeMs)
        {
            // Fade out — shuttle "flies away"
            const uint32_t fadeElapsed = modeElapsed - kShuttleLaunchRampMs - kShuttleLaunchHoldMs;
            const uint8_t fadeProgress = static_cast<uint8_t>(
                min(255UL, (fadeElapsed * 255UL) / kShuttleLaunchFadeMs));
            const uint8_t brightness = lerp8by8(255, 0, fadeProgress);

            for (uint8_t i = 0; i < kShuttleLedCount; ++i)
            {
                CRGB color = CRGB(255, 180, 80);
                color.nscale8_video(brightness);
                segment[i] = color;
            }
        }
        else
        {
            // Dark pause before next cycle
            fill_solid(segment, kShuttleLedCount, CRGB::Black);
        }

        FastLED.show();
        delay(30);
    }

    void RunShuttleMode(ShuttleMode mode, uint32_t modeElapsed = 0)
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
            case ShuttleMode::Launch:
                RenderShuttleLaunch(modeElapsed);
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
            case StreetMode::CarHeadlights:
                RenderCarHeadlights();
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

void FlickerSpotlight(uint8_t index, const CRGB & color)
{
    if (index >= NUM_LEDS1)
        return;

    constexpr uint8_t kFlickerBursts = 6;
    for (uint8_t i = 0; i < kFlickerBursts; ++i)
    {
        leds1[index] = (i % 2 == 0) ? CRGB::Black : color;
        FastLED.show();
        delay(random8(25, 90));
    }

    constexpr uint8_t kRampSteps = 4;
    for (uint8_t step = 0; step < kRampSteps; ++step)
    {
        CRGB ramp = color;
        const uint8_t scale = lerp8by8(30, 255, static_cast<uint8_t>((step * 255) / (kRampSteps - 1)));
        ramp.nscale8_video(scale);
        leds1[index] = ramp;
        FastLED.show();
        delay(65);
    }

    leds1[index] = color;
    FastLED.show();
}

void FlickerSpotlights(uint8_t indexA, uint8_t indexB, const CRGB & color)
{
    if (indexA >= NUM_LEDS1 || indexB >= NUM_LEDS1)
        return;

    constexpr uint8_t kFlickerBursts = 6;
    for (uint8_t i = 0; i < kFlickerBursts; ++i)
    {
        const CRGB level = (i % 2 == 0) ? CRGB::Black : color;
        leds1[indexA] = level;
        leds1[indexB] = level;
        FastLED.show();
        delay(random8(25, 90));
    }

    constexpr uint8_t kRampSteps = 4;
    for (uint8_t step = 0; step < kRampSteps; ++step)
    {
        CRGB ramp = color;
        const uint8_t scale = lerp8by8(30, 255, static_cast<uint8_t>((step * 255) / (kRampSteps - 1)));
        ramp.nscale8_video(scale);
        leds1[indexA] = ramp;
        leds1[indexB] = ramp;
        FastLED.show();
        delay(65);
    }

    leds1[indexA] = color;
    leds1[indexB] = color;
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

void BreathingEyes()
{
    // Slow breathing: brightness oscillates between dim and full
    const uint8_t breath = beatsin8(kEyeBreathBpm, 40, 255);
    // Subtle hue shift: violet (192) ↔ blue-violet (210)
    const uint8_t hue = lerp8by8(192, 210, beatsin8(6, 0, 255));
    CRGB color = CHSV(hue, 200, breath);

    leds0[NUM_LEDS0 -2] = color;
    leds0[NUM_LEDS0 -3] = color;
    leds0[NUM_LEDS0 -4] = color;
    leds0[NUM_LEDS0 -5] = color;
    FastLED.show();
}

void TriggerJackpotCelebration()
{
    g_jackpotCelebrationRequested = true;
}

void TriggerAwakening()
{
    g_awakeningRequested = true;
}

void SetAllStopped(bool stopped)
{
    g_allStopped = stopped;
    if (stopped)
    {
        fill_solid(leds0, NUM_LEDS0, CRGB::Black);
        fill_solid(leds1, NUM_LEDS1, CRGB::Black);
        FastLED.show();
        debugI("All animations stopped, LEDs off");
    }
    else
    {
        debugI("Animations resumed");
    }
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
        if (g_allStopped || g_globalHeartActive || g_showcaseActive || g_awakeningActive)
        {
            PostDrawHandler();
            continue;
        }

        const uint32_t now = millis();
        const uint32_t shuttleModeElapsed = now - lastModeChange;
        const uint32_t shuttleDuration = (currentMode == ShuttleMode::Launch)
            ? kShuttleLaunchTotalMs : kShuttleModeDurationMs;
        if (shuttleModeElapsed >= shuttleDuration)
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

        UpdateMoonPhases();
        UpdateAppleGlow();
        UpdateBrideAnimation();
        UpdateMeteorShower();
        RunStreetMode(currentStreetMode);
        RunShuttleMode(currentMode, now - lastModeChange);
        BreathingEyes();
        PostDrawHandler();
    }
}

// heart
void IRAM_ATTR DrawLoopTaskEntryTwo(void *) 
{
    uint32_t lastAutoAwakening = millis();
    for (;;)
    {
        if (g_allStopped)
        {
            PostDrawHandler();
            continue;
        }

        // Check for awakening trigger (API or auto)
        if (g_awakeningRequested)
        {
            g_awakeningRequested = false;
            RunAwakeningMode();
            lastAutoAwakening = millis();
        }

        // Auto-awakening every 30 minutes
        const uint32_t now = millis();
        if (now - lastAutoAwakening >= kAutoAwakeningIntervalMs)
        {
            lastAutoAwakening = now;
            if (!g_showcaseActive)
            {
                RunAwakeningMode();
            }
        }

        if (!g_allStopped && !g_showcaseActive && !g_awakeningActive)
        {
            Heartbeat(0);
        }
        EVERY_N_SECONDS(kGlobalHeartIntervalSeconds)
        {
            if (!g_allStopped && !g_showcaseActive && !g_awakeningActive)
            {
                RunGlobalHeartMode();
            }
        }
        PostDrawHandler();
    }
}

// jackpot (been)
void IRAM_ATTR DrawLoopTaskEntryThree(void *)
{
    ResetJackpotRuntime(JackpotMode::Classic, millis());
    uint32_t lastAutoJackpot = millis();
    for (;;)
    {
        if (g_allStopped)
        {
            PostDrawHandler();
            continue;
        }

        if (g_jackpotCelebrationRequested)
        {
            g_jackpotCelebrationRequested = false;
            RunJackpotCelebration();
            ResetJackpotRuntime(JackpotMode::Classic, millis());
            lastAutoJackpot = millis();
        }

        // Auto jackpot celebration every 10 minutes
        const uint32_t now = millis();
        if (now - lastAutoJackpot >= kAutoJackpotIntervalMs)
        {
            lastAutoJackpot = now;
            RunJackpotCelebration();
            ResetJackpotRuntime(JackpotMode::Classic, millis());
        }

        if (!g_allStopped && !g_globalHeartActive && !g_showcaseActive && !g_jackpotCelebrationActive && !g_awakeningActive)
        {
            UpdateJackpotAnimations();
        }

        PostDrawHandler();
    }
}

// the machine logo
void IRAM_ATTR DrawLoopTaskEntryFour(void *)
{
    MachineMode currentActiveMode = MachineMode::Showcase;
    MachineMode currentMode = currentActiveMode;
    uint32_t lastModeChange = millis();

    for (;;)
    {
        if (g_allStopped || g_globalHeartActive || g_awakeningActive)
        {
            PostDrawHandler();
            continue;
        }

        const uint32_t now = millis();
        const uint32_t modeDuration = (currentMode == MachineMode::Idle)
            ? kMachineIdleDurationMs : kMachineActiveDurationMs;
        if (now - lastModeChange >= modeDuration)
        {
            lastModeChange = now;
            if (currentMode == MachineMode::Idle)
            {
                currentActiveMode = NextActiveMachineMode(currentActiveMode);
                currentMode = currentActiveMode;
            }
            else
            {
                currentMode = MachineMode::Idle;
            }
            debugI("Switching The Machine mode to %u", static_cast<unsigned>(currentMode));
        }

        RunMachineMode(currentMode);
        PostDrawHandler();
    }
}
