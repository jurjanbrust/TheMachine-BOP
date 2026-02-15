# Bride of Pinbot — LED Topper Display

This document describes the hardware layout, LED zones, animation modes, and architecture of the Bride of Pinbot pinball topper LED display. It is intended as a reference for both Copilot and human editors.

---

## Hardware Overview

- **Microcontroller:** ESP32 (dual-core, with PSRAM)
- **LED type:** WS2812B (GRB order), driven by FastLED
- **LED strips:**
  - **Strip 0 (`leds0`, pin 14):** 53 LEDs — Jackpot segments (8 × 6 = 48 LEDs) + eyes (4 LEDs) + heart (1 LED). Referred to as "been" (legs).
  - **Strip 1 (`leds1`, pin 12):** 121 LEDs — All backglass/topper artwork elements. Referred to as "overig" (other).
- **Default brightness:** 100 (saved to NVS flash via `Preferences`, adjustable at runtime through the HTTP API).
- **Connectivity:** WiFi, OTA updates, RemoteDebug telnet, HTTP API on port 80.

---

### Strip 1 Physical LED Layout (121 LEDs)

One continuous WS2812B strip that starts at index 0 (top-left) and snakes clockwise inward in three loops to index 120 (center).

(strip 1)
0--------------------------18 
                            |
61--------------------78    |
|                     |     |
|                     |     |
|                     |     |
|  105--------120     |     |
|  |                  |     |
|  |                  |     | 
|  99 --------------- 85    |
|                           |
51-------------------------31


**Path summary** (one continuous strip, always clockwise):
- **Loop 1 (0–50):** 0→18 top L→R, 19→31 right T→B, 32→50 bottom R→L
- **→ snakes inward →**
- **Loop 2 (51–85):** 51→61 left B→T, 62→78 top L→R, 79→85 right T→B
- **→ snakes inward →**
- **Loop 3 (86–120):** 86→100 bottom R→L, 101→105 left B→T, 106→111 top L→R, 112→113 right T→B, 114→120 inner row L→R

The strip is a single continuous snake — there are no breaks or separate rings. Each loop transitions seamlessly into the next by turning inward at the bottom-left corner.

---

## LED Zones & Named Positions (Strip 1)

These are the individually-addressable artwork elements on the backglass:

| Name | Index/Range | Description |
|---|---|---|
| **Moon** | 2–4 (`moonTopLeft`, `moonTopLeft+1`, `moonTopLeft+2`) | Three LEDs for the crescent moon (top-left area) |
| **Front of Head** | 4 (`fronthead`) | Accent LED on the bride's forehead; pulses during Showcase mode |
| **The Bride** | 3, 5, 6, 62–69, 79, 88, 94–98, 103–113, 115–118 | Scattered LEDs forming the bride figure outline |
| **The Machine Logo** | 8–17 (`theMachineFirstLed`–`theMachineLastLed`, 10 LEDs) | "The Machine" title text |
| **Spotlights** | 29 (`spotlights2`), 85 (`spotlights1`) | Two spotlights that flicker on during Showcase |
| **People / Street** | 38 (`people`) | City street people element |
| **Cars** | 39–42 (`carright1`, `carright2`, `carleft1`, `carleft2`) | Four LEDs for cars in the street scene |
| **Fingers Left Corner** | 50 (`fingersLeftCorner`) | Bride's fingers, left side |
| **Shuttle** | 55–57 (`kShuttleFirstLed`, 3 LEDs) | Space shuttle flame/exhaust |
| **Big Blue Planet** | 73 (`bigBluePlanetLeftSide`), 74 (`bigBluePlanetRightSide`) | Two LEDs for the large blue planet |
| **Apple** | 81 (`apple`) | Apple of knowledge element |
| **Jupiter** | 83 (`jupiterUpper`), 84 (`jupiterLower`) | Two LEDs for Jupiter |

### Strip 0 Layout (from end)

| Name | Offset from End | Description |
|---|---|---|
| **Heart** | `NUM_LEDS0 - 1` (last LED) | Single red heartbeat LED |
| **Eyes** | `NUM_LEDS0 - 2` through `NUM_LEDS0 - 5` (4 LEDs) | Bride's eyes, default BlueViolet |
| **Jackpot** | 0–47 (first 48 LEDs) | 8 segments × 6 LEDs each |

---

## Animation Threads

The display runs four concurrent FreeRTOS tasks on core 1, plus a heartbeat task:

| Task | Function | What It Drives |
|---|---|---|
| **Shuttle** | `DrawLoopTaskEntryOne` | Shuttle flames, planet sparkles, street scene |
| **Heart** | `DrawLoopTaskEntryTwo` | Heartbeat LED + periodic global heart mode |
| **Jackpot** | `DrawLoopTaskEntryThree` | Jackpot ring animations (strip 0) |
| **TheMachine** | `DrawLoopTaskEntryFour` | "The Machine" logo animations |

---

## Mode Descriptions

### The Machine Logo Modes (`MachineMode`)

Cycles through active modes, with an **Idle** rest period between each. Each active mode runs for 30 seconds, then Idle for 2 minutes, then the next active mode.

| Mode | Description |
|---|---|
| **Rainbow** | Scrolling rainbow hue across the 10 logo LEDs (base hue at 12 BPM) |
| **Pulse** | All logo LEDs pulse DeepPink using a heartbeat brightness curve (30 BPM) |
| **Sparkle** | Random white sparkle flashes on the logo with fade-to-black decay |
| **Scanner** | Single red LED bounces back and forth (Larson scanner / Knight Rider style) |
| **Comet** | Bright white pixel sweeps across the 10 logo LEDs leaving a warm-white fading trail (shooting star effect) |
| **Showcase** | Multi-stage theatrical sequence: blacks out all LEDs, then spotlights flicker like fluorescent tubes turning on for 10 seconds while everything else stays dark. All other animation tasks (shuttle, heartbeat, jackpot) pause during the flicker via the `g_showcaseActive` flag. Once the spotlights are permanently lit, the logo ramps up in warm white (246,200,160) and planets fade in to their base colors. During hold, spotlights slowly wash through warm tones (white → warm white → soft amber → back). 3 stages. |
| **Idle** | Solid warm white (246,200,160) fill — rest state between active modes |

**Rotation order:** Rainbow → Idle → Pulse → Idle → Sparkle → Idle → Scanner → Idle → Comet → Idle → Showcase → Idle → (repeat)

---

### Jackpot Ring Modes (`JackpotMode`)

The jackpot ring (48 LEDs on strip 0) cycles through animation modes. Most modes run for 15 seconds; DimmedHold runs for 60 seconds. Output is dimmed (scaled to 80/255) when `dimOutput` is true or during Showcase.

| Mode | Interval | Description |
|---|---|---|
| **Classic** | 220 ms | Single red-lit segment bounces back and forth across all 8 segments |
| **AlternatingFill** | 180 ms | Segments fill one at a time cycling through DarkOrange → Gold → Red, then clear and repeat |
| **DualChase** | 140 ms | Two LEDs (Cyan and Magenta) chase toward each other from opposite ends |
| **Meteor** | 90 ms | A 5-LED DeepSkyBlue meteor with fading trail sweeps across the ring |
| **RainbowSweep** | 120 ms | Continuous rainbow gradient scrolling around the ring |
| **Sparkle** | 110 ms | *(Currently commented out in code)* Random colorful sparkle bursts with fade |
| **Pulse** | 100 ms | All LEDs pulse Gold using heartbeat curve (28 BPM) |
| **Plasma** | 90 ms | Dual overlapping sine waves creating shifting plasma-like color patterns |
| **DimmedHold** | 1000 ms | Static hold: first 4 segments DarkOrange, last 4 segments Red (60-second duration) |

**Rotation order:** Classic → AlternatingFill → DualChase → Meteor → RainbowSweep → (Sparkle skipped) → Pulse → Plasma → DimmedHold → (repeat)

---

### Shuttle Flame Modes (`ShuttleMode`)

Three LEDs (indices 55–57) simulate the shuttle's engine exhaust. Each mode runs for 15 seconds, except Launch which runs for ~11.5 seconds.

| Mode | Description |
|---|---|
| **Flicker** | Randomized warm-orange flame flicker (hue 10–18, high saturation, random brightness 160–255) |
| **Wave** | Smooth sinusoidal color wave with warm orange tones flowing across the 3 LEDs |
| **Boost** | Pulsing blend from white to orange simulating engine boost (18 BPM) |
| **Launch** | Simulated launch sequence: 5-second ignition ramp from dim red through orange to white-hot, 1.5-second peak hold with flicker, 2-second fade-out as shuttle "flies away", then 3-second dark pause before repeating |

**Rotation order:** Flicker → Wave → Boost → Launch → (repeat)

---

### Street Scene Modes (`StreetMode`)

The 5 street LEDs (people + 4 cars) cycle through modes every 12 seconds.

| Mode | Description |
|---|---|
| **Pulse** | All 5 street LEDs pulse white in unison (24 BPM sine wave) |
| **Runner** | *(Defined in enum but no dedicated render function — falls through to default)* |
| **Sparkle** | Random colorful sparkle bursts with per-LED fade decay on the street elements |
| **CarHeadlights** | Cars alternate left/right pairs in warm yellow (255,200,60) like passing traffic, with the opposite pair dimmed. People LED breathes gently alongside |

**Rotation order:** Pulse → Runner → Sparkle → CarHeadlights → (repeat)

---

### Planet Sparkles

The 5 planet LEDs (moon, blue planet left/right, Jupiter upper/lower) receive continuous white sparkle overlays every 150 ms. One random planet gets a white highlight each frame, all decay rapidly (220/255 fade). Base colors are:

- **Moon:** AntiqueWhite
- **Big Blue Planet (both):** DeepSkyBlue
- **Jupiter (both):** OrangeRed

---

### Jackpot Win Celebration

Triggered via HTTP API (`/jackpot`) or automatically every **10 minutes** (`kAutoJackpotIntervalMs = 600000`). Overrides normal jackpot animation for 5 seconds:

1. **Rainbow Flash (2s):** Rapid rainbow hue cycling across all 48 jackpot LEDs at 120 BPM
2. **Golden Cascade (3s):** Segments fill one by one with Gold, random white sparkles flash on filled LEDs then blend back to gold

After completion, the jackpot ring resets to Classic mode and resumes normal rotation.

---

### Bride Animation Modes (`BrideMode`)

The 33 bride-outline LEDs (scattered across strip 1) alternate between two animation modes, each running for 20 seconds. Driven by `UpdateBrideAnimation()` in the shuttle task loop.

| Mode | Description |
|---|---|
| **Aurora** | Northern lights effect using overlapping sine waves. Hues shift through green-teal-purple range with organic flowing brightness. Creates a living, breathing silhouette. |
| **Starfield** | Subtle night-sky effect: bride LEDs stay very dim (4), sparse random LEDs gently twinkle up to soft white (peak 140) then slowly fade back with a long decay. ~12% spark chance per frame for a calm, understated look. |

**Rotation order:** Aurora → Starfield → (repeat)

---

### Moon Phases

The 3 moon LEDs (indices 2–4) cycle through simulated moon phases. Each phase lasts 15 seconds. The cycle is: full (3 lit, cool white) → gibbous (2 lit) → crescent (1 lit, warm gold) → new (0 lit) → crescent → gibbous → full. 7 phases total before repeating. Driven continuously from DrawLoopTaskEntryOne.

---

### Reactive Apple Glow

The apple LED (index 81) slowly pulses between green and red at 6 BPM, creating a "forbidden fruit" temptation effect. Runs continuously from DrawLoopTaskEntryOne.

---

### Meteor Shower

Periodically (every 20 minutes), a bright meteor streak crosses strip 1. The meteor is 6 LEDs long with a fading trail (decay 64/255 per frame), moving at 2 LEDs per frame. Each meteor spawns with a random hue at low saturation for a near-white shooting-star look. The trail is blended additively onto strip 1 so it overlays other animations. Driven from DrawLoopTaskEntryOne.

---

### Spotlight Color Wash

During Showcase hold (stage 2), the two spotlights slowly cycle through warm tones at 6 BPM: pure white → warm white → soft amber → back. Creates a theatrical gel-filter effect. Uses `GetSpotlightWashColor()` instead of static white.

---

### Global Heart Mode

Every **5 minutes** (`kGlobalHeartIntervalSeconds = 300`), a global heartbeat takes over **both strips**:

1. **Cross-fade transition (2s):** Snapshots the current LED state and smoothly blends both strips from their current colors toward Red (strip 0) / BlueViolet (strip 1) over 2 seconds.
2. **Heartbeat pulse (15s):** Both strips pulse using the heartbeat brightness curve — strip 0 Red, strip 1 BlueViolet.
- All other animations pause during this event (checked via `g_globalHeartActive` flag)

---

### Persistent Heart LED

The single heart LED (last LED on strip 0) beats Red continuously using the heartbeat lookup table, independent of all other animations.

### Eyes

Four LEDs near the end of strip 0 — initialized to BlueViolet at startup, then continuously animated with a breathing effect: brightness oscillates slowly at 10 BPM between dim (40) and full (255), with a subtle hue shift between violet and blue-violet at 6 BPM. Driven by `BreathingEyes()` called from the shuttle task loop (DrawLoopTaskEntryOne).

---

### Awakening Mode

Triggered via HTTP API (`/awakening`) or automatically every **30 minutes** (`kAutoAwakeningIntervalMs = 1800000`). A 1-minute theatrical sequence where the bride comes alive. All other animations pause during this event (checked via `g_awakeningActive` flag). All LEDs start dark, then elements light up in stages:

| Time | Phase | Description |
|---|---|---|
| 0–10s | **Eyes Open** | Four eye LEDs slowly fade from black to BlueViolet, like the bride waking up |
| 10–20s | **Heart Starts** | Eyes hold steady at full BlueViolet. Heart LED starts with faint beats that grow stronger over 10 seconds |
| 20–30s | **Moon & Bride** | Moon fades in (crescent → full). Bride outline begins dim aurora. Fronthead accent pulses |
| 30–40s | **Planets & Apple** | All 5 planet LEDs fade to base colors. Apple begins green↔red glow. Finger accent appears |
| 40–50s | **Logo, Shuttle, Jackpot** | Machine logo ramps warm white. Shuttle flames flicker to life. Jackpot segments fill outward from center in warm orange/red |
| 50–58s | **Street & Spotlights** | Street and car headlights fade in. Spotlights flicker on with fluorescent tube effect for the full 8 seconds (50–58s), probability of being on increases over time, then lock solid |
| 58–60s | **Hold & Release** | All elements hold at full brightness, then normal animation resumes |

---

## Static Startup State

On boot, both strips start fully dark (black). The Machine logo task begins in **Showcase** mode, which runs the 10-second fluorescent tube flicker sequence as the theatrical power-on reveal. After the flicker completes, the spotlights lock on and the logo and planets ramp up. Once Showcase finishes, the normal mode rotation continues (Idle → Rainbow → ...).

---

## HTTP API

| Endpoint | Method | Params | Description |
|---|---|---|---|
| `/setled` | GET | `index` (0–120) | Clears strip 1, then sets the specified LED to white |
| `/setbrightness` | GET | `value` (0–255) | Sets global brightness and persists to NVS flash |
| `/jackpot` | GET | *(none)* | Triggers a 5-second jackpot win celebration on the jackpot ring |
| `/awakening` | GET | *(none)* | Triggers the 1-minute Awakening sequence — bride comes alive |

---

## Build & Configuration

- **Framework:** Arduino (PlatformIO)
- **WiFi credentials:** defined in `include/secrets.h` (see `secrets.example.h` for template)
- **Feature flags** (in `globals.h`): `ENABLE_OTA`, `ENABLE_WIFI`, `ENABLE_WEBSERVER` — all enabled by default

---

## Editing This File

You can modify this file to:
- Add or rename animation modes
- Change timing constants or color preferences
- Document new LED zones or hardware changes
- Add notes about planned features or known issues

Copilot will use this file as context for future conversations about this project.

## copilot instructions
Do not give a summarizing conversation history
