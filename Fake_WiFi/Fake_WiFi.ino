#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_wifi.h>

#include "src/common/AppState.h"
#include "src/config/ProfileStore.h"
#include "src/web/WebHandlers.h"

WebServer server(80);
Preferences preferences;

String ssid = "Fake_WiFi_Setup";
String password = "";
int channel = 1;
String bssid_str = "";

const char* AUTH_USER = "ad321";
const char* AUTH_PASS = "ad123";

WifiProfile profiles[MAX_PROFILES];
int profileCount = 0;
int activeProfile = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 初始化 Preferences，命名空间为 wifi-config
  preferences.begin("wifi-config", false);
  loadProfilesFromPreferences();

  Serial.println("正在启动 AP 模式...");
  
  // 设为 AP 模式
  WiFi.mode(WIFI_AP);
  
  // 如果配置了自定义 BSSID，则修改底层 MAC 地址
  if (bssid_str.length() == 17) {
    uint8_t mac[6];
    int values[6];
    // 解析 MAC 字符串
    if (6 == sscanf(bssid_str.c_str(), "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5])) {
      for (int i = 0; i < 6; ++i) {
        mac[i] = (uint8_t)values[i];
      }
      // 使用 ESP-IDF 接口修改 AP 接口的 MAC 地址
      esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, mac);
      if (err == ESP_OK) {
        Serial.println("自定义 BSSID 设置成功。");
      } else {
        Serial.println("自定义 BSSID 设置失败！");
      }
    }
  }

  // 启动热点
  if (password.length() > 0) {
    WiFi.softAP(ssid.c_str(), password.c_str(), channel);
  } else {
    WiFi.softAP(ssid.c_str(), NULL, channel);
  }

  Serial.print("AP IP 地址: ");
  Serial.println(WiFi.softAPIP()); // 默认通常是 192.168.4.1
  
  // 配置路由
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/resources", HTTP_GET, handleResources);
  server.on("/resources/export", HTTP_GET, handleResourcesExport);
  server.on("/edit", HTTP_GET, handleEdit);
  server.on("/profile/save", HTTP_POST, handleProfileSave);
  server.on("/profile/delete", HTTP_POST, handleProfileDelete);
  server.on("/profile/switch", HTTP_POST, handleProfileSwitch);
  server.begin();
  
  Serial.println("Web 服务器已启动");
}

void loop() {
  server.handleClient();
}
