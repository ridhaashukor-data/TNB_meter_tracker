#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>
#include <esp_task_wdt.h>
#include <cstring>

// Build-time config from PlatformIO env vars
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

#ifndef SCRIPT_URL
#define SCRIPT_URL ""
#endif

#ifndef DEVICE_TOKEN
#define DEVICE_TOKEN ""
#endif

#ifndef ENABLE_OTA
#define ENABLE_OTA 0
#endif

#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "tnb-meter-tracker"
#endif

#ifndef OTA_PASSWORD
#define OTA_PASSWORD ""
#endif

#ifndef PULSE_LOCKOUT_MS
#define PULSE_LOCKOUT_MS 120
#endif

// =========================
// USER CONFIG
// =========================
static const char* WIFI_SSID_CFG  = WIFI_SSID;
static const char* WIFI_PASS_CFG  = WIFI_PASS;
static const char* SCRIPT_URL_CFG = SCRIPT_URL; // https://script.google.com/macros/s/.../exec
static const char* DEVICE_TOKEN_CFG = DEVICE_TOKEN;
static const char* OTA_HOSTNAME_CFG = OTA_HOSTNAME;
static const char* OTA_PASSWORD_CFG = OTA_PASSWORD;

// Meter config
static const uint16_t IMP_PER_KWH     = 1000;   // from your meter
static const uint32_t UPLOAD_INTERVAL = 30000;  // upload every 30 seconds
static const uint16_t DEBOUNCE_MS     = 50;     // sensor debounce
static const uint16_t PULSE_LOCKOUT   = PULSE_LOCKOUT_MS; // reject unrealistically fast repeats
static const uint16_t MAX_NVS_ENTRIES = 2880;   // 24h offline @ 30s
static const float WATTS_SCALE        = (3600.0f / (UPLOAD_INTERVAL / 1000.0f)) * 1000.0f;

// Pin config
static const uint8_t SENSOR_PIN = 4; // LDR module D0 -> GPIO4

// Time config (Malaysia UTC+8)
const char* NTP_SERVER = "pool.ntp.org";
static const long GMT_OFFSET_SEC      = 8 * 3600;
static const int  DAYLIGHT_OFFSET_SEC = 0;

// Watchdog
static const uint32_t WDT_TIMEOUT_SEC = 30;

// =========================
// GLOBALS
// =========================
volatile uint32_t pulseCount = 0;
volatile uint32_t lastPulseMs = 0;

Preferences prefs;
unsigned long lastUploadMs = 0;
unsigned long lastWifiRetryMs = 0;
unsigned long lastNtpSyncMs = 0;
bool wasOffline = false;
bool otaStarted = false;
unsigned long lastOtaProgressLogMs = 0;

// =========================
// ISR
// =========================
void IRAM_ATTR pulseISR() {
  uint32_t nowMs = millis();
  uint32_t elapsed = nowMs - lastPulseMs;
  if (elapsed < DEBOUNCE_MS) return;
  if (elapsed < PULSE_LOCKOUT) return;
  pulseCount++;
  lastPulseMs = nowMs;
}

// =========================
// HELPERS
// =========================
String twoDigits(int v) {
  if (v < 10) return "0" + String(v);
  return String(v);
}

String getTimestampDDMMYYYY() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 2000)) {
    return "01-01-1970 00:00:00";
  }
  String dd = twoDigits(timeinfo.tm_mday);
  String mm = twoDigits(timeinfo.tm_mon + 1);
  String yyyy = String(timeinfo.tm_year + 1900);
  String hh = twoDigits(timeinfo.tm_hour);
  String mi = twoDigits(timeinfo.tm_min);
  String ss = twoDigits(timeinfo.tm_sec);
  return dd + "-" + mm + "-" + yyyy + " " + hh + ":" + mi + ":" + ss;
}

bool wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool hasValue(const char* s) {
  return s != nullptr && s[0] != '\0';
}

bool isHttpsUrl(const char* s) {
  return s != nullptr && std::strncmp(s, "https://", 8) == 0;
}

void connectWiFi() {
  if (wifiConnected()) return;
  if (!hasValue(WIFI_SSID_CFG) || !hasValue(WIFI_PASS_CFG)) {
    Serial.println("[CONFIG] WIFI_SSID/WIFI_PASS not set.");
    return;
  }
  Serial.println("[WiFi] Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID_CFG, WIFI_PASS_CFG);

  unsigned long start = millis();
  while (!wifiConnected() && millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (wifiConnected()) {
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Connect timeout.");
  }
}

void beginOtaIfReady() {
#if ENABLE_OTA
  if (otaStarted || !wifiConnected()) return;

  if (hasValue(OTA_HOSTNAME_CFG)) {
    ArduinoOTA.setHostname(OTA_HOSTNAME_CFG);
  }
  if (hasValue(OTA_PASSWORD_CFG)) {
    ArduinoOTA.setPassword(OTA_PASSWORD_CFG);
  }

  ArduinoOTA.onStart([]() {
    const char* type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.printf("[OTA] Start updating %s\n", type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("[OTA] Update complete.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned long now = millis();
    if (now - lastOtaProgressLogMs >= 1000 || progress == total) {
      lastOtaProgressLogMs = now;
      unsigned int percent = (total == 0) ? 0 : (progress * 100U / total);
      Serial.printf("[OTA] Progress: %u%%\n", percent);
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]\n", (unsigned int)error);
  });

  ArduinoOTA.begin();
  otaStarted = true;
  Serial.print("[OTA] Ready. Hostname: ");
  Serial.println(hasValue(OTA_HOSTNAME_CFG) ? OTA_HOSTNAME_CFG : "esp32");
#endif
}

void syncTimeIfNeeded(bool force = false) {
  if (!force && millis() - lastNtpSyncMs < 24UL * 3600UL * 1000UL) return;
  if (!wifiConnected()) return;

  Serial.println("[NTP] Syncing time...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) {
    Serial.println("[NTP] Time sync OK.");
    lastNtpSyncMs = millis();
  } else {
    Serial.println("[NTP] Time sync failed.");
  }
}

bool postReading(const String& timestamp, uint32_t pulses, float kwh, float watts, const char* source) {
  if (!wifiConnected()) return false;
  if (!hasValue(SCRIPT_URL_CFG)) {
    Serial.println("[CONFIG] SCRIPT_URL not set.");
    return false;
  }
  if (!isHttpsUrl(SCRIPT_URL_CFG)) {
    Serial.println("[CONFIG] SCRIPT_URL must use https://");
    return false;
  }
  if (!hasValue(DEVICE_TOKEN_CFG)) {
    Serial.println("[CONFIG] DEVICE_TOKEN not set.");
    return false;
  }

  HTTPClient http;
  http.setTimeout(5000);
  http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
  http.begin(SCRIPT_URL_CFG);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Token", DEVICE_TOKEN_CFG);

  JsonDocument doc;
  doc["timestamp"]   = timestamp;     // DD-MM-YYYY HH:MM:SS
  doc["pulse_count"] = pulses;
  doc["kwh"]         = kwh;
  doc["watts"]       = watts;
  doc["source"]      = source;
  doc["token"]       = DEVICE_TOKEN_CFG;

  String body;
  serializeJson(doc, body);

  int code = http.POST(body);
  String resp;

  // Google Apps Script returns 302 redirect; follow it manually with a clean GET
  if (code == 301 || code == 302) {
    String redirectUrl = http.getLocation();
    http.end();

    if (redirectUrl.length() > 0) {
      Serial.printf("[HTTP] Redirect -> %s\n", redirectUrl.c_str());
      HTTPClient http2;
      http2.setTimeout(5000);
      http2.begin(redirectUrl);
      code = http2.GET();
      resp = http2.getString();
      http2.end();
    }
  } else {
    resp = http.getString();
    http.end();
  }

  resp.trim();
  bool okHttp = (code >= 200 && code < 300);
  bool okBody = resp.equalsIgnoreCase("ok");

  Serial.printf("[HTTP] code=%d, resp=%s\n", code, resp.c_str());
  return okHttp && okBody;
}

// NVS keys
String entryKey(uint16_t idx) {
  return "entry_" + String(idx);
}

uint16_t getNvsCount() {
  return prefs.getUShort("count", 0);
}

void setNvsCount(uint16_t c) {
  prefs.putUShort("count", c);
}

void saveOfflineToNvs(const String& timestamp, uint32_t pulses) {
  uint16_t count = getNvsCount();
  if (count >= MAX_NVS_ENTRIES) {
    Serial.println("[NVS] Buffer full. Dropping oldest and shifting...");
    // Shift left by 1 (simple safe method; okay at this scale)
    for (uint16_t i = 1; i < count; i++) {
      String val = prefs.getString(entryKey(i).c_str(), "");
      prefs.putString(entryKey(i - 1).c_str(), val);
    }
    count = MAX_NVS_ENTRIES - 1;
  }

  String line = timestamp + "," + String(pulses);
  prefs.putString(entryKey(count).c_str(), line);
  setNvsCount(count + 1);
  Serial.printf("[NVS] Saved offline entry idx=%u val=%s\n", count, line.c_str());
}

void flushNvsIfOnline() {
  if (!wifiConnected()) return;

  uint16_t count = getNvsCount();
  if (count == 0) return;

  Serial.printf("[NVS] Flushing %u entries...\n", count);

  for (uint16_t i = 0; i < count; i++) {
    String line = prefs.getString(entryKey(i).c_str(), "");
    if (line.length() == 0) continue;

    int comma = line.lastIndexOf(',');
    if (comma <= 0) continue;

    String ts = line.substring(0, comma);
    uint32_t pulses = (uint32_t) line.substring(comma + 1).toInt();

    float kwh = (float)pulses / (float)IMP_PER_KWH;  // pulses/1000
    float watts = kwh * WATTS_SCALE; // interval avg W

    bool ok = postReading(ts, pulses, kwh, watts, "nvs_recovery");
    if (!ok) {
      Serial.println("[NVS] Flush stopped (post failed).");
      return; // stop and retry later
    }
    esp_task_wdt_reset();
  }

  // Clear only after full success
  for (uint16_t i = 0; i < count; i++) {
    prefs.remove(entryKey(i).c_str());
  }
  setNvsCount(0);
  Serial.println("[NVS] Flush complete and cleared.");
}

void processInterval() {
  // Atomic read+reset pulse counter
  noInterrupts();
  uint32_t pulses = pulseCount;
  pulseCount = 0;
  interrupts();

  String ts = getTimestampDDMMYYYY();
  float kwh = (float)pulses / (float)IMP_PER_KWH; // 1 pulse = 0.001 kWh
  float watts = kwh * WATTS_SCALE;

  Serial.printf("[DATA] %s | pulses=%lu | kWh=%.6f | W=%.2f\n",
                ts.c_str(), (unsigned long)pulses, kwh, watts);

  if (wifiConnected()) {
    bool ok = postReading(ts, pulses, kwh, watts, "live");
    if (!ok) {
      saveOfflineToNvs(ts, pulses);
      wasOffline = true;
    }
  } else {
    saveOfflineToNvs(ts, pulses);
    wasOffline = true;
  }
}

// =========================
// SETUP / LOOP
// =========================
void setup() {
  Serial.begin(115200);
  delay(300);

  // WDT init
  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);

  // NVS init
  prefs.begin("tnb_tracker", false);

  // Sensor pin
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), pulseISR, FALLING);

  // Wi-Fi + time
  connectWiFi();
  beginOtaIfReady();
  syncTimeIfNeeded(true);

  // Try flush any old offline data at boot
  flushNvsIfOnline();

  lastUploadMs = millis();
  Serial.println("[BOOT] Ready.");
}

void loop() {
  esp_task_wdt_reset();

  // Keep Wi-Fi alive
  if (!wifiConnected() && millis() - lastWifiRetryMs > 5000) {
    lastWifiRetryMs = millis();
    connectWiFi();
  }

  beginOtaIfReady();

#if ENABLE_OTA
  if (otaStarted) {
    ArduinoOTA.handle();
  }
#endif

  // Sync NTP periodically
  syncTimeIfNeeded(false);

  // If recovered from offline, flush backlog first
  if (wifiConnected() && wasOffline) {
    flushNvsIfOnline();
    if (getNvsCount() == 0) wasOffline = false;
  }

  // Periodic upload interval
  if (millis() - lastUploadMs >= UPLOAD_INTERVAL) {
    lastUploadMs = millis();
    processInterval();
  }

  delay(20);
}