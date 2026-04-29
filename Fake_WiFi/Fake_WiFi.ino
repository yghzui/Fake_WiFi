#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include "soc/soc.h"           // 引入 BOD 控制需要的头文件
#include "soc/rtc_cntl_reg.h"  // 引入 BOD 控制需要的头文件
#if defined(__has_include)
#if __has_include("lwip/lwip_napt.h")
#include "lwip/lwip_napt.h"
#include "lwip/tcpip.h"
#define HAS_LWIP_NAPT 1
#else
#define HAS_LWIP_NAPT 0
#endif
#else
#define HAS_LWIP_NAPT 0
#endif

#include "src/common/AppState.h"
#include "src/config/ProfileStore.h"
#include "src/web/WebHandlers.h"

WebServer server(80);
Preferences preferences;

String ssid = "Fake_WiFi_Setup";
String password = "";
int channel = 1;
String bssid_str = "";
int currentMode = MODE_AP;
String upstreamSsid = "";
String upstreamPassword = "";
String upstreamBssid = "";

const char* AUTH_USER = "ad321";
const char* AUTH_PASS = "ad123";

WifiProfile profiles[MAX_PROFILES];
int profileCount = 0;
int activeProfile = 0;

#if HAS_LWIP_NAPT
struct NaptEnableArgs {
  uint32_t apIp;
};

static NaptEnableArgs gNaptArgs = {0};

static void naptEnableOnTcpipThread(void* arg) {
  NaptEnableArgs* args = static_cast<NaptEnableArgs*>(arg);
#if defined(SOFTAP_IF)
  ip_napt_enable_no(SOFTAP_IF, 1);
#else
  ip_napt_enable(args->apIp, 1);
#endif
}
#endif

static bool parseMacText(const String& macText, uint8_t mac[6]) {
  if (macText.length() != 17) return false;
  int values[6];
  if (6 != sscanf(macText.c_str(), "%x:%x:%x:%x:%x:%x",
                  &values[0], &values[1], &values[2], &values[3], &values[4], &values[5])) {
    return false;
  }
  for (int i = 0; i < 6; ++i) {
    mac[i] = static_cast<uint8_t>(values[i]);
  }
  return true;
}

static void applyApMacIfNeeded() {
  if (bssid_str.length() == 0) return;

  // 将逗号分隔的 BSSID 字符串解析为数组
  int macCount = 0;
  String macs[10]; // 最多支持10个MAC地址轮换
  int startIdx = 0;
  while (startIdx < bssid_str.length() && macCount < 10) {
    int commaIdx = bssid_str.indexOf(',', startIdx);
    if (commaIdx == -1) commaIdx = bssid_str.length();
    
    String mac = bssid_str.substring(startIdx, commaIdx);
    mac.trim();
    if (mac.length() == 17) {
      macs[macCount++] = mac;
    }
    startIdx = commaIdx + 1;
  }

  if (macCount == 0) return;

  // 读取上次使用的索引，并计算本次应该使用的索引
  int lastBssidIdx = preferences.getInt("last_bssid_idx", -1);
  int currentBssidIdx = (lastBssidIdx + 1) % macCount;
  
  // 更新并保存当前索引供下次使用
  preferences.putInt("last_bssid_idx", currentBssidIdx);

  uint8_t mac[6];
  if (!parseMacText(macs[currentBssidIdx], mac)) return;
  
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, mac);
  if (err == ESP_OK) {
    Serial.print("自定义 BSSID 设置成功。当前轮换到: ");
    Serial.println(macs[currentBssidIdx]);
  } else {
    Serial.print("自定义 BSSID 设置失败！目标: ");
    Serial.println(macs[currentBssidIdx]);
  }
}

static bool connectUpstreamSta() {
  if (upstreamSsid.length() == 0) {
    Serial.println("中继模式未配置上游 WiFi，跳过 STA 连接。");
    return false;
  }

  uint8_t bssid[6];
  const uint8_t* bssidPtr = nullptr;
  if (upstreamBssid.length() > 0 && parseMacText(upstreamBssid, bssid)) {
    bssidPtr = bssid;
  }

  WiFi.disconnect(true, true);
  delay(200);
  WiFi.begin(upstreamSsid.c_str(),
             upstreamPassword.length() > 0 ? upstreamPassword.c_str() : nullptr,
             0, bssidPtr, true);

  Serial.print("连接上游 WiFi: ");
  Serial.println(upstreamSsid);
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("上游连接成功，STA IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("上游连接失败，将仅保留 AP。");
  return false;
}

static void enableNatIfPossible() {
#if HAS_LWIP_NAPT
  gNaptArgs.apIp = (uint32_t)WiFi.softAPIP();
  err_t err = tcpip_callback(naptEnableOnTcpipThread, &gNaptArgs);
  if (err == ERR_OK) {
    Serial.println("NAT 启用任务已提交。");
  } else {
    Serial.println("NAT 启用失败，tcpip_callback 调用异常。");
  }
#else
  Serial.println("当前环境不支持 lwip_napt，未启用 NAT。");
#endif
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // 关闭欠压检测 (Brownout detector)，防止因瞬间电流过大导致重启

  Serial.begin(115200);
  delay(1000);
  
  // 初始化 Preferences，命名空间为 wifi-config
  preferences.begin("wifi-config", false);
  loadProfilesFromPreferences();

  if (currentMode == MODE_BRIDGE) {
    Serial.println("正在启动中继模式(AP+STA)...");
    WiFi.mode(WIFI_AP_STA);
  } else {
    Serial.println("正在启动 AP 模式(启用 STA 接口以支持扫描)...");
    WiFi.mode(WIFI_AP_STA);
  }

  // 降低 Wi-Fi 发射功率以减小峰值电流 (默认约为 19.5dBm 或 20dBm，这里降低到 8.5dBm 左右)
  // 如果发现信号太弱，可以改成 WIFI_POWER_11dBm, WIFI_POWER_15dBm 等
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  applyApMacIfNeeded();

  bool upstreamConnected = false;
  if (currentMode == MODE_BRIDGE) {
    upstreamConnected = connectUpstreamSta();
  }

  // 启动热点
  if (password.length() > 0) {
    WiFi.softAP(ssid.c_str(), password.c_str(), channel);
  } else {
    WiFi.softAP(ssid.c_str(), NULL, channel);
  }

  Serial.print("AP IP 地址: ");
  Serial.println(WiFi.softAPIP()); // 默认通常是 192.168.4.1
  if (currentMode == MODE_BRIDGE && upstreamConnected) {
    enableNatIfPossible();
  }
  
  // 配置路由
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/resources", HTTP_GET, handleResources);
  server.on("/resources/export", HTTP_GET, handleResourcesExport);
  server.on("/edit", HTTP_GET, handleEdit);
  server.on("/profile/save", HTTP_POST, handleProfileSave);
  server.on("/profile/delete", HTTP_POST, handleProfileDelete);
  server.on("/profile/switch", HTTP_POST, handleProfileSwitch);
  server.on("/wifi/scan", HTTP_GET, handleWifiScan);
  server.begin();
  
  Serial.println("Web 服务器已启动");
}

void loop() {
  server.handleClient();
}
