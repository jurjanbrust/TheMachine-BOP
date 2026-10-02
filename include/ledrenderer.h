#pragma once

#include <Arduino.h>
#include <FastLED.h>

void InitializeLedRenderer(uint8_t brightness);
void LedRenderTaskEntry(void *);
void PublishLedFrame();
void SetLedBrightness(uint8_t brightness);
uint8_t GetLedBrightness();
uint32_t GetPublishedFrameCount();
uint16_t GetLedPowerLimitMilliamps();
void SetLedOutputEnabled(bool enabled);
bool IsLedOutputEnabled();
