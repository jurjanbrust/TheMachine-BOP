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
void TheBride(CRGB color);
void SingleLed(int index, CRGB color);
void Eyes(CRGB color);
void JackPotDefaultColors();

#define moonTopLeft 2
#define fronthead 4
#define people 38
#define carright1 39
#define carright2 40
#define carleft1 41
#define carleft2 42
#define apple 81
#define bigBluePlanetLeftSide 73
#define bigBluePlanetRightSide 74
#define jupiterUpper 83
#define jupiterLower 84
#define spotlights 85
#define fingersLeftCorner 50
