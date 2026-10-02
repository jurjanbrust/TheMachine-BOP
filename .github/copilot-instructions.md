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
- **Power budget:** 3000 mA at 5 V, enforced by FastLED. Adjust
  `kLedPowerLimitMilliamps` only after checking the installed power supply and
  wiring.
- **Connectivity:** WiFi, OTA updates, RemoteDebug telnet, HTTP API on port 80.
  WiFi reconnects in a background task with exponential backoff; LED startup
  never waits for the network.

---

### Strip 1 Physical LED Layout (121 LEDs)

One continuous WS2812B strip that starts at index 0 (top-left) and snakes clockwise inward in three loops to index 120 (center).

**Canonical layout reference:** See [`docs/LED-LAYOUT.md`](../docs/LED-LAYOUT.md).
It combines the authoritative `leds.xlsx` index grid with the player-facing
photo `IMG_2055.JPG`, the mirrored rear/inside photo `IMG_2054.JPG`, and the
runtime `kLedCoords` mapping. For all effects, left/right and `(x,y)` directions
are defined from the player-facing artwork view. Use coordinates for spatial
effects and raw index order only when an effect should follow the physical wire.

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

Animation logic runs in four FreeRTOS tasks on core 1. These tasks update the
composition buffers and call `PublishLedFrame()`. One higher-priority
`LedRenderTaskEntry` owns the physical FastLED buffers and is the only code
allowed to call `FastLED.show()`, capped at roughly 60 FPS.

| Task | Function | What It Drives |
|---|---|---|
| **Shuttle** | `DrawLoopTaskEntryOne` | Shuttle flames, planet sparkles, street scene |
| **Heart** | `DrawLoopTaskEntryTwo` | Heartbeat LED + periodic global heart mode |
| **Jackpot** | `DrawLoopTaskEntryThree` | Jackpot ring animations (strip 0) |
| **TheMachine** | `DrawLoopTaskEntryFour` | "The Machine" logo animations |
| **LED Render** | `LedRenderTaskEntry` | Publishes coherent frames to both physical strips |

Long-running scenes check `g_sceneCancellationRequested` each frame. A new
manual scene or `stop` therefore interrupts the current scene without waiting
for its full duration.

---

## Mode Descriptions

### The Machine Logo Modes (`MachineMode`)

Cycles through active modes, with an **Idle** rest period between each. Each active mode runs for 60 seconds, then Idle for 2 minutes, then the next active mode.

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

The jackpot ring (48 LEDs on strip 0) cycles through animation modes. Most modes run for 43 seconds; DimmedHold runs for 90 seconds. Output is dimmed (scaled to 80/255) when `dimOutput` is true or during Showcase.
Artwork segments are addressed in visible order 1→8 using
`kJackpotVisualToPhysical = {7,6,5,4,3,2,1,0}`; do not assume physical strip
order matches the numbered ladder.

| Mode | Interval | Description |
|---|---|---|
| **Classic** | 500 ms | Single red-lit segment bounces back and forth across all 8 segments |
| **AlternatingFill** | 450 ms | Segments fill one at a time cycling through DarkOrange → Gold → Red, then clear and repeat |
| **DualChase** | 400 ms | Two LEDs (Cyan and Magenta) chase toward each other from opposite ends |
| **Meteor** | 300 ms | A 5-LED DeepSkyBlue meteor with fading trail sweeps across the ring |
| **RainbowSweep** | 160 ms | Continuous rainbow gradient scrolling around the ring |
| **Sparkle** | 110 ms | *(Currently commented out in code)* Random colorful sparkle bursts with fade |
| **Pulse** | 100 ms | All LEDs pulse Gold using heartbeat curve (28 BPM) |
| **Plasma** | 140 ms | Dual overlapping sine waves creating shifting plasma-like color patterns |
| **DimmedHold** | 1000 ms | Static hold: first 4 segments DarkOrange, last 4 segments Red (60-second duration) |

**Rotation order:** Classic → AlternatingFill → DualChase → Meteor → RainbowSweep → (Sparkle skipped) → Pulse → Plasma → DimmedHold → (repeat)

The central LED renderer interpolates every change on jackpot LEDs 0–47 over
roughly 300–350 ms. This applies to normal modes, celebrations, manual effects
and openings, while eyes and heart remain immediately responsive.

---

### Shuttle Flame Modes (`ShuttleMode`)

Three LEDs (indices 55–57) simulate the shuttle's engine exhaust. Each mode runs for 37 seconds. Launch uses a deliberately slow 15-second ignition, 3-second hold, 7-second fade and 12-second dark pause.

| Mode | Description |
|---|---|
| **Flicker** | Randomized warm-orange flame flicker (hue 10–18, high saturation, random brightness 160–255) |
| **Wave** | Smooth sinusoidal color wave with warm orange tones flowing across the 3 LEDs |
| **Boost** | Pulsing blend from white to orange simulating engine boost (18 BPM) |
| **Launch** | Slow launch sequence: 15-second ignition ramp from dim red through orange to white-hot, 3-second peak hold with flicker, 7-second fade-out as the shuttle flies away, then a 12-second dark pause |

**Rotation order:** Flicker → Wave → Boost → Launch → (repeat)

---

### Street Scene Modes (`StreetMode`)

The 5 street LEDs (people + 4 cars) cycle through modes every 29 seconds.
All street modes use continuous low-frequency interpolation. They intentionally
contain no random sparkles, discrete runners, flashes or on/off blinking.

| Mode | Description |
|---|---|
| **EveningGlow** | People transition slowly between cool dusk and warm ambient light; all four cars share a soft amber breath |
| **PassingTraffic** | Left and right car pairs exchange warm headlight intensity over a 20-second sine cycle while people remain softly lit |
| **CityBreath** | A very slow continuous dusk-to-amber wave moves across people and cars with small phase offsets |
| **QuietNight** | Dim blue-violet people silhouette with gently breathing red and amber parked-car lights |

**Rotation order:** EveningGlow → PassingTraffic → CityBreath → QuietNight → (repeat)

---

### Planet Sparkles

The 5 planet LEDs (moon, blue planet left/right, Jupiter upper/lower) receive continuous white sparkle overlays every 150 ms. One random planet gets a white highlight each frame, all decay rapidly (220/255 fade). Base colors are:

- **Moon:** AntiqueWhite
- **Big Blue Planet (both):** DeepSkyBlue
- **Jupiter (both):** OrangeRed

---

### Jackpot Win Celebration

Triggered via HTTP API (`/jackpot`) or automatically via the random queue scheduler. Overrides normal jackpot animation for about 7 seconds:

1. **Rainbow Flash (2s):** Rapid rainbow hue cycling across all 48 jackpot LEDs at 120 BPM
2. **Golden Cascade (4.8s):** Segments fill one by one with Gold, random white sparkles flash on filled LEDs then blend back to gold

After completion, the jackpot ring resets to Classic mode and resumes normal rotation.

---

### Bride Animation Modes (`BrideMode`)

The 33 bride-outline LEDs (scattered across strip 1) alternate between two animation modes, each running for 53 seconds. Driven by `UpdateBrideAnimation()` in the shuttle task loop.

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

### Radial Pulse

Triggered via HTTP API (`/radialpulse`) or automatically via the random queue scheduler. A 3-second sonar-like ripple that expands from the center of the 19×15 grid outward. Three concentric rings with a subtle rainbow tint emanate outward, each staggered by 0.25 phase. When HTTP-triggered, pauses all animations and settles on semantic artwork colors. When auto-triggered, smoothly cross-fades from the current animation state to black (500ms), plays the ripple, then cross-fades back to the live animation state (1s).

### Auto Sweep

Triggered automatically via the random queue scheduler. A random sweep direction (from diagonal TL→BR, TR→BL, BR→TL, T→B, or B→T) washes warm white across the entire backglass. After a 1-second hold, the display cross-fades back to the live animation state (1.5s).

---

### Awakening Mode

Triggered via HTTP API (`/awakening`) or automatically via the random queue scheduler. A 1-minute theatrical sequence where the bride comes alive. All other animations pause during this event (checked via `g_awakeningActive` flag). All LEDs start dark, then elements light up in stages:

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

## Random Queue Scheduler

Instead of fixed-interval timers, all auto-triggered special modes are managed by a centralized random queue scheduler running on Task 1. The pool of modes is:

**Radial Pulse, Sweep, Jackpot Celebration, Awakening, Plasma, Lightning
Storm, Multiball, Spotlight Cone, Spatial Meteor, Crimson Takeover** (10 total)

**How it works:**
1. A shuffled queue of all 10 modes is built (Fisher-Yates shuffle). Each mode plays once before any repeats.
2. After each mode finishes, a random cooldown of **5–10 minutes** (`kSchedulerCooldownMinMs`–`kSchedulerCooldownMaxMs`) elapses before the next one fires.
3. When the queue is exhausted, it reshuffles and starts over — ensuring variety.
4. A **2-minute startup delay** (`kSchedulerStartupDelayMs`) prevents modes from firing immediately after boot.
5. Jackpot and Awakening are triggered via their `Requested` flags so they still execute on their own FreeRTOS tasks (Task 3 and Task 2 respectively).
6. Strip-1 effects use a generic auto-wrapper that pauses competing tasks,
   snapshots the current state, cross-fades to black, runs the effect, then
   cross-fades back.
7. Rain and Breathing Grid remain available manually but are intentionally
   excluded from automatic rotation.
8. HTTP-triggered modes still work independently at any time.

Generic automatically scheduled spatial effects run for 15 seconds. Normal
Street, Shuttle and Bride mode changes cross-fade their own artwork zones over
750 ms. Their 29/37/53-second durations are intentionally staggered so changes
rarely occur together. Machine modes run for 60 seconds and jackpot modes for
43 seconds.

---

## Latest Hardware Ratings

Scores are from player-facing evaluation on the physical topper. Prefer
high-rated scenes for automatic scheduling; keep lower-rated scenes manual.

| Special scene | Score | Opening scene | Score |
|---|---:|---|---:|
| Quantum Vortex | 7 | Improved Showcase | 6 |
| Lightning Storm | 9 | Cosmic Alignment | 8 |
| Neon Rings | 6 | Bride Assembly | 8 |
| Artwork Story | 10 | Launch Control | 8 |
| Fireworks | 5 | City Awakening | 6 |
| Laser Matrix | 8 | System Diagnostics | 7 |
| Ghost Bride | 7 | Stellar Transmission | 8 |
| Multiball | 9 | Pulse of Life | 9 |
| Solar Eclipse | 7 | Moonlight Reveal | 9 |
| Prism Shatter | 6 | | |

Crimson Takeover has not yet received a hardware score.

---

## Static Startup State

On boot, both strips start fully dark and `PrepareRandomStartupOpening()` uses
the ESP32 hardware RNG (`esp_random()`) to select one of all 9 opening scenes.
The selected scene owns both strips while it runs and automatically releases
them to normal animation afterward. The choice is logged as
`[BOOT] randomly selected opening N of 9`.

The opening pool is: Improved Fluorescent Showcase, Cosmic Alignment, Bride
Assembly, Launch Control, City Awakening, System Diagnostics, Stellar
Transmission, Pulse of Life and Moonlight Reveal. All openings can also be
replayed using serial commands `11` through `19` or their HTTP endpoints.
USB serial also accepts `jackpot`, `stop`, `resume`, and `status`.

---

## HTTP API

| Endpoint | Method | Params | Description |
|---|---|---|---|
| `/setled` | GET | `index` (0–120) | Clears strip 1, then sets the specified LED to white |
| `/setbrightness` | GET | `value` (0–255) | Sets global brightness and persists to NVS flash |
| `/jackpot` | GET | *(none)* | Triggers a roughly 7-second jackpot win celebration on the jackpot ring |
| `/awakening` | GET | *(none)* | Triggers the 1-minute Awakening sequence — bride comes alive |
| `/stop` | GET | *(none)* | Stops all animations, turns off all LEDs |
| `/resume` | GET | *(none)* | Resumes normal animation after stop |
| `/sweep` | GET | `dir` (0–8) | Spatial sweep fill: 0=L→R, 1=R→L, 2=T→B, 3=B→T, 4=outer→inner, 5=inner→outer, 6=diag TL→BR, 7=diag TR→BL, 8=diag BR→TL |
| `/radialpulse` | GET | *(none)* | Sonar-like ripple expanding from center of grid outward (3 concentric rings with rainbow tint) |
| `/plasma` | GET | *(none)* | 10-second spatial plasma / lava lamp effect using 2D sine waves across the grid |
| `/rain` | GET | *(none)* | 10-second rain effect — drops of cyan light fall down random columns with fading trails |
| `/breathinggrid` | GET | *(none)* | 10-second diagonal breathing wave — all LEDs breathe with spatial phase offset creating a rolling brightness wave |
| `/spotlightcone` | GET | *(none)* | 10-second spotlight cone effect — two spotlights cast pulsing light cones (warm amber + cool white) across the panel |
| `/spatialmeteor` | GET | *(none)* | 10-second spatial meteor shower — up to 5 meteors travel at diagonal angles across the grid with fading trails |
| `/vortex` | GET | *(none)* | 10-second quantum vortex — rotating spiral arms collapse into a pulsing white core |
| `/lightning` | GET | *(none)* | 10-second lightning storm — randomized jagged bolts, electric-blue afterglow and sky flashes |
| `/neonrings` | GET | *(none)* | 10-second neon rings — the outer, middle and inner physical strip loops counter-rotate independently |
| `/artworkstory` | GET | *(none)* | 12-second semantic reveal — moon, shuttle, bride, eyes, planets, logo, street, jackpot and heart tell a staged story |
| `/fireworks` | GET | *(none)* | 10-second fireworks show — launches rise from the horizon and burst spatially across the artwork |
| `/lasergrid` | GET | *(none)* | 10-second laser matrix — cyan and magenta scanner beams cross with bright white intersections |
| `/ghostbride` | GET | *(none)* | 10-second spectral bride — ectoplasm travels through the bride outline while her eyes and heart glow |
| `/multiball` | GET | *(none)* | 10-second multiball simulation — five colored particles bounce through the spatial layout with fading trails |
| `/eclipse` | GET | *(none)* | 10-second solar eclipse — a dark disc and warm corona travel across a dim star field |
| `/prismshatter` | GET | *(none)* | 10-second prism shatter — rotating stained-glass facets and white fracture lines burst from the center |
| `/crimsontakeover` | GET | *(none)* | 9-second full-display crimson double heartbeat, blackout and golden artwork release |
| `/opening-showcase` | GET | *(none)* | Replay the improved fluorescent opening with independent warm/cool spotlights and a chromatic artwork reveal |
| `/opening-cosmic` | GET | *(none)* | 12-second Cosmic Alignment opening — stars, orbiting energy, planets, title and bride align |
| `/opening-bride` | GET | *(none)* | 12-second Bride Assembly opening — body, heart, eyes, title and forehead power up in stages |
| `/opening-launch` | GET | *(none)* | 12-second Launch Control opening — jackpot countdown, shuttle ignition, launch and shockwave |
| `/opening-city` | GET | *(none)* | 12-second City Awakening opening — sunrise, traffic, dual-color spotlights and title reveal |
| `/opening-diagnostics` | GET | *(none)* | 12-second System Diagnostics opening — RGB test, loop scan and subsystem confirmation |
| `/opening-transmission` | GET | *(none)* | 12-second Stellar Transmission opening — star field, scanning signal, planet lock and decoded title |
| `/opening-pulse` | GET | *(none)* | 12-second Pulse of Life opening — double heartbeat pulses across both strips before the bride and title awaken |
| `/opening-moonlight` | GET | *(none)* | 12-second Moonlight Reveal opening — moonbeam uncovers the bride, silver title and eyes |
| `/status` | GET | *(none)* | JSON diagnostics: animation state, opening, scheduler, brightness, frame count, power budget, WiFi and heap |

---

## Build & Configuration

- **Framework:** Arduino (PlatformIO)
- **WiFi credentials:** defined in `include/secrets.h` (see `secrets.example.h` for template)
- **Feature flags** (in `globals.h`): `ENABLE_OTA`, `ENABLE_WIFI`, `ENABLE_WEBSERVER` — all enabled by default
- **Layout tests:** `python3 -m unittest discover -s tests -v`

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
