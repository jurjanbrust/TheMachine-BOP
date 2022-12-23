#pragma once

void IRAM_ATTR DrawLoopTaskEntryOne(void *);
void IRAM_ATTR DrawLoopTaskEntryTwo(void *);
void IRAM_ATTR DrawLoopTaskEntryThree(void *);
void IRAM_ATTR DrawLoopTaskEntryFour(void *);
void DrawWalkingDot();
void ColorFillEffect(CRGB color, int nrOfLeds, int everyNth);
void Heartbeat(int channel);
void PostDrawHandler();
void TheMachineLogo(CRGB color);

#define moonTopLeft 2
#define fronthead 4
#define apple 25
#define apple2 80
#define bigBluePlanetLeftSIde 73
#define bigBluePlanetRightSIde 74
#define jupiter = 84
#define spotlights = 85
