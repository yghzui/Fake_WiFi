#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

constexpr int MAX_PROFILES = 8;

struct WifiProfile {
  String name;
  String ssid;
  String password;
  int channel;
  String bssid;
};

extern WebServer server;
extern Preferences preferences;

extern String ssid;
extern String password;
extern int channel;
extern String bssid_str;

extern const char* AUTH_USER;
extern const char* AUTH_PASS;

extern WifiProfile profiles[MAX_PROFILES];
extern int profileCount;
extern int activeProfile;
