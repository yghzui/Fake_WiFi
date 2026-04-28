#include "ProfileHandlers.h"

#include "../../common/AppState.h"
#include "../../config/ProfileStore.h"
#include "../auth/Auth.h"
#include "../shared/WebUiUtils.h"

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
  html += "<h2>Wi-Fi 配置组管理</h2>";
  html += "<div class='toolbar'><a class='btn btn-primary' href='/edit'>新增配置组</a>";
  html += "<a class='btn btn-secondary' href='/resources'>资源监控</a>";
  html += "<a class='btn btn-muted' href='/status'>刷新</a></div>";

  html += "<h3>当前生效配置</h3>";
  html += "<div class='row-card'>";
  html += "<div class='kv'>组名：" + htmlEscape(profiles[activeProfile].name) + "</div>";
  html += "<div class='kv'>SSID：" + htmlEscape(ssid) + "</div>";
  html += "<div class='kv'>信道：" + String(channel) + "</div>";
  html += "<div class='kv'>BSSID：" + htmlEscape(macDisplay()) + "</div>";
  html += "</div>";

  html += "<h3>配置组列表</h3>";
  for (int i = 0; i < profileCount; ++i) {
    html += "<div class='row-card'>";
    html += "<div class='kv'>#" + String(i) + " 组名：" + htmlEscape(profiles[i].name);
    if (i == activeProfile) html += "（当前）";
    html += "</div>";
    html += "<div class='kv'>SSID：" + htmlEscape(profiles[i].ssid) + "</div>";
    html += "<div class='kv'>信道：" + String(clampChannel(profiles[i].channel)) + "</div>";
    html += "<div class='kv'>BSSID：" + htmlEscape(profiles[i].bssid.length() == 0 ? "默认" : profiles[i].bssid) + "</div>";
    html += "<div class='actions'>";
    html += "<a class='btn btn-muted' href='/edit?idx=" + String(i) + "'>修改</a>";

    if (i != activeProfile) {
      html += "<form class='inline-form' action='/profile/switch' method='POST'>";
      html += "<input type='hidden' name='idx' value='" + String(i) + "'>";
      html += "<button class='btn btn-success' type='submit'>切换并重启</button>";
      html += "</form>";
    }

    if (profileCount > 1 && i != activeProfile) {
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
    p.ssid = ssid;
    p.password = password;
    p.channel = channel;
    p.bssid = bssid_str;
  }

  String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>编辑配置组</title>";
  html += baseStyles();
  html += "</head><body><div class='container'>";
  html += "<h2>" + String(isEdit ? "修改配置组" : "新增配置组") + "</h2>";
  html += "<form action='/profile/save' method='POST'>";
  html += "<input type='hidden' name='idx' value='" + String(isEdit ? idx : -1) + "'>";

  html += "<label>组名：</label><input type='text' name='name' value='" + htmlEscape(p.name) + "' required>";
  html += "<label>Wi-Fi 名称 (SSID)：</label><input type='text' name='ssid' value='" + htmlEscape(p.ssid) + "' required>";
  html += "<label>Wi-Fi 密码 (为空则不加密)：</label><input type='text' name='password' value='" + htmlEscape(p.password) + "'>";
  html += "<label>Wi-Fi 信道 (1-13)：</label><input type='number' name='channel' min='1' max='13' value='" + String(clampChannel(p.channel)) + "' required>";
  html += "<label>目标 BSSID (MAC地址, 例如 1A:2B:3C:4D:5E:6F)：</label><input type='text' name='bssid' value='" + htmlEscape(p.bssid) + "'>";
  html += "<button class='btn btn-success' type='submit' name='apply' value='0'>按组保存</button> ";
  html += "<button class='btn btn-secondary' type='submit' name='apply' value='1'>保存并应用(重启)</button> ";
  html += "<a class='btn btn-muted' href='/status'>返回查看页</a>";
  html += "<div class='note'>新增组也可直接“保存并应用”，会自动切换为该组并重启。</div>";
  html += "</form></div></body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

void handleProfileSave() {
  if (!ensureAuth()) return;
  if (!server.hasArg("name") || !server.hasArg("ssid") || !server.hasArg("channel")) {
    server.send(400, "text/plain; charset=UTF-8", "参数不完整");
    return;
  }

  String nameArg = server.arg("name");
  String ssidArg = server.arg("ssid");
  String passArg = server.hasArg("password") ? server.arg("password") : "";
  int channelArg = clampChannel(server.arg("channel").toInt());
  String bssidArg = server.hasArg("bssid") ? server.arg("bssid") : "";

  if (nameArg.length() == 0 || ssidArg.length() == 0) {
    server.send(400, "text/plain; charset=UTF-8", "组名和SSID不能为空");
    return;
  }
  if (!validBssid(bssidArg)) {
    server.send(400, "text/plain; charset=UTF-8", "BSSID 格式错误");
    return;
  }

  int idx = server.hasArg("idx") ? server.arg("idx").toInt() : -1;
  int targetIdx = -1;
  if (idx >= 0 && idx < profileCount) {
    profiles[idx].name = nameArg;
    profiles[idx].ssid = ssidArg;
    profiles[idx].password = passArg;
    profiles[idx].channel = channelArg;
    profiles[idx].bssid = bssidArg;
    targetIdx = idx;
  } else {
    if (profileCount >= MAX_PROFILES) {
      server.send(400, "text/plain; charset=UTF-8", "配置组数量已达上限");
      return;
    }
    profiles[profileCount].name = nameArg;
    profiles[profileCount].ssid = ssidArg;
    profiles[profileCount].password = passArg;
    profiles[profileCount].channel = channelArg;
    profiles[profileCount].bssid = bssidArg;
    targetIdx = profileCount;
    profileCount++;
  }

  bool applyNow = server.hasArg("apply") && server.arg("apply") == "1";
  if (applyNow && targetIdx >= 0 && targetIdx < profileCount) {
    activeProfile = targetIdx;
  }
  saveProfilesToPreferences();

  if (applyNow && targetIdx >= 0 && targetIdx < profileCount) {
    applyProfileToRuntime();
    sendRestartPage("已应用配置组：" + htmlEscape(profiles[activeProfile].name));
    delay(1000);
    ESP.restart();
    return;
  }

  server.sendHeader("Location", "/status");
  server.send(302, "text/plain", "");
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
