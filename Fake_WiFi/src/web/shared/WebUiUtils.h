#pragma once

#include <Arduino.h>

String baseStyles();
String htmlEscape(const String& src);
void sendRestartPage(const String& msg);
String macDisplay();
String formatBytes(uint32_t bytes);
String formatUptime();
