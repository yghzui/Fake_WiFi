#include "ProfileStore.h"

#include "../common/AppState.h"

int clampChannel(int ch) {
  if (ch < 1) return 1;
  if (ch > 13) return 13;
  return ch;
}

int clampMode(int mode) {
  if (mode != MODE_BRIDGE) return MODE_AP;
  return MODE_BRIDGE;
}

bool validBssid(const String& macText) {
  if (macText.length() == 0) return true;
  // 允许使用逗号分隔的多个 MAC 地址
  int startIdx = 0;
  while (startIdx < macText.length()) {
    int commaIdx = macText.indexOf(',', startIdx);
    if (commaIdx == -1) commaIdx = macText.length();
    
    String mac = macText.substring(startIdx, commaIdx);
    mac.trim();
    
    if (mac.length() > 0) {
      if (mac.length() != 17) return false;
      int values[6];
      if (6 != sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x",
                      &values[0], &values[1], &values[2], &values[3], &values[4], &values[5])) {
        return false;
      }
    }
    startIdx = commaIdx + 1;
  }
  return true;
}

static String keyFor(int index, const char* suffix) {
  return "p" + String(index) + "_" + suffix;
}

static void removeProfileKeys(int index) {
  preferences.remove(keyFor(index, "name").c_str());
  preferences.remove(keyFor(index, "mode").c_str());
  preferences.remove(keyFor(index, "ssid").c_str());
  preferences.remove(keyFor(index, "password").c_str());
  preferences.remove(keyFor(index, "channel").c_str());
  preferences.remove(keyFor(index, "bssid").c_str());
  preferences.remove(keyFor(index, "up_ssid").c_str());
  preferences.remove(keyFor(index, "up_password").c_str());
  preferences.remove(keyFor(index, "up_bssid").c_str());
}

void saveProfilesToPreferences() {
  preferences.putInt("profile_count", profileCount);
  preferences.putInt("active_profile", activeProfile);

  for (int i = 0; i < profileCount; ++i) {
    preferences.putString(keyFor(i, "name").c_str(), profiles[i].name);
    preferences.putInt(keyFor(i, "mode").c_str(), clampMode(profiles[i].mode));
    preferences.putString(keyFor(i, "ssid").c_str(), profiles[i].ssid);
    preferences.putString(keyFor(i, "password").c_str(), profiles[i].password);
    preferences.putInt(keyFor(i, "channel").c_str(), clampChannel(profiles[i].channel));
    preferences.putString(keyFor(i, "bssid").c_str(), profiles[i].bssid);
    preferences.putString(keyFor(i, "up_ssid").c_str(), profiles[i].upstreamSsid);
    preferences.putString(keyFor(i, "up_password").c_str(), profiles[i].upstreamPassword);
    preferences.putString(keyFor(i, "up_bssid").c_str(), profiles[i].upstreamBssid);
  }

  for (int i = profileCount; i < MAX_PROFILES; ++i) {
    removeProfileKeys(i);
  }
}

void applyProfileToRuntime() {
  if (activeProfile < 0 || activeProfile >= profileCount) activeProfile = 0;
  currentMode = clampMode(profiles[activeProfile].mode);
  ssid = profiles[activeProfile].ssid;
  password = profiles[activeProfile].password;
  channel = clampChannel(profiles[activeProfile].channel);
  bssid_str = profiles[activeProfile].bssid;
  upstreamSsid = profiles[activeProfile].upstreamSsid;
  upstreamPassword = profiles[activeProfile].upstreamPassword;
  upstreamBssid = profiles[activeProfile].upstreamBssid;
}

void loadProfilesFromPreferences() {
  profileCount = preferences.getInt("profile_count", 0);
  if (profileCount < 0 || profileCount > MAX_PROFILES) profileCount = 0;

  if (profileCount == 0) {
    // 兼容旧版本：读取单组配置，迁移为第 0 组
    profiles[0].name = "默认组";
    profiles[0].mode = MODE_AP;
    profiles[0].ssid = preferences.getString("ssid", "Fake_WiFi_Setup");
    profiles[0].password = preferences.getString("password", "");
    profiles[0].channel = clampChannel(preferences.getInt("channel", 1));
    profiles[0].bssid = preferences.getString("bssid", "");
    profiles[0].upstreamSsid = "";
    profiles[0].upstreamPassword = "";
    profiles[0].upstreamBssid = "";
    profileCount = 1;
    activeProfile = 0;
    saveProfilesToPreferences();
  } else {
    for (int i = 0; i < profileCount; ++i) {
      profiles[i].name = preferences.getString(keyFor(i, "name").c_str(), "配置组" + String(i + 1));
      profiles[i].mode = clampMode(preferences.getInt(keyFor(i, "mode").c_str(), MODE_AP));
      profiles[i].ssid = preferences.getString(keyFor(i, "ssid").c_str(), "Fake_WiFi_Setup");
      profiles[i].password = preferences.getString(keyFor(i, "password").c_str(), "");
      profiles[i].channel = clampChannel(preferences.getInt(keyFor(i, "channel").c_str(), 1));
      profiles[i].bssid = preferences.getString(keyFor(i, "bssid").c_str(), "");
      profiles[i].upstreamSsid = preferences.getString(keyFor(i, "up_ssid").c_str(), "");
      profiles[i].upstreamPassword = preferences.getString(keyFor(i, "up_password").c_str(), "");
      profiles[i].upstreamBssid = preferences.getString(keyFor(i, "up_bssid").c_str(), "");
    }
    activeProfile = preferences.getInt("active_profile", 0);
    if (activeProfile < 0 || activeProfile >= profileCount) activeProfile = 0;
  }

  applyProfileToRuntime();
}
