#include "globals.h"
#include "drawing.h"
#include <ArduinoOTA.h>             // Over-the-air helper object so we can be flashed via WiFi

// The g_buffer_mutex is a global mutex used to protect access while adding or removing frames
// from the led buffer.  

extern uint32_t           g_FPS;
extern bool               g_bUpdateStarted;

namespace
{
    constexpr uint16_t kGlobalHeartIntervalSeconds = 600;   // 10 minutes
    constexpr uint32_t kGlobalHeartDurationMs      = 15000;  // run heartbeat for 15s
    constexpr uint32_t kMachineModeDurationMs      = 60000;  // rotate every minute
    constexpr uint8_t  kMachineLedCount            = theMachineLastLed - theMachineFirstLed + 1;
    constexpr uint8_t  kJackpotSegments            = 8;
    constexpr uint8_t  kJackpotLedsPerSegment      = 6;
    constexpr uint8_t  kJackpotLedCount            = kJackpotSegments * kJackpotLedsPerSegment;
    constexpr uint8_t  kPlanetCount                = 5;
    constexpr uint16_t kPlanetSparkleIntervalMs    = 150;
    constexpr uint8_t  kPlanetFadeAmount           = 220;

    enum class MachineMode : uint8_t
    {
        Rainbow = 0,
        Pulse,
        Sparkle,
        Scanner,
        Count
    };

    enum class JackpotMode : uint8_t
    {
        Classic = 0,
        AlternatingFill,
        DualChase,
        Meteor,
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

    void RunMachineMode(MachineMode mode)
    {
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
            FastLED.show();
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
            FastLED.show();
            delay(300);

            for (uint8_t i = 0; i < kJackpotLedsPerSegment; ++i)
            {
                leds0[current + i] = CRGB::Black;
            }
        }

        constexpr uint8_t kPlanetIndices[kPlanetCount] = {
            moonTopLeft,
            bigBluePlanetLeftSide,
            bigBluePlanetRightSide,
            jupiterUpper,
            jupiterLower
        };

        static const CRGB kPlanetBaseColors[kPlanetCount] = {
            CRGB::AntiqueWhite,
            CRGB::DeepSkyBlue,
            CRGB::DeepSkyBlue,
            CRGB::OrangeRed,
            CRGB::OrangeRed
        };

        void UpdatePlanetSparkles()
        {
            for (uint8_t i = 0; i < kPlanetCount; ++i)
            {
                const uint8_t idx = kPlanetIndices[i];
                leds1[idx].nscale8_video(kPlanetFadeAmount);
                leds1[idx] += kPlanetBaseColors[i];
            }

            const uint8_t sparkleIdx = random8(kPlanetCount);
            leds1[kPlanetIndices[sparkleIdx]] += CRGB::White;
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
                FastLED.show();
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
            FastLED.show();
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

            FastLED.show();
            delay(40);
        }
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
    FastLED.show();
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
    int start = 55;
    int end = start + 2;
    int delayTime = 70;
    for (;;)
    {
        for(int i = start; i<=end; i++) {
            leds1[i] = CRGB::Red;
        }
        FastLED.show(); delay(delayTime);

         for(int i = start; i<=end; i++) {
            leds1[i] = CRGB::Brown;
        }
        FastLED.show(); delay(delayTime);

        EVERY_N_MILLIS(kPlanetSparkleIntervalMs)
        {
            UpdatePlanetSparkles();
            FastLED.show();
        }

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
        for(int i=0; i<5; i++) {
            DrawWalkingDot();
            delay(1000*1);
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
