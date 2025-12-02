#pragma once

void IRAM_ATTR DrawLoopTaskEntryOne(void *);
void IRAM_ATTR DrawLoopTaskEntryTwo(void *);
void IRAM_ATTR DrawLoopTaskEntryThree(void *);
void IRAM_ATTR DrawLoopTaskEntryFour(void *);
void ColorFillEffect(CRGB color, int nrOfLeds, int everyNth);
void Heartbeat(int channel);
void TheMachineLogo(CRGB color);
void TheBride(CRGB color);
void SingleLed(int index, CRGB color);
void Eyes(CRGB color);
void FlickerSpotlight(uint8_t index, const CRGB & color);

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
#define spotlights1 85
#define spotlights2 29
#define fingersLeftCorner 50
#define theMachineFirstLed 8
#define theMachineLastLed 17