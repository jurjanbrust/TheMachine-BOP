//+--------------------------------------------------------------------------
//
// File:        network.h
//
// NightDriverStrip - (c) 2018 Plummer's Software LLC.  All Rights Reserved.  
//
// This file is part of the NightDriver software project.
//
//    NightDriver is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//   
//    NightDriver is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//   
//    You should have received a copy of the GNU General Public License
//    along with Nightdriver.  It is normally found in copying.txt
//    If not, see <https://www.gnu.org/licenses/>.
//
// Description:
//
//    Network related functions and definitons
//
//---------------------------------------------------------------------------
#include "secrets.h"                          // copy include/secrets.example.h to include/secrets.h
#include <Arduino.h>

extern bool g_bUpdateStarted;

void processRemoteDebugCmd();
bool ConnectToWiFi(uint cRetries);
void SetupOTA(const char *pszHostname);
