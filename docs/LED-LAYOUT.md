# Bride of Pinbot LED layout reference

This is the canonical spatial reference for effects on `leds1`. It combines:

- `leds.xlsx`: authoritative LED index-to-grid mapping.
- `IMG_2055.JPG`: player-facing artwork and visual orientation.
- `IMG_2054.JPG`: physical strip routing and electronics viewed from the rear.
- `BuildCoord()` and `kLedCoords` in `src/drawing.cpp`: runtime representation.

## Orientation

All directions in code and documentation are from the player's view of the
artwork (`IMG_2055.JPG`):

- `(0, 0)` is the top-left corner near the moon.
- `(18, 0)` is the top-right corner above the end of "MACHINE".
- `(0, 14)` is the bottom-left corner near the bride's hand and wine glass.
- `(18, 14)` is the bottom-right corner near the Williams sign.
- `x` increases from left to right; `y` increases from top to bottom.

`IMG_2054.JPG` shows the inside/rear of the topper. Use it to inspect wiring and
strip continuity, but remember that its left/right orientation is mirrored
relative to the player-facing coordinate system.

## Strip 1 index map

The following 19-column by 15-row map is transcribed from `leds.xlsx`. A dot is
a grid location without an LED. The coordinates are logical spatial positions,
not exact pixel or distance measurements.

```text
y\x  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
 0   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18
 1   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  19
 2   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  20
 3  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  21
 4  60   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  79  22
 5  59   . 105 106 107 108 109 110 111   .   .   .   .   .   .   .   .  80  23
 6  58   . 104   .   .   .   .   . 112   .   .   .   .   .   .   .   .  81  24
 7  57   . 103   .   .   .   .   . 113 114 115 116 117 118 119 120   .  82  25
 8  56   . 102   .   .   .   .   .   .   .   .   .   .   .   .   .   .  83  26
 9  55   . 101   .   .   .   .   .   .   .   .   .   .   .   .   .   .  84  27
10  54   . 100  99  98  97  96  95  94  93  92  91  90  89  88  87  86  85  28
11  53   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  29
12  52   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  30
13  51   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .  31
14  50  49  48  47  46  45  44  43  42  41  40  39  38  37  36  35  34  33  32
```

The strip is one continuous clockwise inward path:

1. Outer loop: `0-18` top left-to-right, `19-31` right top-to-bottom,
   `32-50` bottom right-to-left.
2. Middle loop: `51-61` left bottom-to-top, `62-78` top left-to-right,
   `79-85` right top-to-bottom.
3. Inner loop: `86-100` bottom right-to-left, `101-105` left bottom-to-top,
   `106-111` top left-to-right, `112-113` downward, `114-120` left-to-right.

There are no electrical breaks between these ranges. Index adjacency follows
the strip; spatial adjacency should use coordinates instead.

## Artwork anchors on strip 1

| Artwork element | LEDs | Logical position | Player-facing location |
|---|---:|---|---|
| Moon | 2-4 | `(2,0)` to `(4,0)` | Upper-left moon |
| Bride forehead | 4 | `(4,0)` | Forehead/head accent |
| Machine logo | 8-17 | `(8,0)` to `(17,0)` | Main title across upper-right |
| Right spotlight | 29 | `(18,11)` | Lower-right spotlight |
| Left spotlight | 85 | `(17,10)` | Second lower-right spotlight, just inward from LED 29 |
| People | 38 | `(12,14)` | Street/construction scene |
| Cars | 39-42 | `(11,14)` to `(8,14)` | Bottom street vehicles |
| Left fingers | 50 | `(0,14)` | Bride's hand in lower-left corner |
| Shuttle exhaust | 55-57 | `(0,9)` to `(0,7)` | Launching shuttle at left |
| Big blue planet | 73-74 | `(12,3)` to `(13,3)` | Large blue planet, upper-right of center |
| Apple | 81 | `(17,6)` | Apple on the right |
| Jupiter | 83-84 | `(17,8)` to `(17,9)` | Large orange planet at lower-right |

The bride outline uses LEDs:

```text
3, 5, 6,
62-69,
79,
88,
94-98,
103-113,
115-118
```

These LEDs are spatially scattered around her head, body and legs. Treat them
as an artwork mask rather than a contiguous strip range.

## Strip 0

`leds0` is separate from the 19×15 map:

- `0-47`: jackpot ladder, eight numbered segments of six LEDs.
- `48-51` (`NUM_LEDS0 - 5` through `NUM_LEDS0 - 2`): four eye LEDs.
- `52` (`NUM_LEDS0 - 1`): heart.

The physical rear photo shows the eight jackpot light boxes as separate loops
fed from this strip. Do not use strip-1 coordinates for jackpot effects.
The numbered boxes appear in reverse physical-strip order from the player's
view: visible segments `1-8` map to physical segments `7-0`. Use
`kJackpotVisualToPhysical` or `FillJackpotVisualOutputSegment()` for staged
fills, chases and transitions.

## Rules for spatial effects

1. Use `kLedCoords[index]` for player-facing position and `LedRingDepth()` for
   outer/middle/inner ordering.
2. Use index arithmetic only for effects intended to follow the physical wire.
   Consecutive indices are not always nearest neighbours on the artwork.
3. The 19×15 grid preserves ordering and topology, but spacing in the real
   topper is non-uniform. The photos are authoritative for visual context.
4. For waves, cones, rain, radial pulses and meteors, calculate in `(x, y)`
   coordinates and render only the 121 mapped LED positions.
5. For artwork-specific effects, use named constants or explicit masks. Do not
   infer semantic elements solely from coordinate proximity.
6. Describe left/right and sweep directions from the player-facing view, never
   from the rear photo.
