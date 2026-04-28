#pragma once

#include <Arduino.h>

int clampChannel(int ch);
int clampMode(int mode);
bool validBssid(const String& macText);
void saveProfilesToPreferences();
void applyProfileToRuntime();
void loadProfilesFromPreferences();
