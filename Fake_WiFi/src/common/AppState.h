#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

constexpr int MAX_PROFILES = 8;
constexpr int MODE_AP = 0;
constexpr int MODE_BRIDGE = 1;

struct WifiProfile {
  String name;
  int mode;
  String ssid;
  String password;
  int channel;
  String bssid;
  String upstreamSsid;
  String upstreamPassword;
  String upstreamBssid;
};

extern WebServer server;
extern Preferences preferences;

extern String ssid;
extern String password;
extern int channel;
extern String bssid_str;
extern int currentMode;
extern String upstreamSsid;
extern String upstreamPassword;
extern String upstreamBssid;

extern const char* AUTH_USER;
extern const char* AUTH_PASS;

extern WifiProfile profiles[MAX_PROFILES];
extern int profileCount;
extern int activeProfile;
