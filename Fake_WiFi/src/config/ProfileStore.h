#pragma once

#include <Arduino.h>

int clampChannel(int ch);
bool validBssid(const String& macText);
void saveProfilesToPreferences();
void applyProfileToRuntime();
void loadProfilesFromPreferences();
