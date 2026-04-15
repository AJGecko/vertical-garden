#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <ctype.h>
#include <time.h>

// =====================
// Network
// =====================
const char* AP_PASSWORD = "vertical123";
const byte DNS_PORT = 53;
const uint16_t HTTP_PORT = 80;

IPAddress AP_IP(192, 168, 4, 1);
IPAddress AP_GW(192, 168, 4, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);

ESP8266WebServer server(HTTP_PORT);
DNSServer dnsServer;

String apSsid = "";
bool apStarted = false;
bool dnsStarted = false;

// =====================
// Hardware
// =====================
const uint8_t PIN_PUMP = D8;
const uint8_t PIN_MOISTURE = A0;
const uint8_t PIN_LED_STRIP = D1;
const uint16_t LED_STRIP_PIXELS = 18;

const bool PUMP_ACTIVE_HIGH = true;
const int PUMP_DURATION_MIN_MS = 1000;
const int PUMP_DURATION_MAX_MS = 120000;
const int AUTO_COOLDOWN_MIN_MS = 1000;
const int AUTO_COOLDOWN_MAX_MS = 600000;
const int AUTO_COOLDOWN_DEFAULT_MS = 15000;
const uint32_t MOISTURE_SAMPLE_INTERVAL_MS = 1000;

const int LED_EFFECT_STATIC = 0;
const int LED_EFFECT_BLINK = 1;
const int LED_EFFECT_BREATHE = 2;
const int LED_EFFECT_RAINBOW = 3;

Adafruit_NeoPixel ledStrip(LED_STRIP_PIXELS, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);

struct TimeState {
  String timezone;
  String mode;
  String localTime;
  bool liveRateEnabled;
  int liveIntervalMs;

  bool clockValid;
  unsigned long clockBaseEpochSec;
  unsigned long clockCapturedMs;
  unsigned long clockLastWriteMs;

  unsigned long lastUpdateMs;
};

struct GardenState {
  bool pumpOn;
  unsigned long pumpStopAtMs;
  int pumpDurationMs;

  bool manualPumpControlEnabled;
  bool autoPumpEnabled;
  int moistureThresholdPercent;
  int autoPumpDurationMs;
  int autoPumpCooldownMs;
  unsigned long autoPumpCooldownUntilMs;

  int moistureRaw;
  int moisturePercent;
  unsigned long lastMoistureSampleMs;

  bool ledStripOn;
  int ledR;
  int ledG;
  int ledB;
  int ledEffectMode;
  int ledEffectSpeedMs;
  unsigned long lastLedEffectTickMs;
  uint16_t ledEffectStep;

  bool lightScheduleEnabled;
  int lightOnMinute;
  int lightOffMinute;
};

TimeState timeState;
GardenState gardenState;

// =====================
// Helpers
// =====================
int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

void markStateUpdated() {
  timeState.lastUpdateMs = millis();
}

String jsonEscape(const String& input) {
  String out;
  out.reserve(input.length() + 8);
  for (unsigned int i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '\\' || c == '"') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if (c == '\t') {
      out += "\\t";
    } else {
      out += c;
    }
  }
  return out;
}

bool hasJsonKey(const String& body, const String& key) {
  return body.indexOf("\"" + key + "\"") >= 0;
}

int extractJsonInt(const String& body, const String& key, int fallback) {
  int keyPos = body.indexOf("\"" + key + "\"");
  if (keyPos < 0) return fallback;

  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;

  int pos = colonPos + 1;
  while (pos < (int)body.length() && isspace((unsigned char)body[pos])) {
    pos++;
  }

  int start = pos;
  while (pos < (int)body.length() && (isdigit((unsigned char)body[pos]) || body[pos] == '-')) {
    pos++;
  }

  if (pos <= start) return fallback;
  return body.substring(start, pos).toInt();
}

bool extractJsonBool(const String& body, const String& key, bool fallback) {
  int keyPos = body.indexOf("\"" + key + "\"");
  if (keyPos < 0) return fallback;

  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;

  int pos = colonPos + 1;
  while (pos < (int)body.length() && isspace((unsigned char)body[pos])) {
    pos++;
  }

  if (body.substring(pos).startsWith("true")) return true;
  if (body.substring(pos).startsWith("false")) return false;
  return fallback;
}

String extractJsonString(const String& body, const String& key, const String& fallback) {
  int keyPos = body.indexOf("\"" + key + "\"");
  if (keyPos < 0) return fallback;

  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;

  int firstQuote = body.indexOf('"', colonPos);
  if (firstQuote < 0) return fallback;

  String out = "";
  int i = firstQuote + 1;
  while (i < (int)body.length()) {
    char c = body[i];
    if (c == '\\') {
      i++;
      if (i >= (int)body.length()) break;
      char escaped = body[i];
      if (escaped == 'n') out += '\n';
      else if (escaped == 'r') out += '\r';
      else if (escaped == 't') out += '\t';
      else out += escaped;
      i++;
      continue;
    }

    if (c == '"') {
      return out;
    }

    out += c;
    i++;
  }

  return fallback;
}

const char* effectToText(int mode) {
  if (mode == LED_EFFECT_BLINK) return "blink";
  if (mode == LED_EFFECT_BREATHE) return "breathe";
  if (mode == LED_EFFECT_RAINBOW) return "rainbow";
  return "static";
}

int parseEffectMode(const String& mode) {
  if (mode == "static") return LED_EFFECT_STATIC;
  if (mode == "blink") return LED_EFFECT_BLINK;
  if (mode == "breathe" || mode == "breath") return LED_EFFECT_BREATHE;
  if (mode == "rainbow") return LED_EFFECT_RAINBOW;
  return -1;
}

void addCorsHeaders() {
  String origin = server.header("Origin");
  if (origin.length() > 0) {
    server.sendHeader("Access-Control-Allow-Origin", origin);
    server.sendHeader("Vary", "Origin");
  } else {
    server.sendHeader("Access-Control-Allow-Origin", "*");
  }
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
  server.sendHeader("Access-Control-Max-Age", "600");
  server.sendHeader("Access-Control-Allow-Private-Network", "true");
}

void sendJson(int code, const String& json) {
  addCorsHeaders();
  server.send(code, "application/json", json);
}

// =====================
// Time
// =====================
int monthNameToNumber(const String& mon) {
  if (mon == "Jan") return 1;
  if (mon == "Feb") return 2;
  if (mon == "Mar") return 3;
  if (mon == "Apr") return 4;
  if (mon == "May") return 5;
  if (mon == "Jun") return 6;
  if (mon == "Jul") return 7;
  if (mon == "Aug") return 8;
  if (mon == "Sep") return 9;
  if (mon == "Oct") return 10;
  if (mon == "Nov") return 11;
  if (mon == "Dec") return 12;
  return 0;
}

String buildIsoFromCompileTime() {
  String d = String(__DATE__);  // e.g. "Apr 10 2026"
  String t = String(__TIME__);  // e.g. "13:37:42"

  int month = monthNameToNumber(d.substring(0, 3));
  int day = d.substring(4, 6).toInt();
  int year = d.substring(7, 11).toInt();

  if (month <= 0 || day < 1 || day > 31 || year < 1970) {
    return String("2026-01-01T00:00:00");
  }

  char out[20];
  snprintf(out, sizeof(out), "%04d-%02d-%02dT%s", year, month, day, t.c_str());
  return String(out);
}

bool parseIsoDateTime(const String& value, int& year, int& month, int& day, int& hour, int& minute, int& second) {
  if (value.length() < 19) return false;
  if (value[4] != '-' || value[7] != '-') return false;
  if (!(value[10] == 'T' || value[10] == ' ')) return false;
  if (value[13] != ':' || value[16] != ':') return false;

  year = value.substring(0, 4).toInt();
  month = value.substring(5, 7).toInt();
  day = value.substring(8, 10).toInt();
  hour = value.substring(11, 13).toInt();
  minute = value.substring(14, 16).toInt();
  second = value.substring(17, 19).toInt();

  if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) return false;
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return false;
  return true;
}

void syncClockFromState() {
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;

  if (!parseIsoDateTime(timeState.localTime, year, month, day, hour, minute, second)) {
    timeState.clockValid = false;
    return;
  }

  tm tmValue;
  memset(&tmValue, 0, sizeof(tmValue));
  tmValue.tm_year = year - 1900;
  tmValue.tm_mon = month - 1;
  tmValue.tm_mday = day;
  tmValue.tm_hour = hour;
  tmValue.tm_min = minute;
  tmValue.tm_sec = second;

  time_t epoch = mktime(&tmValue);
  if (epoch < 0) {
    timeState.clockValid = false;
    return;
  }

  timeState.clockBaseEpochSec = (unsigned long)epoch;
  timeState.clockCapturedMs = millis();
  timeState.clockLastWriteMs = millis();
  timeState.clockValid = true;
}

void tickLocalClock() {
  if (!timeState.clockValid) return;

  unsigned long nowMs = millis();
  if (nowMs - timeState.clockLastWriteMs < 1000UL) return;

  unsigned long elapsedSec = (nowMs - timeState.clockCapturedMs) / 1000UL;
  unsigned long nowEpochSec = timeState.clockBaseEpochSec + elapsedSec;

  tm out;
  time_t tt = (time_t)nowEpochSec;
  if (!gmtime_r(&tt, &out)) return;

  char iso[20];
  snprintf(
      iso,
      sizeof(iso),
      "%04d-%02d-%02dT%02d:%02d:%02d",
      out.tm_year + 1900,
      out.tm_mon + 1,
      out.tm_mday,
      out.tm_hour,
      out.tm_min,
      out.tm_sec);

  timeState.localTime = String(iso);
  timeState.clockLastWriteMs = nowMs;
}

int getCurrentMinuteOfDay() {
  if (!timeState.clockValid) return -1;
  unsigned long elapsedSec = (millis() - timeState.clockCapturedMs) / 1000UL;
  unsigned long nowEpochSec = timeState.clockBaseEpochSec + elapsedSec;
  return (int)((nowEpochSec % 86400UL) / 60UL);
}

bool isMinuteInWindow(int current, int start, int end) {
  if (start == end) return true;
  if (start < end) return current >= start && current < end;
  return current >= start || current < end;
}

// =====================
// Pump + sensor
// =====================
void setPumpState(bool on) {
  gardenState.pumpOn = on;
  digitalWrite(PIN_PUMP, (on == PUMP_ACTIVE_HIGH) ? HIGH : LOW);
  if (!on) {
    gardenState.pumpStopAtMs = 0;
  }
}

void startPumpForMs(int durationMs, bool persistAsPreset) {
  int safeDuration = clampInt(durationMs, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
  if (persistAsPreset) {
    gardenState.pumpDurationMs = safeDuration;
  }
  setPumpState(true);
  gardenState.pumpStopAtMs = millis() + (unsigned long)safeDuration;
  markStateUpdated();
}

unsigned long getPumpRemainingMs() {
  if (!gardenState.pumpOn) return 0;

  unsigned long nowMs = millis();
  if (gardenState.pumpStopAtMs == 0 || gardenState.pumpStopAtMs <= nowMs) {
    return 0;
  }
  return gardenState.pumpStopAtMs - nowMs;
}

void sampleMoisture() {
  int raw = analogRead(PIN_MOISTURE);
  gardenState.moistureRaw = clampInt(raw, 0, 1023);
  gardenState.moisturePercent = (int)((long)gardenState.moistureRaw * 100L / 1023L);
  gardenState.lastMoistureSampleMs = millis();
}

void updateAutoPump() {
  unsigned long nowMs = millis();

  if (nowMs - gardenState.lastMoistureSampleMs >= MOISTURE_SAMPLE_INTERVAL_MS) {
    sampleMoisture();
  }

  if (!gardenState.autoPumpEnabled) return;
  if (gardenState.pumpOn) return;
  if (nowMs < gardenState.autoPumpCooldownUntilMs) return;

  if (gardenState.moisturePercent < gardenState.moistureThresholdPercent) {
    startPumpForMs(gardenState.autoPumpDurationMs, false);
    gardenState.autoPumpCooldownUntilMs = millis() + (unsigned long)gardenState.autoPumpCooldownMs;
  }
}

// =====================
// LED
// =====================
void renderLedColor(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < LED_STRIP_PIXELS; i++) {
    ledStrip.setPixelColor(i, ledStrip.Color(r, g, b));
  }
  ledStrip.show();
}

void setLedState(bool on, int r, int g, int b) {
  gardenState.ledStripOn = on;
  gardenState.ledR = clampInt(r, 0, 255);
  gardenState.ledG = clampInt(g, 0, 255);
  gardenState.ledB = clampInt(b, 0, 255);

  uint8_t outR = on ? (uint8_t)gardenState.ledR : 0;
  uint8_t outG = on ? (uint8_t)gardenState.ledG : 0;
  uint8_t outB = on ? (uint8_t)gardenState.ledB : 0;
  renderLedColor(outR, outG, outB);
}

void setLedEffectMode(int mode, int speedMs) {
  gardenState.ledEffectMode = mode;
  gardenState.ledEffectSpeedMs = clampInt(speedMs, 120, 10000);
  gardenState.lastLedEffectTickMs = 0;
  gardenState.ledEffectStep = 0;
}

void updateLedEffects() {
  if (!gardenState.ledStripOn) {
    renderLedColor(0, 0, 0);
    return;
  }

  if (gardenState.ledEffectMode == LED_EFFECT_STATIC) {
    renderLedColor((uint8_t)gardenState.ledR, (uint8_t)gardenState.ledG, (uint8_t)gardenState.ledB);
    return;
  }

  unsigned long nowMs = millis();
  unsigned long frameMs = (unsigned long)clampInt(gardenState.ledEffectSpeedMs / 20, 16, 200);
  if (nowMs - gardenState.lastLedEffectTickMs < frameMs) return;

  gardenState.lastLedEffectTickMs = nowMs;
  gardenState.ledEffectStep++;

  if (gardenState.ledEffectMode == LED_EFFECT_BLINK) {
    bool onFrame = ((gardenState.ledEffectStep / 10) % 2) == 0;
    if (onFrame) {
      renderLedColor((uint8_t)gardenState.ledR, (uint8_t)gardenState.ledG, (uint8_t)gardenState.ledB);
    } else {
      renderLedColor(0, 0, 0);
    }
    return;
  }

  if (gardenState.ledEffectMode == LED_EFFECT_BREATHE) {
    int phase = gardenState.ledEffectStep % 200;
    int wave = phase < 100 ? phase : (199 - phase);
    uint8_t factor = (uint8_t)(wave * 255 / 99);

    uint8_t r = (uint8_t)((gardenState.ledR * factor) / 255);
    uint8_t g = (uint8_t)((gardenState.ledG * factor) / 255);
    uint8_t b = (uint8_t)((gardenState.ledB * factor) / 255);
    renderLedColor(r, g, b);
    return;
  }

  if (gardenState.ledEffectMode == LED_EFFECT_RAINBOW) {
    uint16_t hue = (uint16_t)(gardenState.ledEffectStep * 256);
    uint32_t color = ledStrip.gamma32(ledStrip.ColorHSV(hue, 255, 255));
    for (uint16_t i = 0; i < LED_STRIP_PIXELS; i++) {
      ledStrip.setPixelColor(i, color);
    }
    ledStrip.show();
  }
}

void updateLightSchedule() {
  if (!gardenState.lightScheduleEnabled) return;

  int minuteOfDay = getCurrentMinuteOfDay();
  if (minuteOfDay < 0) return;

  bool shouldBeOn = isMinuteInWindow(minuteOfDay, gardenState.lightOnMinute, gardenState.lightOffMinute);
  if (shouldBeOn == gardenState.ledStripOn) return;

  setLedState(shouldBeOn, gardenState.ledR, gardenState.ledG, gardenState.ledB);
  markStateUpdated();
}

// =====================
// JSON builders
// =====================
String buildTimeJson() {
  tickLocalClock();

  String json = "{";
  json.reserve(180);
  json += "\"timezone\":\"" + jsonEscape(timeState.timezone) + "\",";
  json += "\"mode\":\"" + jsonEscape(timeState.mode) + "\",";
  json += "\"localTime\":\"" + jsonEscape(timeState.localTime) + "\",";
  json += "\"liveRateEnabled\":" + String(timeState.liveRateEnabled ? "true" : "false") + ",";
  json += "\"liveIntervalMs\":" + String(timeState.liveIntervalMs);
  json += "}";
  return json;
}

String buildStatusJson() {
  tickLocalClock();
  sampleMoisture();

  String json = "{";
  json.reserve(1300);

  json += "\"timezone\":\"" + jsonEscape(timeState.timezone) + "\",";
  json += "\"mode\":\"" + jsonEscape(timeState.mode) + "\",";
  json += "\"localTime\":\"" + jsonEscape(timeState.localTime) + "\",";
  json += "\"liveRateEnabled\":" + String(timeState.liveRateEnabled ? "true" : "false") + ",";
  json += "\"liveIntervalMs\":" + String(timeState.liveIntervalMs) + ",";

  json += "\"pumpOn\":" + String(gardenState.pumpOn ? "true" : "false") + ",";
  json += "\"pumpDurationMs\":" + String(gardenState.pumpDurationMs) + ",";
  json += "\"pumpRemainingMs\":" + String(getPumpRemainingMs()) + ",";
  json += "\"manualPumpControlEnabled\":" + String(gardenState.manualPumpControlEnabled ? "true" : "false") + ",";
  json += "\"autoPumpEnabled\":" + String(gardenState.autoPumpEnabled ? "true" : "false") + ",";
  json += "\"moistureThresholdPercent\":" + String(gardenState.moistureThresholdPercent) + ",";
  json += "\"autoPumpDurationMs\":" + String(gardenState.autoPumpDurationMs) + ",";
  json += "\"autoPumpCooldownMs\":" + String(gardenState.autoPumpCooldownMs) + ",";

  json += "\"moistureRaw\":" + String(gardenState.moistureRaw) + ",";
  json += "\"moisturePercent\":" + String(gardenState.moisturePercent) + ",";

  json += "\"ledStripOn\":" + String(gardenState.ledStripOn ? "true" : "false") + ",";
  json += "\"ledStripR\":" + String(gardenState.ledR) + ",";
  json += "\"ledStripG\":" + String(gardenState.ledG) + ",";
  json += "\"ledStripB\":" + String(gardenState.ledB) + ",";
  json += "\"ledEffect\":\"" + String(effectToText(gardenState.ledEffectMode)) + "\",";
  json += "\"ledEffectSpeedMs\":" + String(gardenState.ledEffectSpeedMs) + ",";
  json += "\"lightScheduleEnabled\":" + String(gardenState.lightScheduleEnabled ? "true" : "false") + ",";
  json += "\"lightOnMinute\":" + String(gardenState.lightOnMinute) + ",";
  json += "\"lightOffMinute\":" + String(gardenState.lightOffMinute) + ",";

  json += "\"apSsid\":\"" + jsonEscape(apSsid) + "\",";
  json += "\"apIp\":\"" + (apStarted ? WiFi.softAPIP().toString() : String("0.0.0.0")) + "\",";
  json += "\"httpPort\":" + String(HTTP_PORT) + ",";
  json += "\"uptimeMs\":" + String(millis()) + ",";
  json += "\"updatedAgoMs\":" + String(millis() - timeState.lastUpdateMs);

  json += "}";
  return json;
}

String buildSensorsJson() {
  tickLocalClock();
  sampleMoisture();

  String json = "{";
  json.reserve(900);

  json += "\"ok\":true,";
  json += "\"timezone\":\"" + jsonEscape(timeState.timezone) + "\",";
  json += "\"localTime\":\"" + jsonEscape(timeState.localTime) + "\",";

  json += "\"pump\":{";
  json += "\"on\":" + String(gardenState.pumpOn ? "true" : "false") + ",";
  json += "\"durationMs\":" + String(gardenState.pumpDurationMs) + ",";
  json += "\"remainingMs\":" + String(getPumpRemainingMs()) + ",";
  json += "\"manualEnabled\":" + String(gardenState.manualPumpControlEnabled ? "true" : "false") + ",";
  json += "\"autoEnabled\":" + String(gardenState.autoPumpEnabled ? "true" : "false") + ",";
  json += "\"thresholdPercent\":" + String(gardenState.moistureThresholdPercent) + ",";
  json += "\"autoDurationMs\":" + String(gardenState.autoPumpDurationMs) + ",";
  json += "\"autoCooldownMs\":" + String(gardenState.autoPumpCooldownMs);
  json += "},";

  json += "\"moisture\":{";
  json += "\"raw\":" + String(gardenState.moistureRaw) + ",";
  json += "\"percent\":" + String(gardenState.moisturePercent);
  json += "},";

  json += "\"ledStrip\":{";
  json += "\"on\":" + String(gardenState.ledStripOn ? "true" : "false") + ",";
  json += "\"r\":" + String(gardenState.ledR) + ",";
  json += "\"g\":" + String(gardenState.ledG) + ",";
  json += "\"b\":" + String(gardenState.ledB) + ",";
  json += "\"effect\":\"" + String(effectToText(gardenState.ledEffectMode)) + "\",";
  json += "\"effectSpeedMs\":" + String(gardenState.ledEffectSpeedMs) + ",";
  json += "\"scheduleEnabled\":" + String(gardenState.lightScheduleEnabled ? "true" : "false") + ",";
  json += "\"scheduleOnMinute\":" + String(gardenState.lightOnMinute) + ",";
  json += "\"scheduleOffMinute\":" + String(gardenState.lightOffMinute);
  json += "}";

  json += "}";
  return json;
}

String buildGardenSettingsJson() {
  String json = "{";
  json.reserve(380);

  json += "\"ok\":true,";
  json += "\"manualPumpControlEnabled\":" + String(gardenState.manualPumpControlEnabled ? "true" : "false") + ",";
  json += "\"autoPumpEnabled\":" + String(gardenState.autoPumpEnabled ? "true" : "false") + ",";
  json += "\"moistureThresholdPercent\":" + String(gardenState.moistureThresholdPercent) + ",";
  json += "\"autoPumpDurationMs\":" + String(gardenState.autoPumpDurationMs) + ",";
  json += "\"autoPumpCooldownMs\":" + String(gardenState.autoPumpCooldownMs) + ",";
  json += "\"ledEffect\":\"" + String(effectToText(gardenState.ledEffectMode)) + "\",";
  json += "\"ledEffectSpeedMs\":" + String(gardenState.ledEffectSpeedMs) + ",";
  json += "\"lightScheduleEnabled\":" + String(gardenState.lightScheduleEnabled ? "true" : "false") + ",";
  json += "\"lightOnMinute\":" + String(gardenState.lightOnMinute) + ",";
  json += "\"lightOffMinute\":" + String(gardenState.lightOffMinute);

  json += "}";
  return json;
}

// =====================
// HTTP handlers
// =====================
void handleOptions() {
  addCorsHeaders();
  server.send(204, "text/plain", "");
}

void handleRoot() {
  addCorsHeaders();

  String page = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Vertical Garden Backend</title>";
  page.reserve(1800);
  page += "<style>body{font-family:Arial,sans-serif;margin:0;padding:24px;background:#edf4f0;color:#143126;}";
  page += ".card{max-width:680px;margin:0 auto;background:#fff;border:1px solid #dce7e0;border-radius:14px;padding:20px;box-shadow:0 10px 28px rgba(12,34,24,.08);}";
  page += "h2{margin-top:0}.kv{display:flex;justify-content:space-between;gap:12px;padding:8px 0;border-bottom:1px solid #e7efea;}";
  page += ".kv:last-of-type{border-bottom:none}.k{font-weight:700}.v{font-family:monospace;background:#f5f8f6;padding:2px 8px;border-radius:6px}.hint{margin-top:14px;padding:10px;background:#f7fbf9;border:1px solid #e2eee8;border-radius:8px}</style></head><body><div class=\"card\">";
  page += "<h2>Vertical Garden Backend</h2>";
  page += "<div class=\"kv\"><span class=\"k\">SSID</span><span class=\"v\">" + apSsid + "</span></div>";
  page += "<div class=\"kv\"><span class=\"k\">IP</span><span class=\"v\">" + WiFi.softAPIP().toString() + "</span></div>";
  page += "<div class=\"kv\"><span class=\"k\">Port</span><span class=\"v\">" + String(HTTP_PORT) + "</span></div>";
  page += "<div class=\"hint\">Frontend kompatibel. Startpunkt: <strong>/api/status</strong></div>";
  page += "</div></body></html>";

  server.send(200, "text/html; charset=utf-8", page);
}

void handleGetTime() {
  sendJson(200, buildTimeJson());
}

void handleGetStatus() {
  sendJson(200, buildStatusJson());
}

void handleGetSensors() {
  sendJson(200, buildSensorsJson());
}

void handleGetGardenSettings() {
  sendJson(200, buildGardenSettingsJson());
}

void handlePostSettings() {
  String body = server.arg("plain");

  timeState.timezone = extractJsonString(body, "timezone", timeState.timezone);
  timeState.liveRateEnabled = extractJsonBool(body, "liveRateEnabled", timeState.liveRateEnabled);
  timeState.liveIntervalMs = clampInt(extractJsonInt(body, "liveIntervalMs", timeState.liveIntervalMs), 250, 60000);

  markStateUpdated();
  sendJson(200, "{\"ok\":true,\"message\":\"settings updated\"}");
}

void handlePostTime() {
  String body = server.arg("plain");

  String newTimezone = extractJsonString(body, "timezone", timeState.timezone);
  String newMode = extractJsonString(body, "mode", timeState.mode);
  String newLocalTime = extractJsonString(body, "localTime", timeState.localTime);

  if (!(newMode == "now" || newMode == "manual")) {
    newMode = "now";
  }

  timeState.timezone = newTimezone;
  timeState.mode = newMode;
  timeState.localTime = newLocalTime;
  syncClockFromState();

  markStateUpdated();
  sendJson(200, "{\"ok\":true,\"message\":\"time updated\"}");
}

void handlePostGardenSettings() {
  String body = server.arg("plain");

  bool manualDefined = hasJsonKey(body, "manualPumpControlEnabled");
  bool autoDefined = hasJsonKey(body, "autoPumpEnabled");

  bool manual = extractJsonBool(body, "manualPumpControlEnabled", gardenState.manualPumpControlEnabled);
  bool autoEnabled = extractJsonBool(body, "autoPumpEnabled", gardenState.autoPumpEnabled);

  if (manualDefined) {
    gardenState.manualPumpControlEnabled = manual;
    gardenState.autoPumpEnabled = !manual;
  } else if (autoDefined) {
    gardenState.autoPumpEnabled = autoEnabled;
    gardenState.manualPumpControlEnabled = !autoEnabled;
  }

  gardenState.moistureThresholdPercent = clampInt(extractJsonInt(body, "moistureThresholdPercent", gardenState.moistureThresholdPercent), 0, 100);
  gardenState.autoPumpDurationMs = clampInt(extractJsonInt(body, "autoPumpDurationMs", gardenState.autoPumpDurationMs), PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
  gardenState.autoPumpCooldownMs = clampInt(extractJsonInt(body, "autoPumpCooldownMs", gardenState.autoPumpCooldownMs), AUTO_COOLDOWN_MIN_MS, AUTO_COOLDOWN_MAX_MS);

  gardenState.lightScheduleEnabled = extractJsonBool(body, "lightScheduleEnabled", gardenState.lightScheduleEnabled);
  gardenState.lightOnMinute = clampInt(extractJsonInt(body, "lightOnMinute", gardenState.lightOnMinute), 0, 1439);
  gardenState.lightOffMinute = clampInt(extractJsonInt(body, "lightOffMinute", gardenState.lightOffMinute), 0, 1439);

  markStateUpdated();
  sendJson(200, buildGardenSettingsJson());
}

void handlePostPump() {
  if (!gardenState.manualPumpControlEnabled) {
    sendJson(403, "{\"ok\":false,\"message\":\"manual control disabled\"}");
    return;
  }

  String body = server.arg("plain");
  String action = extractJsonString(body, "action", "");
  bool on = extractJsonBool(body, "on", false);
  int durationMs = clampInt(extractJsonInt(body, "durationMs", gardenState.pumpDurationMs), PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);

  if (action == "start" || action == "run") {
    startPumpForMs(durationMs, true);
  } else if (action == "stop") {
    setPumpState(false);
    gardenState.pumpDurationMs = durationMs;
    markStateUpdated();
  } else if (action == "toggle") {
    if (gardenState.pumpOn) {
      setPumpState(false);
      gardenState.pumpDurationMs = durationMs;
      markStateUpdated();
    } else {
      startPumpForMs(durationMs, true);
    }
  } else {
    if (on) {
      startPumpForMs(durationMs, true);
    } else {
      setPumpState(false);
      gardenState.pumpDurationMs = durationMs;
      markStateUpdated();
    }
  }

  String json = "{\"ok\":true,\"message\":\"ok\",\"pumpOn\":";
  json += String(gardenState.pumpOn ? "true" : "false");
  json += ",\"durationMs\":" + String(gardenState.pumpDurationMs) + "}";
  sendJson(200, json);
}

void handlePostPumpDuration() {
  String body = server.arg("plain");
  gardenState.pumpDurationMs = clampInt(extractJsonInt(body, "pumpDurationMs", gardenState.pumpDurationMs), PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);

  markStateUpdated();
  String json = "{\"ok\":true,\"message\":\"ok\",\"pumpDurationMs\":" + String(gardenState.pumpDurationMs) + "}";
  sendJson(200, json);
}

void handlePostLed() {
  String body = server.arg("plain");

  bool on = extractJsonBool(body, "on", gardenState.ledStripOn);
  on = extractJsonBool(body, "ledStripOn", on);

  int r = clampInt(extractJsonInt(body, "r", gardenState.ledR), 0, 255);
  r = clampInt(extractJsonInt(body, "ledStripR", r), 0, 255);

  int g = clampInt(extractJsonInt(body, "g", gardenState.ledG), 0, 255);
  g = clampInt(extractJsonInt(body, "ledStripG", g), 0, 255);

  int b = clampInt(extractJsonInt(body, "b", gardenState.ledB), 0, 255);
  b = clampInt(extractJsonInt(body, "ledStripB", b), 0, 255);

  if (r > 0 || g > 0 || b > 0) {
    on = true;
  }

  gardenState.lightScheduleEnabled = false;
  setLedEffectMode(LED_EFFECT_STATIC, gardenState.ledEffectSpeedMs);
  setLedState(on, r, g, b);

  markStateUpdated();

  String json = "{\"ok\":true,\"message\":\"ok\",\"ledStripOn\":";
  json += String(gardenState.ledStripOn ? "true" : "false");
  json += ",\"r\":" + String(gardenState.ledR);
  json += ",\"g\":" + String(gardenState.ledG);
  json += ",\"b\":" + String(gardenState.ledB) + "}";
  sendJson(200, json);
}

void handlePostLedEffect() {
  String body = server.arg("plain");

  String effectText = extractJsonString(body, "effect", String(effectToText(gardenState.ledEffectMode)));
  int effectMode = parseEffectMode(effectText);
  if (effectMode < 0) {
    sendJson(400, "{\"ok\":false,\"message\":\"invalid effect\"}");
    return;
  }

  int speedMs = clampInt(extractJsonInt(body, "effectSpeedMs", gardenState.ledEffectSpeedMs), 120, 10000);

  bool on = extractJsonBool(body, "on", gardenState.ledStripOn);
  on = extractJsonBool(body, "ledStripOn", on);

  int r = clampInt(extractJsonInt(body, "r", gardenState.ledR), 0, 255);
  r = clampInt(extractJsonInt(body, "ledStripR", r), 0, 255);

  int g = clampInt(extractJsonInt(body, "g", gardenState.ledG), 0, 255);
  g = clampInt(extractJsonInt(body, "ledStripG", g), 0, 255);

  int b = clampInt(extractJsonInt(body, "b", gardenState.ledB), 0, 255);
  b = clampInt(extractJsonInt(body, "ledStripB", b), 0, 255);

  if (r > 0 || g > 0 || b > 0) {
    on = true;
  }

  gardenState.lightScheduleEnabled = false;
  setLedState(on, r, g, b);
  setLedEffectMode(effectMode, speedMs);
  updateLedEffects();

  markStateUpdated();

  String json = "{\"ok\":true,\"message\":\"ok\",\"ledStripOn\":";
  json += String(gardenState.ledStripOn ? "true" : "false");
  json += ",\"effect\":\"" + String(effectToText(gardenState.ledEffectMode)) + "\"";
  json += ",\"effectSpeedMs\":" + String(gardenState.ledEffectSpeedMs);
  json += ",\"r\":" + String(gardenState.ledR);
  json += ",\"g\":" + String(gardenState.ledG);
  json += ",\"b\":" + String(gardenState.ledB) + "}";
  sendJson(200, json);
}

void handleCaptiveRedirect() {
  addCorsHeaders();
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  server.send(302, "text/plain", "Redirecting to setup page");
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }

  if (server.method() == HTTP_GET && apStarted) {
    handleCaptiveRedirect();
    return;
  }

  sendJson(404, "{\"ok\":false,\"message\":\"not found\"}");
}

// =====================
// Routing
// =====================
void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/time", HTTP_OPTIONS, handleOptions);
  server.on("/api/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/sensors", HTTP_OPTIONS, handleOptions);
  server.on("/api/garden/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/pump", HTTP_OPTIONS, handleOptions);
  server.on("/api/pump-duration", HTTP_OPTIONS, handleOptions);
  server.on("/api/led", HTTP_OPTIONS, handleOptions);
  server.on("/api/led/effect", HTTP_OPTIONS, handleOptions);

  server.on("/api/time", HTTP_GET, handleGetTime);
  server.on("/api/time", HTTP_POST, handlePostTime);

  server.on("/api/settings", HTTP_POST, handlePostSettings);

  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/api/sensors", HTTP_GET, handleGetSensors);

  server.on("/api/garden/settings", HTTP_GET, handleGetGardenSettings);
  server.on("/api/garden/settings", HTTP_POST, handlePostGardenSettings);

  server.on("/api/pump", HTTP_POST, handlePostPump);
  server.on("/api/pump-duration", HTTP_POST, handlePostPumpDuration);
  server.on("/api/led", HTTP_POST, handlePostLed);
  server.on("/api/led/effect", HTTP_POST, handlePostLedEffect);

  server.on("/generate_204", HTTP_GET, handleCaptiveRedirect);
  server.on("/gen_204", HTTP_GET, handleCaptiveRedirect);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptiveRedirect);
  server.on("/connecttest.txt", HTTP_GET, handleCaptiveRedirect);
  server.on("/ncsi.txt", HTTP_GET, handleCaptiveRedirect);
  server.on("/redirect", HTTP_GET, handleCaptiveRedirect);
  server.on("/fwlink", HTTP_GET, handleCaptiveRedirect);

  server.onNotFound(handleNotFound);
}

// =====================
// Access point
// =====================
bool startAccessPoint() {
  WiFi.persistent(false);

  if (dnsStarted) {
    dnsServer.stop();
    dnsStarted = false;
  }

  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  delay(200);

  WiFi.mode(WIFI_AP);
  delay(100);

  apSsid = String("VerticalGarden-") + String(ESP.getChipId(), HEX);
  apSsid.toUpperCase();

  WiFi.softAPConfig(AP_IP, AP_GW, AP_SUBNET);

  apStarted = WiFi.softAP(apSsid.c_str(), AP_PASSWORD);
  if (!apStarted) {
    apStarted = WiFi.softAP(apSsid.c_str());
  }

  if (apStarted) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsStarted = true;
  }

  return apStarted;
}

// =====================
// Arduino lifecycle
// =====================
void setup() {
  Serial.begin(115200);
  delay(120);

  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  timeState.timezone = "Europe/Berlin";
  timeState.mode = "now";
  timeState.localTime = buildIsoFromCompileTime();
  timeState.liveRateEnabled = false;
  timeState.liveIntervalMs = 500;
  timeState.clockValid = false;
  timeState.clockBaseEpochSec = 0;
  timeState.clockCapturedMs = 0;
  timeState.clockLastWriteMs = 0;
  timeState.lastUpdateMs = millis();

  gardenState.pumpOn = false;
  gardenState.pumpStopAtMs = 0;
  gardenState.pumpDurationMs = 5000;
  gardenState.manualPumpControlEnabled = true;
  gardenState.autoPumpEnabled = false;
  gardenState.moistureThresholdPercent = 35;
  gardenState.autoPumpDurationMs = 5000;
  gardenState.autoPumpCooldownMs = AUTO_COOLDOWN_DEFAULT_MS;
  gardenState.autoPumpCooldownUntilMs = 0;
  gardenState.moistureRaw = 0;
  gardenState.moisturePercent = 0;
  gardenState.lastMoistureSampleMs = 0;
  gardenState.ledStripOn = false;
  gardenState.ledR = 255;
  gardenState.ledG = 64;
  gardenState.ledB = 12;
  gardenState.ledEffectMode = LED_EFFECT_STATIC;
  gardenState.ledEffectSpeedMs = 1200;
  gardenState.lastLedEffectTickMs = 0;
  gardenState.ledEffectStep = 0;
  gardenState.lightScheduleEnabled = false;
  gardenState.lightOnMinute = 18 * 60;
  gardenState.lightOffMinute = 23 * 60;

  pinMode(PIN_PUMP, OUTPUT);
  setPumpState(false);

  ledStrip.begin();
  ledStrip.clear();
  ledStrip.show();
  setLedState(false, gardenState.ledR, gardenState.ledG, gardenState.ledB);

  syncClockFromState();
  sampleMoisture();

  startAccessPoint();
  setupRoutes();
  server.begin();
}

void loop() {
  tickLocalClock();

  if (dnsStarted) {
    dnsServer.processNextRequest();
  }

  updateAutoPump();
  updateLightSchedule();

  if (gardenState.pumpOn && gardenState.pumpStopAtMs != 0 && millis() >= gardenState.pumpStopAtMs) {
    setPumpState(false);
    markStateUpdated();
  }

  updateLedEffects();

  server.handleClient();
  yield();
}