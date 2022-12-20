#pragma once

void IRAM_ATTR DrawLoopTaskEntryOne(void *);
void IRAM_ATTR DrawLoopTaskEntryTwo(void *);
void IRAM_ATTR DrawLoopTaskEntryThree(void *);
void DrawWalkingDot();
void ColorFillEffect(CRGB color, int nrOfLeds, int everyNth);
void Heartbeat();
