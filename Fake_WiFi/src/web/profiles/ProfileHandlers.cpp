#include "ProfileHandlers.h"

#include <WiFi.h>

#include "../../common/AppState.h"
#include "../../config/ProfileStore.h"
#include "../auth/Auth.h"
#include "../shared/WebUiUtils.h"

static String modeLabel(int mode) {
  return (clampMode(mode) == MODE_BRIDGE) ? "中继模式(AP+STA)" : "AP模式";
}

static String jsonEscape(const String& src) {
  String out;
  out.reserve(src.length() + 8);
  for (size_t i = 0; i < src.length(); ++i) {
    char c = src[i];
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

void handleRoot() {
  if (!ensureAuth()) return;
  server.sendHeader("Location", "/status");
  server.send(302, "text/plain", "");
}

void handleStatus() {
  if (!ensureAuth()) return;

  String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Wi-Fi 配置组管理</title>";
  html += baseStyles();
  html += "</head><body><div class='container'>";
  html += "<h2 class='page-title'>Wi-Fi 配置组管理</h2>";
  html += "<div class='toolbar'><a class='btn btn-primary' href='/edit'>新增配置组</a>";
  html += "<a class='btn btn-secondary' href='/resources'>资源监控</a>";
  html += "<a class='btn btn-muted' href='/status'>刷新</a></div>";

  html += "<h3 class='section-title'>配置组列表</h3>";
  int displayIndices[profileCount];
  displayIndices[0] = activeProfile;
  int idx = 1;
  for (int i = 0; i < profileCount; ++i) {
    if (i != activeProfile) {
      displayIndices[idx++] = i;
    }
  }

  for (int j = 0; j < profileCount; ++j) {
    int i = displayIndices[j];
    html += "<div class='row-card'>";
    html += "<div class='card-title'>#" + String(j) + " " + htmlEscape(profiles[i].name);
    if (j == 0) html += "<span class='badge'>当前生效</span>";
    html += "</div>";
    html += "<div class='kv-grid'>";
    html += "<div class='kv-item'><span class='kv-label'>模式：</span>" + modeLabel(profiles[i].mode) + "</div>";
    html += "<div class='kv-item'><span class='kv-label'>SSID：</span>" + htmlEscape(profiles[i].ssid) + "</div>";
    html += "<div class='kv-item'><span class='kv-label'>信道：</span>" + String(clampChannel(profiles[i].channel)) + "</div>";
    html += "<div class='kv-item'><span class='kv-label'>BSSID：</span>" + htmlEscape(profiles[i].bssid.length() == 0 ? "默认" : profiles[i].bssid) + "</div>";
    if (clampMode(profiles[i].mode) == MODE_BRIDGE) {
      html += "<div class='kv-item'><span class='kv-label'>上游 WiFi：</span>" + htmlEscape(profiles[i].upstreamSsid.length() > 0 ? profiles[i].upstreamSsid : "未配置") + "</div>";
      html += "<div class='kv-item'><span class='kv-label'>上游 BSSID：</span>" + htmlEscape(profiles[i].upstreamBssid.length() > 0 ? profiles[i].upstreamBssid : "自动") + "</div>";
    }
    html += "</div>";
    html += "<div class='actions'>";
    html += "<a class='btn btn-muted' href='/edit?idx=" + String(i) + "'>修改</a>";

    if (j > 0) {
      html += "<form class='inline-form' action='/profile/switch' method='POST'>";
      html += "<input type='hidden' name='idx' value='" + String(i) + "'>";
      html += "<button class='btn btn-success' type='submit'>切换并重启</button>";
      html += "</form>";
    }

    if (profileCount > 1 && j > 0) {
      html += "<form class='inline-form' action='/profile/delete' method='POST' onsubmit=\"return confirm('确认删除该配置组？');\">";
      html += "<input type='hidden' name='idx' value='" + String(i) + "'>";
      html += "<button class='btn btn-danger' type='submit'>删除</button>";
      html += "</form>";
    }

    html += "</div></div>";
  }
  html += "<div class='note'>默认限制信道范围为 1-13；如留空 BSSID 则使用设备默认 BSSID。账号密码请在代码中修改 AUTH_USER / AUTH_PASS。当前组不能删除，如需删除请先切换到其他组。</div>";
  html += "</div></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

void handleEdit() {
  if (!ensureAuth()) return;

  int idx = -1;
  if (server.hasArg("idx")) idx = server.arg("idx").toInt();

  bool isEdit = (idx >= 0 && idx < profileCount);
  WifiProfile p;
  if (isEdit) {
    p = profiles[idx];
  } else {
    p.name = "配置组" + String(profileCount + 1);
    p.mode = MODE_AP;
    p.ssid = ssid;
    p.password = password;
    p.channel = channel;
    p.bssid = bssid_str;
    p.upstreamSsid = "";
    p.upstreamPassword = "";
    p.upstreamBssid = "";
  }

  String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>编辑配置组</title>";
  html += baseStyles();
  html += "</head><body><div class='container'>";
  html += "<h2 class='page-title'>" + String(isEdit ? "修改配置组" : "新增配置组") + "</h2>";
  html += "<form action='/profile/save' method='POST'>";
  html += "<input type='hidden' name='idx' value='" + String(isEdit ? idx : -1) + "'>";

  html += "<div class='section'><h3 class='section-title'>基础信息</h3>";
  html += "<label>组名：</label><input type='text' name='name' value='" + htmlEscape(p.name) + "' required>";
  html += "<label>模式：</label><select id='mode' name='mode'>";
  html += "<option value='0'" + String(clampMode(p.mode) == MODE_AP ? " selected" : "") + ">AP 模式（仅热点）</option>";
  html += "<option value='1'" + String(clampMode(p.mode) == MODE_BRIDGE ? " selected" : "") + ">中继模式（连接上游WiFi并共享热点）</option>";
  html += "</select>";
  html += "</div>";

  html += "<div class='section'><h3 class='section-title'>热点参数</h3>";
  html += "<label>Wi-Fi 名称 (SSID)：</label><input type='text' name='ssid' value='" + htmlEscape(p.ssid) + "' required>";
  html += "<label>Wi-Fi 密码 (为空则不加密)：</label><input type='text' name='password' value='" + htmlEscape(p.password) + "'>";
  html += "<label>Wi-Fi 信道 (1-13)：</label><input type='number' name='channel' min='1' max='13' value='" + String(clampChannel(p.channel)) + "' required>";
  html += "<label>目标 BSSID (多MAC请用逗号分隔, 例如 1A:2B:3C:4D:5E:6F,1A:2B:3C:4D:5E:7F)：</label><input type='text' name='bssid' value='" + htmlEscape(p.bssid) + "'>";
  html += "</div>";

  html += "<div id='bridgeFields'>";
  html += "<div class='section'><h3 class='section-title'>上游 WiFi（中继模式）</h3>";
  html += "<label>上游 WiFi 名称 (SSID)：</label><input id='upstreamSsid' type='text' name='upstream_ssid' value='" + htmlEscape(p.upstreamSsid) + "'>";
  html += "<label>上游 WiFi 密码：</label><input type='text' name='upstream_password' value='" + htmlEscape(p.upstreamPassword) + "'>";
  html += "<label>上游 BSSID (可选)：</label><input id='upstreamBssid' type='text' name='upstream_bssid' value='" + htmlEscape(p.upstreamBssid) + "'>";
  html += "<div class='toolbar'><a class='btn btn-secondary' href='#' onclick='scanNearbyWifi();return false;'>扫描附近 WiFi</a></div>";
  html += "<div id='scanResult' class='note'>点击扫描后可快速填充上游 WiFi。</div>";
  html += "</div></div>";
  html += "<div class='actions'>";
  html += "<button class='btn btn-primary' type='submit' name='apply' value='1'>保存并应用(重启)</button>";
  html += "<button class='btn btn-success' type='submit' name='apply' value='0'>按组保存</button>";
  html += "<a class='btn btn-muted' href='/status'>返回查看页</a>";
  html += "</div>";
  html += "<div class='note'>新增组也可直接“保存并应用”，会自动切换为该组并重启。</div>";
  html += "<script>";
  html += "const modeEl=document.getElementById('mode');";
  html += "const bridgeFields=document.getElementById('bridgeFields');";
  html += "const scanResult=document.getElementById('scanResult');";
  html += "function updateBridgeFields(){";
  html += "const isBridge=modeEl&&modeEl.value==='1';";
  html += "bridgeFields.style.display=isBridge?'block':'none';";
  html += "}";
  html += "modeEl.addEventListener('change',updateBridgeFields);";
  html += "updateBridgeFields();";
  html += "function applyNetwork(ssid,bssid){";
  html += "document.getElementById('mode').value='1';";
  html += "updateBridgeFields();";
  html += "document.getElementById('upstreamSsid').value=ssid;";
  html += "document.getElementById('upstreamBssid').value=bssid||'';";
  html += "}";
  html += "async function scanNearbyWifi(){";
  html += "scanResult.innerHTML='扫描中...';";
  html += "try{";
  html += "const resp=await fetch('/wifi/scan');";
  html += "if(!resp.ok){throw new Error('HTTP '+resp.status);}";
  html += "const data=await resp.json();";
  html += "if(!Array.isArray(data.networks)||data.networks.length===0){scanResult.innerHTML='未扫描到可用 WiFi';return;}";
  html += "const rows=data.networks.map((n)=>{";
  html += "const lock=n.open?'开放':'加密';";
  html += "const bssid=n.bssid||'';";
  html += "const safeSsid=(n.ssid||'').replace(/'/g,'&#39;');";
  html += "const safeBssid=bssid.replace(/'/g,'&#39;');";
  html += "return \"<div class='row-card'><div class='kv'>\"+safeSsid+\" | CH\"+n.channel+\" | RSSI \"+n.rssi+\" | \"+lock+\"</div><div class='actions'><a class='btn btn-primary' href='#' onclick=\\\"applyNetwork('\"+safeSsid+\"','\"+safeBssid+\"');return false;\\\">设为上游并填充</a></div></div>\";";
  html += "});";
  html += "scanResult.innerHTML=rows.join('');";
  html += "}catch(e){";
  html += "scanResult.innerHTML='扫描失败：'+e;";
  html += "}";
  html += "}";
  html += "</script>";
  html += "</form></div></body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

void handleProfileSave() {
  if (!ensureAuth()) return;
  if (!server.hasArg("name") || !server.hasArg("ssid") || !server.hasArg("channel") || !server.hasArg("mode")) {
    server.send(400, "text/plain; charset=UTF-8", "参数不完整");
    return;
  }

  String nameArg = server.arg("name");
  int modeArg = clampMode(server.arg("mode").toInt());
  String ssidArg = server.arg("ssid");
  String passArg = server.hasArg("password") ? server.arg("password") : "";
  int channelArg = clampChannel(server.arg("channel").toInt());
  String bssidArg = server.hasArg("bssid") ? server.arg("bssid") : "";
  String upstreamSsidArg = server.hasArg("upstream_ssid") ? server.arg("upstream_ssid") : "";
  String upstreamPassArg = server.hasArg("upstream_password") ? server.arg("upstream_password") : "";
  String upstreamBssidArg = server.hasArg("upstream_bssid") ? server.arg("upstream_bssid") : "";

  if (nameArg.length() == 0 || ssidArg.length() == 0) {
    server.send(400, "text/plain; charset=UTF-8", "组名和SSID不能为空");
    return;
  }
  if (!validBssid(bssidArg) || !validBssid(upstreamBssidArg)) {
    server.send(400, "text/plain; charset=UTF-8", "BSSID 格式错误");
    return;
  }
  if (modeArg == MODE_BRIDGE && upstreamSsidArg.length() == 0) {
    server.send(400, "text/plain; charset=UTF-8", "中继模式必须填写上游WiFi SSID");
    return;
  }

  int idx = server.hasArg("idx") ? server.arg("idx").toInt() : -1;
  int targetIdx = -1;
  if (idx >= 0 && idx < profileCount) {
    profiles[idx].name = nameArg;
    profiles[idx].mode = modeArg;
    profiles[idx].ssid = ssidArg;
    profiles[idx].password = passArg;
    profiles[idx].channel = channelArg;
    profiles[idx].bssid = bssidArg;
    profiles[idx].upstreamSsid = upstreamSsidArg;
    profiles[idx].upstreamPassword = upstreamPassArg;
    profiles[idx].upstreamBssid = upstreamBssidArg;
    targetIdx = idx;
  } else {
    if (profileCount >= MAX_PROFILES) {
      server.send(400, "text/plain; charset=UTF-8", "配置组数量已达上限");
      return;
    }
    profiles[profileCount].name = nameArg;
    profiles[profileCount].mode = modeArg;
    profiles[profileCount].ssid = ssidArg;
    profiles[profileCount].password = passArg;
    profiles[profileCount].channel = channelArg;
    profiles[profileCount].bssid = bssidArg;
    profiles[profileCount].upstreamSsid = upstreamSsidArg;
    profiles[profileCount].upstreamPassword = upstreamPassArg;
    profiles[profileCount].upstreamBssid = upstreamBssidArg;
    targetIdx = profileCount;
    profileCount++;
  }

  bool applyNow = server.hasArg("apply") && server.arg("apply") == "1";
  if (applyNow && targetIdx >= 0 && targetIdx < profileCount) {
    activeProfile = targetIdx;
  }
  saveProfilesToPreferences();

  if (applyNow) {
    applyProfileToRuntime();
    sendRestartPage("已应用配置组：" + htmlEscape(profiles[activeProfile].name));
    delay(1000);
    ESP.restart();
    return;
  }

  // 区分是否修改了当前激活的组
  if (targetIdx == activeProfile) {
    // 使用 JS 弹窗提示，然后返回状态页
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body>";
    html += "<script>alert('当前生效组的配置已修改，将在下次设备重启后生效。');window.location.href='/status';</script>";
    html += "</body></html>";
    server.send(200, "text/html; charset=UTF-8", html);
    return;
  }

  server.sendHeader("Location", "/status");
  server.send(302, "text/plain", "");
}

void handleWifiScan() {
  if (!ensureAuth()) return;

  int count = WiFi.scanNetworks(false, true);
  if (count < 0) {
    server.send(500, "application/json; charset=UTF-8", "{\"ok\":false,\"error\":\"scan_failed\"}");
    return;
  }

  String json = "{\"ok\":true,\"networks\":[";
  for (int i = 0; i < count; ++i) {
    if (i > 0) json += ",";
    String ssidScan = WiFi.SSID(i);
    String bssidScan = WiFi.BSSIDstr(i);
    int rssi = WiFi.RSSI(i);
    int ch = WiFi.channel(i);
    bool isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    json += "{";
    json += "\"ssid\":\"" + jsonEscape(ssidScan) + "\",";
    json += "\"bssid\":\"" + jsonEscape(bssidScan) + "\",";
    json += "\"rssi\":" + String(rssi) + ",";
    json += "\"channel\":" + String(ch) + ",";
    json += "\"open\":" + String(isOpen ? "true" : "false");
    json += "}";
  }
  json += "]}";
  WiFi.scanDelete();
  server.send(200, "application/json; charset=UTF-8", json);
}

void handleProfileDelete() {
  if (!ensureAuth()) return;
  if (!server.hasArg("idx")) {
    server.send(400, "text/plain; charset=UTF-8", "缺少 idx 参数");
    return;
  }

  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= profileCount) {
    server.send(400, "text/plain; charset=UTF-8", "配置组不存在");
    return;
  }
  if (profileCount <= 1) {
    server.send(400, "text/plain; charset=UTF-8", "至少保留一个配置组");
    return;
  }
  if (idx == activeProfile) {
    server.send(400, "text/plain; charset=UTF-8", "当前生效组不能删除");
    return;
  }

  for (int i = idx; i < profileCount - 1; ++i) {
    profiles[i] = profiles[i + 1];
  }
  profileCount--;
  if (activeProfile >= profileCount) activeProfile = profileCount - 1;

  saveProfilesToPreferences();
  server.sendHeader("Location", "/status");
  server.send(302, "text/plain", "");
}

void handleProfileSwitch() {
  if (!ensureAuth()) return;
  if (!server.hasArg("idx")) {
    server.send(400, "text/plain; charset=UTF-8", "缺少 idx 参数");
    return;
  }

  int idx = server.arg("idx").toInt();
  if (idx < 0 || idx >= profileCount) {
    server.send(400, "text/plain; charset=UTF-8", "配置组不存在");
    return;
  }

  activeProfile = idx;
  saveProfilesToPreferences();
  applyProfileToRuntime();

  sendRestartPage("已切换到配置组：" + htmlEscape(profiles[activeProfile].name));
  delay(1000);
  ESP.restart();
}
