#include "secrets.h"                          // copy include/secrets.example.h to include/secrets.h
#include <Arduino.h>

extern bool g_bUpdateStarted;

void processRemoteDebugCmd();
bool ConnectToWiFi(uint cRetries);
void SetupOTA(const char *pszHostname);
