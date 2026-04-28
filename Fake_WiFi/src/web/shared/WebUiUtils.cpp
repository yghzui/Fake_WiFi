#include "WebUiUtils.h"

#include "../../common/AppState.h"

String baseStyles() {
  return F(
    "<style>"
    "body{font-family:'Microsoft YaHei',sans-serif;margin:0;padding:12px;background:#f4f4f9;color:#333;}"
    ".container{max-width:860px;margin:0 auto;background:#fff;padding:14px;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,0.1);}"
    "h2,h3{margin-top:0;}"
    "table{width:100%;border-collapse:collapse;margin-top:12px;}"
    "th,td{border:1px solid #ddd;padding:8px;font-size:14px;text-align:left;vertical-align:middle;}"
    "th{background:#f0f2f5;}"
    "label{display:block;margin:10px 0 5px;color:#666;font-size:14px;}"
    "input[type='text'],input[type='password'],input[type='number'],select{width:100%;padding:10px;margin-bottom:10px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}"
    ".btn{display:inline-block;padding:10px 12px;border:none;border-radius:4px;cursor:pointer;color:#fff;text-decoration:none;font-size:14px;text-align:center;}"
    ".btn-primary{background:#007bff;}"
    ".btn-success{background:#28a745;}"
    ".btn-danger{background:#dc3545;}"
    ".btn-muted{background:#6c757d;}"
    ".btn-secondary{background:#17a2b8;}"
    ".inline-form{display:inline-block;margin-right:6px;margin-top:6px;}"
    ".actions{display:flex;flex-wrap:wrap;gap:6px;}"
    ".row-card{border:1px solid #ddd;border-radius:8px;padding:10px;margin-top:10px;background:#fafafa;}"
    ".kv{font-size:13px;line-height:1.6;color:#555;word-break:break-all;}"
    ".toolbar{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:12px;}"
    ".note{font-size:12px;color:#888;margin-top:8px;}"
    "@media (max-width:700px){"
    "body{padding:8px;}"
    ".container{padding:12px;}"
    ".btn{display:block;width:100%;box-sizing:border-box;}"
    ".inline-form{display:block;margin-right:0;}"
    ".actions{display:block;}"
    "}"
    "</style>"
  );
}

String htmlEscape(const String& src) {
  String out = src;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "&quot;");
  out.replace("'", "&#39;");
  return out;
}

void sendRestartPage(const String& msg) {
  String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>重启中</title>";
  html += baseStyles();
  html += "</head><body><div class='container'><h2>配置已保存</h2><p>" + msg + "</p><p>设备正在重启，请稍后重新连接热点并打开管理页面。</p></div></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

String macDisplay() {
  if (bssid_str.length() == 0) {
    return "设备默认 BSSID";
  }
  return bssid_str;
}

String formatBytes(uint32_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  if (bytes < 1024UL * 1024UL) return String(bytes / 1024.0f, 2) + " KB";
  return String(bytes / 1024.0f / 1024.0f, 2) + " MB";
}

String formatUptime() {
  uint32_t sec = millis() / 1000;
  uint32_t day = sec / 86400;
  sec %= 86400;
  uint32_t hour = sec / 3600;
  sec %= 3600;
  uint32_t min = sec / 60;
  sec %= 60;
  return String(day) + "天 " + String(hour) + "小时 " + String(min) + "分 " + String(sec) + "秒";
}
