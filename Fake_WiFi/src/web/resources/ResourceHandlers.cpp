#include "ResourceHandlers.h"

#include <WiFi.h>

#include "../../common/AppState.h"
#include "../auth/Auth.h"
#include "../shared/WebUiUtils.h"

static String buildResourceSnapshotJson() {
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t minFreeHeap = ESP.getMinFreeHeap();
  uint32_t maxAllocHeap = ESP.getMaxAllocHeap();
  uint32_t freePsram = ESP.getFreePsram();
  uint32_t psramSize = ESP.getPsramSize();
  uint32_t sketchSize = ESP.getSketchSize();
  uint32_t freeSketch = ESP.getFreeSketchSpace();
  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t flashSpeed = ESP.getFlashChipSpeed();
  uint8_t stationCount = WiFi.softAPgetStationNum();

  String json = "{";
  json += "\"uptime\":\"" + formatUptime() + "\",";
  json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\",";
  json += "\"chipRevision\":" + String(ESP.getChipRevision()) + ",";
  json += "\"stations\":" + String(stationCount) + ",";
  json += "\"heap\":{";
  json += "\"free\":" + String(freeHeap) + ",";
  json += "\"minFree\":" + String(minFreeHeap) + ",";
  json += "\"maxAlloc\":" + String(maxAllocHeap) + "},";
  json += "\"psram\":{";
  json += "\"size\":" + String(psramSize) + ",";
  json += "\"free\":" + String(freePsram) + "},";
  json += "\"flash\":{";
  json += "\"size\":" + String(flashSize) + ",";
  json += "\"speed\":" + String(flashSpeed) + "},";
  json += "\"sketch\":{";
  json += "\"size\":" + String(sketchSize) + ",";
  json += "\"free\":" + String(freeSketch) + "}";
  json += "}";
  return json;
}

void handleResources() {
  if (!ensureAuth()) return;

  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t minFreeHeap = ESP.getMinFreeHeap();
  uint32_t maxAllocHeap = ESP.getMaxAllocHeap();
  uint32_t freePsram = ESP.getFreePsram();
  uint32_t psramSize = ESP.getPsramSize();
  uint32_t sketchSize = ESP.getSketchSize();
  uint32_t freeSketch = ESP.getFreeSketchSpace();
  uint32_t flashSize = ESP.getFlashChipSize();
  uint32_t flashSpeed = ESP.getFlashChipSpeed();
  uint8_t stationCount = WiFi.softAPgetStationNum();

  String html = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>资源监控</title>";
  html += baseStyles();
  html += "</head><body><div class='container'>";
  html += "<h2 class='page-title'>资源监控</h2>";
  html += "<div class='toolbar'><a class='btn btn-muted' href='/status'>返回配置页</a><a class='btn btn-primary' href='/resources'>刷新</a><a class='btn btn-secondary' href='#' onclick='copyResourceSnapshot();return false;'>导出信息(JSON)</a></div>";
  html += "<div class='row-card'><h3 class='section-title'>运行信息</h3><div class='kv-grid'>";
  html += "<div class='kv-item'><span class='kv-label'>运行时长：</span>" + formatUptime() + "</div>";
  html += "<div class='kv-item'><span class='kv-label'>SDK 版本：</span>" + String(ESP.getSdkVersion()) + "</div>";
  html += "<div class='kv-item'><span class='kv-label'>芯片修订版本：</span>" + String(ESP.getChipRevision()) + "</div>";
  html += "<div class='kv-item'><span class='kv-label'>当前连接设备数：</span>" + String(stationCount) + "</div></div></div>";

  html += "<div class='row-card'><h3 class='section-title'>内存信息</h3><div class='kv-grid'>";
  html += "<div class='kv-item'><span class='kv-label'>可用 Heap：</span>" + formatBytes(freeHeap) + "</div>";
  html += "<div class='kv-item'><span class='kv-label'>最小剩余 Heap：</span>" + formatBytes(minFreeHeap) + "</div>";
  html += "<div class='kv-item'><span class='kv-label'>最大连续可分配 Heap：</span>" + formatBytes(maxAllocHeap) + "</div>";
  if (psramSize > 0) {
    html += "<div class='kv-item'><span class='kv-label'>PSRAM 总量：</span>" + formatBytes(psramSize) + "</div>";
    html += "<div class='kv-item'><span class='kv-label'>可用 PSRAM：</span>" + formatBytes(freePsram) + "</div>";
  } else {
    html += "<div class='kv-item'><span class='kv-label'>PSRAM：</span>未启用或硬件不支持</div>";
  }
  html += "</div></div>";

  html += "<div class='row-card'><h3 class='section-title'>程序与 Flash</h3><div class='kv-grid'>";
  html += "<div class='kv-item'><span class='kv-label'>程序已占用：</span>" + formatBytes(sketchSize) + "</div>";
  html += "<div class='kv-item'><span class='kv-label'>可用程序空间：</span>" + formatBytes(freeSketch) + "</div>";
  html += "<div class='kv-item'><span class='kv-label'>Flash 总量：</span>" + formatBytes(flashSize) + "</div>";
  html += "<div class='kv-item'><span class='kv-label'>Flash 频率：</span>" + String(flashSpeed / 1000000UL) + " MHz</div></div></div>";

  html += "<div class='note'>提示：该页面为实时快照，点击刷新可查看最新资源状态。</div>";
  html += "<script>";
  html += "async function copyResourceSnapshot(){";
  html += "try{";
  html += "const resp=await fetch('/resources/export');";
  html += "if(!resp.ok){throw new Error('HTTP '+resp.status);}";
  html += "const text=await resp.text();";
  html += "if(navigator.clipboard&&window.isSecureContext){";
  html += "await navigator.clipboard.writeText(text);";
  html += "alert('资源快照 JSON 已复制到剪贴板');";
  html += "}else{";
  html += "const ta=document.createElement('textarea');";
  html += "ta.value=text;";
  html += "document.body.appendChild(ta);";
  html += "ta.select();";
  html += "document.execCommand('copy');";
  html += "document.body.removeChild(ta);";
  html += "alert('资源快照 JSON 已复制到剪贴板');";
  html += "}";
  html += "}catch(e){";
  html += "alert('复制失败：'+e);";
  html += "}";
  html += "}";
  html += "</script>";
  html += "</div></body></html>";
  server.send(200, "text/html; charset=UTF-8", html);
}

void handleResourcesExport() {
  if (!ensureAuth()) return;

  String json = buildResourceSnapshotJson();
  server.send(200, "application/json; charset=UTF-8", json);
}
