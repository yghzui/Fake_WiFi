#include "WebUiUtils.h"

#include "../../common/AppState.h"

String baseStyles() {
  return F(
    "<style>"
    ":root{"
    "--bg:#f3f6fb;"
    "--surface:#ffffff;"
    "--surface-soft:#f8fafc;"
    "--text:#1f2937;"
    "--text-muted:#6b7280;"
    "--border:#e5e7eb;"
    "--primary:#3b82f6;"
    "--primary-hover:#2563eb;"
    "--success:#10b981;"
    "--success-hover:#059669;"
    "--danger:#ef4444;"
    "--danger-hover:#dc2626;"
    "--muted:#64748b;"
    "--muted-hover:#475569;"
    "--accent:#0ea5b7;"
    "--accent-hover:#0f8898;"
    "}"
    "body{font-family:'Microsoft YaHei','PingFang SC','Segoe UI',sans-serif;margin:0;padding:16px;background:var(--bg);color:var(--text);line-height:1.45;}"
    ".container{max-width:920px;margin:0 auto;background:var(--surface);padding:16px;border:1px solid var(--border);border-radius:12px;box-shadow:0 6px 24px rgba(15,23,42,0.06);}"
    "h2,h3{margin:0;color:var(--text);}"
    ".page-title{font-size:22px;font-weight:600;margin-bottom:12px;}"
    ".section-title{font-size:16px;font-weight:600;margin:0 0 10px 0;}"
    "label{display:block;margin:9px 0 5px;color:var(--text-muted);font-size:13px;}"
    "input[type='text'],input[type='password'],input[type='number'],select{width:100%;padding:9px 10px;margin-bottom:8px;border:1px solid var(--border);border-radius:8px;box-sizing:border-box;background:#fff;color:var(--text);outline:none;transition:border-color .15s,box-shadow .15s;}"
    "input[type='text']:focus,input[type='password']:focus,input[type='number']:focus,select:focus{border-color:#93c5fd;box-shadow:0 0 0 3px rgba(59,130,246,0.16);}"
    ".btn{display:inline-block;padding:9px 13px;border:none;border-radius:8px;cursor:pointer;color:#fff;text-decoration:none;font-size:13px;font-weight:600;text-align:center;line-height:1.2;transition:background-color .15s,transform .05s;}"
    ".btn:active{transform:translateY(1px);}"
    ".btn-primary{background:var(--primary);}"
    ".btn-primary:hover{background:var(--primary-hover);}"
    ".btn-success{background:var(--success);}"
    ".btn-success:hover{background:var(--success-hover);}"
    ".btn-danger{background:var(--danger);}"
    ".btn-danger:hover{background:var(--danger-hover);}"
    ".btn-muted{background:var(--muted);}"
    ".btn-muted:hover{background:var(--muted-hover);}"
    ".btn-secondary{background:var(--accent);}"
    ".btn-secondary:hover{background:var(--accent-hover);}"
    ".inline-form{display:inline-block;margin:0;}"
    ".toolbar{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:12px;}"
    ".row-card{border:1px solid var(--border);border-radius:10px;padding:12px;margin-top:10px;background:var(--surface-soft);}"
    ".card-title{display:flex;align-items:center;gap:8px;margin-bottom:8px;font-size:15px;font-weight:600;color:var(--text);}"
    ".badge{display:inline-block;padding:2px 8px;border-radius:999px;font-size:11px;line-height:1.4;background:#dbeafe;color:#1d4ed8;}"
    ".kv{font-size:13px;line-height:1.6;color:var(--text-muted);word-break:break-all;}"
    ".kv-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:6px 14px;}"
    ".kv-item{font-size:13px;color:var(--text);word-break:break-all;}"
    ".kv-label{color:var(--text-muted);margin-right:4px;}"
    ".actions{display:flex;flex-wrap:wrap;gap:8px;margin-top:10px;}"
    ".note{font-size:12px;color:var(--text-muted);margin-top:10px;}"
    ".section{margin-top:14px;padding-top:2px;}"
    "@media (max-width:700px){"
    "body{padding:10px;}"
    ".container{padding:12px;border-radius:10px;}"
    ".page-title{font-size:20px;}"
    ".btn{display:block;width:100%;box-sizing:border-box;}"
    ".inline-form{display:block;}"
    ".actions{display:block;margin-top:8px;}"
    ".actions .btn{margin-top:6px;}"
    ".kv-grid{grid-template-columns:1fr;gap:4px;}"
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
