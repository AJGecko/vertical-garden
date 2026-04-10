#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>

const char* AP_PASSWORD = "vertical123";
const byte DNS_PORT = 53;
const uint16_t HTTP_PORT = 80;

ESP8266WebServer server(HTTP_PORT);
DNSServer dnsServer;

IPAddress apIp(192, 168, 4, 1);
IPAddress apGateway(192, 168, 4, 1);
IPAddress apSubnet(255, 255, 255, 0);

String apSsid;
bool apStarted = false;
bool dnsStarted = false;

const uint8_t PIN_PUMP = D8;
const uint8_t PIN_MOISTURE = A0;
const uint8_t PIN_BTN_MORE = D5;
const uint8_t PIN_BTN_LESS = D6;
const uint8_t PIN_BTN_ENTER = D7;
const uint8_t PIN_LED_STRIP = D1;
const uint16_t LED_STRIP_PIXELS = 1;

const bool PUMP_ACTIVE_HIGH = true;
const uint32_t BUTTON_DEBOUNCE_MS = 35;
const int PUMP_DURATION_STEP_MS = 1000;
const int PUMP_DURATION_MIN_MS = 1000;
const int PUMP_DURATION_MAX_MS = 120000;
const uint32_t SENSOR_SAMPLE_INTERVAL_MS = 1000;
const int AUTO_PUMP_COOLDOWN_MIN_MS = 1000;
const int AUTO_PUMP_COOLDOWN_MAX_MS = 600000;
const int AUTO_PUMP_COOLDOWN_DEFAULT_MS = 15000;

const int LED_EFFECT_STATIC = 0;
const int LED_EFFECT_BLINK = 1;
const int LED_EFFECT_BREATHE = 2;
const int LED_EFFECT_RAINBOW = 3;

// --- Button test system (separate toggle) ---
const bool TEST_BUTTON_SYSTEM_ENABLED = false;

Adafruit_NeoPixel ledStrip(LED_STRIP_PIXELS, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);

struct BackendState {
  String timezone = "Europe/Berlin";
  String mode = "now";
  String localTime = "1970-01-01T00:00:00";
  bool liveRateEnabled = false;
  int liveIntervalMs = 500;
  unsigned long lastUpdateMs = 0;
  bool localClockValid = false;
  unsigned long localClockBaseEpochSec = 0;
  unsigned long localClockCapturedMs = 0;
  unsigned long localClockLastWriteMs = 0;
};

BackendState state;

struct ButtonTracker {
  uint8_t pin;
  bool stablePressed;
  bool lastReadPressed;
  unsigned long lastChangeMs;
};

struct DeviceControlState {
  bool pumpOn = false;
  unsigned long pumpStopAtMs = 0;
  int pumpDurationMs = 5000;
  bool autoPumpEnabled = true;
  bool manualPumpControlEnabled = false;
  int moistureThresholdPercent = 35;
  int autoPumpDurationMs = 5000;
  int autoPumpCooldownMs = AUTO_PUMP_COOLDOWN_DEFAULT_MS;
  unsigned long pumpAutoCooldownUntilMs = 0;
  unsigned long lastMoistureSampleMs = 0;

  int moistureRaw = 0;
  int moisturePercent = 0;

  bool ledStripOn = false;
  int ledR = 0;
  int ledG = 0;
  int ledB = 0;
  int ledEffectMode = LED_EFFECT_STATIC;
  int ledEffectSpeedMs = 1200;
  unsigned long lastLedEffectTickMs = 0;
  uint16_t ledEffectStep = 0;
  bool lightScheduleEnabled = false;
  int lightOnMinute = 18 * 60;
  int lightOffMinute = 23 * 60;

  unsigned long lastButtonActionMs = 0;
  int testLightMode = 0; // 0=off, 1=white, 2=rgb
};

DeviceControlState deviceState;

ButtonTracker buttonMore = {PIN_BTN_MORE, false, false, 0};
ButtonTracker buttonLess = {PIN_BTN_LESS, false, false, 0};
ButtonTracker buttonEnter = {PIN_BTN_ENTER, false, false, 0};

const char* ledEffectModeToString(int mode) {
  if (mode == LED_EFFECT_BLINK) return "blink";
  if (mode == LED_EFFECT_BREATHE) return "breathe";
  if (mode == LED_EFFECT_RAINBOW) return "rainbow";
  return "static";
}

int parseLedEffectMode(const String& mode) {
  if (mode == "static") return LED_EFFECT_STATIC;
  if (mode == "blink") return LED_EFFECT_BLINK;
  if (mode == "breathe" || mode == "breath") return LED_EFFECT_BREATHE;
  if (mode == "rainbow") return LED_EFFECT_RAINBOW;
  return -1;
}

void renderLedColor(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < LED_STRIP_PIXELS; i++) {
    ledStrip.setPixelColor(i, ledStrip.Color(r, g, b));
  }
  ledStrip.show();
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

int clampInt(int value, int minVal, int maxVal) {
  if (value < minVal) return minVal;
  if (value > maxVal) return maxVal;
  return value;
}

bool extractJsonBool(const String& body, const String& key, bool fallback) {
  int keyPos = body.indexOf("\"" + key + "\"");
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;

  String tail = body.substring(colonPos + 1);
  tail.trim();

  if (tail.startsWith("true")) return true;
  if (tail.startsWith("false")) return false;
  return fallback;
}

int extractJsonInt(const String& body, const String& key, int fallback) {
  int keyPos = body.indexOf("\"" + key + "\"");
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;

  int pos = colonPos + 1;
  while (pos < (int)body.length() && isspace(body[pos])) pos++;

  int start = pos;
  while (pos < (int)body.length() && (isdigit(body[pos]) || body[pos] == '-')) pos++;
  if (pos <= start) return fallback;

  return body.substring(start, pos).toInt();
}

String extractJsonString(const String& body, const String& key, const String& fallback) {
  int keyPos = body.indexOf("\"" + key + "\"");
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;

  int startQuote = body.indexOf('"', colonPos);
  if (startQuote < 0) return fallback;

  int endQuote = startQuote + 1;
  while (true) {
    endQuote = body.indexOf('"', endQuote);
    if (endQuote < 0) return fallback;
    if (body[endQuote - 1] != '\\') break;
    endQuote++;
  }

  return body.substring(startQuote + 1, endQuote);
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

void sendJson(int code, const String& body) {
  addCorsHeaders();
  server.send(code, "application/json", body);
}

void setPumpState(bool on) {
  deviceState.pumpOn = on;
  digitalWrite(PIN_PUMP, (on == PUMP_ACTIVE_HIGH) ? HIGH : LOW);
  if (!on) {
    deviceState.pumpStopAtMs = 0;
  }
}

void startPumpForMs(int durationMs) {
  int safeDuration = clampInt(durationMs, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
  deviceState.pumpDurationMs = safeDuration;
  setPumpState(true);
  deviceState.pumpStopAtMs = millis() + (unsigned long)safeDuration;
  state.lastUpdateMs = millis();
}

void runPumpForMsNoPreset(int durationMs) {
  int safeDuration = clampInt(durationMs, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
  setPumpState(true);
  deviceState.pumpStopAtMs = millis() + (unsigned long)safeDuration;
  state.lastUpdateMs = millis();
}

void setLedStripState(bool on, int r, int g, int b) {
  deviceState.ledStripOn = on;
  deviceState.ledR = clampInt(r, 0, 255);
  deviceState.ledG = clampInt(g, 0, 255);
  deviceState.ledB = clampInt(b, 0, 255);

  uint8_t outR = on ? (uint8_t)deviceState.ledR : 0;
  uint8_t outG = on ? (uint8_t)deviceState.ledG : 0;
  uint8_t outB = on ? (uint8_t)deviceState.ledB : 0;

  renderLedColor(outR, outG, outB);
}

void setLedEffectMode(int mode, int speedMs) {
  deviceState.ledEffectMode = mode;
  deviceState.ledEffectSpeedMs = clampInt(speedMs, 120, 10000);
  deviceState.lastLedEffectTickMs = 0;
  deviceState.ledEffectStep = 0;
}

void updateLedEffects() {
  if (!deviceState.ledStripOn) {
    renderLedColor(0, 0, 0);
    return;
  }

  if (deviceState.ledEffectMode == LED_EFFECT_STATIC) {
    renderLedColor((uint8_t)deviceState.ledR, (uint8_t)deviceState.ledG, (uint8_t)deviceState.ledB);
    return;
  }

  unsigned long nowMs = millis();
  unsigned long frameMs = (unsigned long)clampInt(deviceState.ledEffectSpeedMs / 20, 16, 200);
  if (nowMs - deviceState.lastLedEffectTickMs < frameMs) {
    return;
  }
  deviceState.lastLedEffectTickMs = nowMs;
  deviceState.ledEffectStep++;

  if (deviceState.ledEffectMode == LED_EFFECT_BLINK) {
    bool onFrame = ((deviceState.ledEffectStep / 10) % 2) == 0;
    if (onFrame) {
      renderLedColor((uint8_t)deviceState.ledR, (uint8_t)deviceState.ledG, (uint8_t)deviceState.ledB);
    } else {
      renderLedColor(0, 0, 0);
    }
    return;
  }

  if (deviceState.ledEffectMode == LED_EFFECT_BREATHE) {
    int phase = deviceState.ledEffectStep % 200;
    int wave = phase < 100 ? phase : (199 - phase);
    uint8_t factor = (uint8_t)(wave * 255 / 99);
    uint8_t r = (uint8_t)((deviceState.ledR * factor) / 255);
    uint8_t g = (uint8_t)((deviceState.ledG * factor) / 255);
    uint8_t b = (uint8_t)((deviceState.ledB * factor) / 255);
    renderLedColor(r, g, b);
    return;
  }

  if (deviceState.ledEffectMode == LED_EFFECT_RAINBOW) {
    uint16_t hue = (uint16_t)(deviceState.ledEffectStep * 256);
    uint32_t c = ledStrip.gamma32(ledStrip.ColorHSV(hue, 255, 255));
    for (uint16_t i = 0; i < LED_STRIP_PIXELS; i++) {
      ledStrip.setPixelColor(i, c);
    }
    ledStrip.show();
  }
}

void refreshSensorValues() {
  int raw = analogRead(PIN_MOISTURE);
  deviceState.moistureRaw = clampInt(raw, 0, 1023);
  deviceState.moisturePercent = (int)((long)deviceState.moistureRaw * 100L / 1023L);
  deviceState.lastMoistureSampleMs = millis();
}

int monthNameToNumber(const String& m) {
  if (m == "Jan") return 1;
  if (m == "Feb") return 2;
  if (m == "Mar") return 3;
  if (m == "Apr") return 4;
  if (m == "May") return 5;
  if (m == "Jun") return 6;
  if (m == "Jul") return 7;
  if (m == "Aug") return 8;
  if (m == "Sep") return 9;
  if (m == "Oct") return 10;
  if (m == "Nov") return 11;
  if (m == "Dec") return 12;
  return 0;
}

bool parseIsoDateTime(const String& value, int& year, int& month, int& day, int& hour, int& minute, int& second) {
  if (value.length() < 19) {
    return false;
  }

  if (value[4] != '-' || value[7] != '-' || value[10] != 'T' || value[13] != ':' || value[16] != ':') {
    return false;
  }

  year = value.substring(0, 4).toInt();
  month = value.substring(5, 7).toInt();
  day = value.substring(8, 10).toInt();
  hour = value.substring(11, 13).toInt();
  minute = value.substring(14, 16).toInt();
  second = value.substring(17, 19).toInt();

  if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) {
    return false;
  }

  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
    return false;
  }

  return true;
}

String buildIsoFromCompileTime() {
  String d = String(__DATE__); // e.g. "Apr 10 2026"
  String t = String(__TIME__); // e.g. "13:37:42"

  String monName = d.substring(0, 3);
  int month = monthNameToNumber(monName);
  int day = d.substring(4, 6).toInt();
  int year = d.substring(7, 11).toInt();

  if (month <= 0 || year < 1970 || day < 1 || day > 31) {
    return String("2026-01-01T00:00:00");
  }

  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%s", year, month, day, t.c_str());
  return String(buf);
}

void syncLocalClockFromState() {
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  if (!parseIsoDateTime(state.localTime, year, month, day, hour, minute, second)) {
    state.localClockValid = false;
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
    state.localClockValid = false;
    return;
  }

  state.localClockBaseEpochSec = (unsigned long)epoch;
  state.localClockCapturedMs = millis();
  state.localClockLastWriteMs = millis();
  state.localClockValid = true;
}

void updateLocalTimeFromClock() {
  if (!state.localClockValid) {
    return;
  }

  unsigned long nowMs = millis();
  if (nowMs - state.localClockLastWriteMs < 1000UL) {
    return;
  }

  unsigned long elapsedSec = (nowMs - state.localClockCapturedMs) / 1000UL;
  unsigned long nowEpochSec = state.localClockBaseEpochSec + elapsedSec;

  time_t tt = (time_t)nowEpochSec;
  tm tmOut;
  if (!gmtime_r(&tt, &tmOut)) {
    return;
  }

  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
      tmOut.tm_year + 1900,
      tmOut.tm_mon + 1,
      tmOut.tm_mday,
      tmOut.tm_hour,
      tmOut.tm_min,
      tmOut.tm_sec);
  state.localTime = String(buf);
  state.localClockLastWriteMs = nowMs;
}

int getCurrentLocalMinuteOfDay() {
  if (!state.localClockValid) {
    return -1;
  }

  unsigned long elapsedSec = (millis() - state.localClockCapturedMs) / 1000UL;
  unsigned long nowEpochSec = state.localClockBaseEpochSec + elapsedSec;
  return (int)((nowEpochSec % 86400UL) / 60UL);
}

bool isMinuteInScheduleWindow(int currentMinute, int startMinute, int endMinute) {
  if (startMinute == endMinute) {
    return true;
  }

  if (startMinute < endMinute) {
    return currentMinute >= startMinute && currentMinute < endMinute;
  }

  return currentMinute >= startMinute || currentMinute < endMinute;
}

void updateLightScheduleControl() {
  if (!deviceState.lightScheduleEnabled) {
    return;
  }

  int currentMinute = getCurrentLocalMinuteOfDay();
  if (currentMinute < 0) {
    return;
  }

  bool shouldBeOn = isMinuteInScheduleWindow(currentMinute, deviceState.lightOnMinute, deviceState.lightOffMinute);
  if (deviceState.ledStripOn == shouldBeOn) {
    return;
  }

  setLedStripState(shouldBeOn, deviceState.ledR, deviceState.ledG, deviceState.ledB);
  state.lastUpdateMs = millis();
}

void updateAutoPumpControl() {
  unsigned long nowMs = millis();
  if (nowMs - deviceState.lastMoistureSampleMs >= SENSOR_SAMPLE_INTERVAL_MS) {
    refreshSensorValues();
  }

  if (!deviceState.autoPumpEnabled) {
    return;
  }

  if (deviceState.pumpOn) {
    return;
  }

  if (nowMs < deviceState.pumpAutoCooldownUntilMs) {
    return;
  }

  if (deviceState.moisturePercent < deviceState.moistureThresholdPercent) {
    runPumpForMsNoPreset(deviceState.autoPumpDurationMs);
    deviceState.pumpAutoCooldownUntilMs = millis() + (unsigned long)deviceState.autoPumpCooldownMs;
    Serial.print("[AUTO] Moisture below threshold, pump started. moisture=");
    Serial.print(deviceState.moisturePercent);
    Serial.print(" threshold=");
    Serial.print(deviceState.moistureThresholdPercent);
    Serial.print(" durationMs=");
    Serial.print(deviceState.autoPumpDurationMs);
    Serial.print(" cooldownMs=");
    Serial.println(deviceState.autoPumpCooldownMs);
  }
}

unsigned long getPumpRemainingMs() {
  if (!deviceState.pumpOn) return 0;
  unsigned long nowMs = millis();
  if (deviceState.pumpStopAtMs == 0 || deviceState.pumpStopAtMs <= nowMs) {
    return 0;
  }
  return deviceState.pumpStopAtMs - nowMs;
}

String buildControlJson() {
  refreshSensorValues();
  unsigned long remainingMs = getPumpRemainingMs();
  String j = "{";
  j += "\"ok\":true,";
  j += "\"pump\":{";
  j += "\"on\":" + String(deviceState.pumpOn ? "true" : "false") + ",";
  j += "\"durationMs\":" + String(deviceState.pumpDurationMs) + ",";
  j += "\"remainingMs\":" + String(remainingMs) + ",";
  j += "\"autoEnabled\":" + String(deviceState.autoPumpEnabled ? "true" : "false") + ",";
  j += "\"manualEnabled\":" + String(deviceState.manualPumpControlEnabled ? "true" : "false") + ",";
  j += "\"thresholdPercent\":" + String(deviceState.moistureThresholdPercent) + ",";
  j += "\"autoDurationMs\":" + String(deviceState.autoPumpDurationMs) + ",";
  j += "\"autoCooldownMs\":" + String(deviceState.autoPumpCooldownMs);
  j += "},";
  j += "\"moisture\":{";
  j += "\"raw\":" + String(deviceState.moistureRaw) + ",";
  j += "\"percent\":" + String(deviceState.moisturePercent);
  j += "},";
  j += "\"buttons\":{";
  j += "\"morePressed\":" + String(buttonMore.stablePressed ? "true" : "false") + ",";
  j += "\"lessPressed\":" + String(buttonLess.stablePressed ? "true" : "false") + ",";
  j += "\"enterPressed\":" + String(buttonEnter.stablePressed ? "true" : "false");
  j += "},";
  j += "\"ledStrip\":{";
  j += "\"on\":" + String(deviceState.ledStripOn ? "true" : "false") + ",";
  j += "\"r\":" + String(deviceState.ledR) + ",";
  j += "\"g\":" + String(deviceState.ledG) + ",";
  j += "\"b\":" + String(deviceState.ledB) + ",";
  j += "\"effect\":\"" + String(ledEffectModeToString(deviceState.ledEffectMode)) + "\",";
  j += "\"effectSpeedMs\":" + String(deviceState.ledEffectSpeedMs) + ",";
  j += "\"scheduleEnabled\":" + String(deviceState.lightScheduleEnabled ? "true" : "false") + ",";
  j += "\"scheduleOnMinute\":" + String(deviceState.lightOnMinute) + ",";
  j += "\"scheduleOffMinute\":" + String(deviceState.lightOffMinute);
  j += "}";
  j += "}";
  return j;
}

bool pollButtonPressEvent(ButtonTracker& button) {
  bool rawPressed = digitalRead(button.pin) == LOW;
  if (rawPressed != button.lastReadPressed) {
    button.lastReadPressed = rawPressed;
    button.lastChangeMs = millis();
  }

  if ((millis() - button.lastChangeMs) > BUTTON_DEBOUNCE_MS && rawPressed != button.stablePressed) {
    button.stablePressed = rawPressed;
    if (button.stablePressed) {
      return true;
    }
  }

  return false;
}

void handleButtonActions() {
  if (pollButtonPressEvent(buttonMore)) {
    deviceState.pumpDurationMs = clampInt(deviceState.pumpDurationMs + PUMP_DURATION_STEP_MS, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
    deviceState.lastButtonActionMs = millis();
    state.lastUpdateMs = millis();
  }

  if (pollButtonPressEvent(buttonLess)) {
    deviceState.pumpDurationMs = clampInt(deviceState.pumpDurationMs - PUMP_DURATION_STEP_MS, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
    deviceState.lastButtonActionMs = millis();
    state.lastUpdateMs = millis();
  }

  if (pollButtonPressEvent(buttonEnter)) {
    startPumpForMs(deviceState.pumpDurationMs);
    deviceState.lastButtonActionMs = millis();
    state.lastUpdateMs = millis();
  }
}

void applyTestLightMode() {
  if (deviceState.testLightMode == 0) {
    setLedStripState(false, 0, 0, 0);
    return;
  }

  if (deviceState.testLightMode == 1) {
    setLedStripState(true, 255, 255, 255);
    return;
  }

  setLedStripState(true, 255, 64, 12);
}

void handleButtonTestActions() {
  // D5: pump toggle test.
  if (pollButtonPressEvent(buttonMore)) {
    if (deviceState.pumpOn) {
      setPumpState(false);
    } else {
      startPumpForMs(deviceState.pumpDurationMs);
    }
    Serial.print("[TEST] Pump toggled: ");
    Serial.println(deviceState.pumpOn ? "ON" : "OFF");
    state.lastUpdateMs = millis();
  }

  // D6: light mode cycle test (off -> white -> rgb -> off).
  if (pollButtonPressEvent(buttonLess)) {
    deviceState.testLightMode = (deviceState.testLightMode + 1) % 3;
    applyTestLightMode();
    Serial.print("[TEST] Light mode: ");
    if (deviceState.testLightMode == 0) {
      Serial.println("OFF");
    } else if (deviceState.testLightMode == 1) {
      Serial.println("WHITE");
    } else {
      Serial.println("RGB");
    }
    state.lastUpdateMs = millis();
  }

  // D7: moisture print test.
  if (pollButtonPressEvent(buttonEnter)) {
    refreshSensorValues();
    Serial.print("[TEST] Moisture raw=");
    Serial.print(deviceState.moistureRaw);
    Serial.print(" percent=");
    Serial.print(deviceState.moisturePercent);
    Serial.println("%");
    state.lastUpdateMs = millis();
  }
}

String buildTimeJson() {
  updateLocalTimeFromClock();
  String j = "{";
  j += "\"timezone\":\"" + jsonEscape(state.timezone) + "\",";
  j += "\"mode\":\"" + jsonEscape(state.mode) + "\",";
  j += "\"localTime\":\"" + jsonEscape(state.localTime) + "\"";
  j += "}";
  return j;
}

String buildStatusJson() {
  updateLocalTimeFromClock();
  refreshSensorValues();
  unsigned long remainingMs = getPumpRemainingMs();

  String j = "{";
  j += "\"timezone\":\"" + jsonEscape(state.timezone) + "\",";
  j += "\"mode\":\"" + jsonEscape(state.mode) + "\",";
  j += "\"localTime\":\"" + jsonEscape(state.localTime) + "\",";
  j += "\"liveRateEnabled\":" + String(state.liveRateEnabled ? "true" : "false") + ",";
  j += "\"liveIntervalMs\":" + String(state.liveIntervalMs) + ",";
  j += "\"pumpOn\":" + String(deviceState.pumpOn ? "true" : "false") + ",";
  j += "\"pumpDurationMs\":" + String(deviceState.pumpDurationMs) + ",";
  j += "\"pumpRemainingMs\":" + String(remainingMs) + ",";
  j += "\"autoPumpEnabled\":" + String(deviceState.autoPumpEnabled ? "true" : "false") + ",";
  j += "\"manualPumpControlEnabled\":" + String(deviceState.manualPumpControlEnabled ? "true" : "false") + ",";
  j += "\"moistureThresholdPercent\":" + String(deviceState.moistureThresholdPercent) + ",";
  j += "\"autoPumpDurationMs\":" + String(deviceState.autoPumpDurationMs) + ",";
  j += "\"autoPumpCooldownMs\":" + String(deviceState.autoPumpCooldownMs) + ",";
  j += "\"moistureRaw\":" + String(deviceState.moistureRaw) + ",";
  j += "\"moisturePercent\":" + String(deviceState.moisturePercent) + ",";
  j += "\"ledStripOn\":" + String(deviceState.ledStripOn ? "true" : "false") + ",";
  j += "\"ledStripR\":" + String(deviceState.ledR) + ",";
  j += "\"ledStripG\":" + String(deviceState.ledG) + ",";
  j += "\"ledStripB\":" + String(deviceState.ledB) + ",";
  j += "\"ledEffect\":\"" + String(ledEffectModeToString(deviceState.ledEffectMode)) + "\",";
  j += "\"ledEffectSpeedMs\":" + String(deviceState.ledEffectSpeedMs) + ",";
  j += "\"lightScheduleEnabled\":" + String(deviceState.lightScheduleEnabled ? "true" : "false") + ",";
  j += "\"lightOnMinute\":" + String(deviceState.lightOnMinute) + ",";
  j += "\"lightOffMinute\":" + String(deviceState.lightOffMinute) + ",";
  j += "\"testButtonSystemEnabled\":" + String(TEST_BUTTON_SYSTEM_ENABLED ? "true" : "false") + ",";
  j += "\"testLightMode\":" + String(deviceState.testLightMode) + ",";
  j += "\"uptimeMs\":" + String(millis()) + ",";
  j += "\"updatedAgoMs\":" + String(millis() - state.lastUpdateMs) + ",";
  j += "\"apStarted\":" + String(apStarted ? "true" : "false") + ",";
  j += "\"apSsid\":\"" + jsonEscape(apSsid) + "\",";
  j += "\"apIp\":\"" + (apStarted ? WiFi.softAPIP().toString() : String("0.0.0.0")) + "\",";
  j += "\"httpPort\":" + String(HTTP_PORT);
  j += "}";
  return j;
}

bool isAccessPointHealthy() {
  if (!apStarted) return false;
  if (WiFi.softAPIP()[0] == 0) return false;
  if (WiFi.softAPSSID().length() == 0) return false;
  return true;
}

bool startAccessPoint() {
  if (apStarted) return true;

  Serial.println();
  Serial.println("========================================");
  Serial.println("[Wi-Fi] Starting access point fallback");
  Serial.println("========================================");

  WiFi.persistent(false);
  Serial.println("[Wi-Fi] WiFi.persistent(false)");

  Serial.println("[Wi-Fi] Resetting WiFi state");
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  delay(200);

  Serial.println("[Wi-Fi] Setting WiFi mode to WIFI_AP");
  WiFi.mode(WIFI_AP);
  delay(100);

  apSsid = String("VerticalGarden-") + String(ESP.getChipId(), HEX);
  apSsid.toUpperCase();
  Serial.print("[Wi-Fi] AP SSID: ");
  Serial.println(apSsid);

  Serial.print("[Wi-Fi] Configuring AP IP ");
  Serial.print(apIp);
  Serial.print(" / ");
  Serial.print(apGateway);
  Serial.print(" / ");
  Serial.println(apSubnet);

  if (!WiFi.softAPConfig(apIp, apGateway, apSubnet)) {
    Serial.println("[Wi-Fi] WARN: softAPConfig returned false");
  }

  Serial.print("[Wi-Fi] Starting soft AP with password: ");
  Serial.println(AP_PASSWORD);
  apStarted = WiFi.softAP(apSsid.c_str(), AP_PASSWORD);

  if (!apStarted) {
    Serial.println("[Wi-Fi] Password AP failed, retrying open AP without password");
    apStarted = WiFi.softAP(apSsid.c_str());
  }

  if (apStarted) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsStarted = true;
    Serial.println("[Wi-Fi] AP erfolgreich erstellt!");
    Serial.print("[Wi-Fi] Netzname: ");
    Serial.println(apSsid);
    Serial.print("[Wi-Fi] IP Adresse: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("[Wi-Fi] AP channel: ");
    Serial.println(WiFi.channel());
    Serial.print("[Wi-Fi] AP stations: ");
    Serial.println(WiFi.softAPgetStationNum());
  } else {
    Serial.println("[Wi-Fi] FEHLER: AP konnte nicht erstellt werden!");
  }

  return apStarted;
}

void handleOptions() {
  addCorsHeaders();
  server.send(204, "text/plain", "");
}

void handleRoot() {
  addCorsHeaders();
  String page = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Vertical Garden Backend</title>";
  page += "<style>body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f4f7fb;color:#1f2937;}";
  page += ".card{max-width:560px;margin:0 auto;background:#fff;border-radius:12px;padding:18px 20px;box-shadow:0 8px 24px rgba(0,0,0,.08);}";
  page += "h2{margin:0 0 10px;font-size:1.35rem;}";
  page += ".kv{display:flex;justify-content:space-between;gap:12px;padding:8px 0;border-bottom:1px solid #e5e7eb;}";
  page += ".kv:last-of-type{border-bottom:none;}";
  page += ".label{font-weight:600;color:#374151;}";
  page += ".value{font-family:monospace;background:#eef2ff;border-radius:6px;padding:2px 8px;}";
  page += ".note{margin-top:10px;font-size:.93rem;line-height:1.4;color:#374151;background:#f8fafc;border:1px solid #e5e7eb;border-radius:8px;padding:10px;}";
  page += "</style></head><body><div class=\"card\">";
  page += "<h2>" + apSsid + "</h2>";
  page += "<div class=\"kv\"><span class=\"label\">IP</span><span class=\"value\">" + WiFi.softAPIP().toString() + "</span></div>";
  page += "<div class=\"kv\"><span class=\"label\">Port</span><span class=\"value\">" + String(HTTP_PORT) + "</span></div>";
  page += "<p class=\"note\"><strong>DE:</strong> Diese Daten sind für das Panel gedacht (z.B. über app.html).<br><strong>EN:</strong> This data is intended for the panel (e.g. via app.html).</p>";
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
  sendJson(200, buildControlJson());
}

void handleGetGardenSettings() {
  String j = "{";
  j += "\"ok\":true,";
  j += "\"autoPumpEnabled\":" + String(deviceState.autoPumpEnabled ? "true" : "false") + ",";
  j += "\"manualPumpControlEnabled\":" + String(deviceState.manualPumpControlEnabled ? "true" : "false") + ",";
  j += "\"moistureThresholdPercent\":" + String(deviceState.moistureThresholdPercent) + ",";
  j += "\"autoPumpDurationMs\":" + String(deviceState.autoPumpDurationMs) + ",";
  j += "\"autoPumpCooldownMs\":" + String(deviceState.autoPumpCooldownMs) + ",";
  j += "\"ledEffect\":\"" + String(ledEffectModeToString(deviceState.ledEffectMode)) + "\",";
  j += "\"ledEffectSpeedMs\":" + String(deviceState.ledEffectSpeedMs) + ",";
  j += "\"lightScheduleEnabled\":" + String(deviceState.lightScheduleEnabled ? "true" : "false") + ",";
  j += "\"lightOnMinute\":" + String(deviceState.lightOnMinute) + ",";
  j += "\"lightOffMinute\":" + String(deviceState.lightOffMinute);
  j += "}";
  sendJson(200, j);
}

void handlePostGardenSettings() {
  String body = server.arg("plain");
  deviceState.manualPumpControlEnabled = extractJsonBool(body, "manualPumpControlEnabled", deviceState.manualPumpControlEnabled);
  // Product rule: auto mode is enabled unless manual control is active.
  deviceState.autoPumpEnabled = !deviceState.manualPumpControlEnabled;
  deviceState.moistureThresholdPercent = clampInt(extractJsonInt(body, "moistureThresholdPercent", deviceState.moistureThresholdPercent), 0, 100);
  deviceState.autoPumpDurationMs = clampInt(extractJsonInt(body, "autoPumpDurationMs", deviceState.autoPumpDurationMs), PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
  deviceState.autoPumpCooldownMs = clampInt(extractJsonInt(body, "autoPumpCooldownMs", deviceState.autoPumpCooldownMs), AUTO_PUMP_COOLDOWN_MIN_MS, AUTO_PUMP_COOLDOWN_MAX_MS);
  deviceState.lightScheduleEnabled = extractJsonBool(body, "lightScheduleEnabled", deviceState.lightScheduleEnabled);
  deviceState.lightOnMinute = clampInt(extractJsonInt(body, "lightOnMinute", deviceState.lightOnMinute), 0, 1439);
  deviceState.lightOffMinute = clampInt(extractJsonInt(body, "lightOffMinute", deviceState.lightOffMinute), 0, 1439);
  state.lastUpdateMs = millis();
  sendJson(200, "{\"ok\":true,\"message\":\"ok\",\"autoPumpEnabled\":" + String(deviceState.autoPumpEnabled ? "true" : "false") + ",\"manualPumpControlEnabled\":" + String(deviceState.manualPumpControlEnabled ? "true" : "false") + ",\"moistureThresholdPercent\":" + String(deviceState.moistureThresholdPercent) + ",\"autoPumpDurationMs\":" + String(deviceState.autoPumpDurationMs) + ",\"autoPumpCooldownMs\":" + String(deviceState.autoPumpCooldownMs) + ",\"lightScheduleEnabled\":" + String(deviceState.lightScheduleEnabled ? "true" : "false") + ",\"lightOnMinute\":" + String(deviceState.lightOnMinute) + ",\"lightOffMinute\":" + String(deviceState.lightOffMinute) + "}");
}

void handlePostPump() {
  if (!deviceState.manualPumpControlEnabled) {
    sendJson(403, "{\"ok\":false,\"message\":\"manual control disabled\"}");
    return;
  }

  String body = server.arg("plain");
  String action = extractJsonString(body, "action", "");
  bool on = extractJsonBool(body, "on", false);
  int durationMs = clampInt(extractJsonInt(body, "durationMs", deviceState.pumpDurationMs), PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);

  if (action == "start" || action == "run") {
    startPumpForMs(durationMs);
  } else if (action == "stop") {
    setPumpState(false);
  } else if (action == "toggle") {
    if (deviceState.pumpOn) {
      setPumpState(false);
    } else {
      startPumpForMs(durationMs);
    }
  } else {
    if (on) {
      startPumpForMs(durationMs);
    } else {
      setPumpState(false);
      deviceState.pumpDurationMs = durationMs;
    }
  }

  state.lastUpdateMs = millis();
  sendJson(200, "{\"ok\":true,\"message\":\"ok\",\"pumpOn\":" + String(deviceState.pumpOn ? "true" : "false") + ",\"durationMs\":" + String(deviceState.pumpDurationMs) + "}");
}

void handlePostLed() {
  String body = server.arg("plain");
  bool on = extractJsonBool(body, "on", deviceState.ledStripOn);
  on = extractJsonBool(body, "ledStripOn", on);
  int r = clampInt(extractJsonInt(body, "r", deviceState.ledR), 0, 255);
  r = clampInt(extractJsonInt(body, "ledStripR", r), 0, 255);
  int g = clampInt(extractJsonInt(body, "g", deviceState.ledG), 0, 255);
  g = clampInt(extractJsonInt(body, "ledStripG", g), 0, 255);
  int b = clampInt(extractJsonInt(body, "b", deviceState.ledB), 0, 255);
  b = clampInt(extractJsonInt(body, "ledStripB", b), 0, 255);

  // Be tolerant to stale frontend state: if any RGB channel is set, force ON.
  if (r > 0 || g > 0 || b > 0) {
    on = true;
  }

  // Manual panel action should remain controllable; disable schedule override.
  deviceState.lightScheduleEnabled = false;
  setLedEffectMode(LED_EFFECT_STATIC, deviceState.ledEffectSpeedMs);
  setLedStripState(on, r, g, b);
  state.lastUpdateMs = millis();

  sendJson(200, "{\"ok\":true,\"message\":\"ok\",\"ledStripOn\":" + String(deviceState.ledStripOn ? "true" : "false") + ",\"r\":" + String(deviceState.ledR) + ",\"g\":" + String(deviceState.ledG) + ",\"b\":" + String(deviceState.ledB) + "}");
}

void handlePostLedEffect() {
  String body = server.arg("plain");
  String modeText = extractJsonString(body, "effect", String(ledEffectModeToString(deviceState.ledEffectMode)));
  int parsedMode = parseLedEffectMode(modeText);
  if (parsedMode < 0) {
    sendJson(400, "{\"ok\":false,\"message\":\"invalid effect\"}");
    return;
  }

  int speedMs = clampInt(extractJsonInt(body, "effectSpeedMs", deviceState.ledEffectSpeedMs), 120, 10000);
  bool on = extractJsonBool(body, "on", deviceState.ledStripOn);
  on = extractJsonBool(body, "ledStripOn", on);
  int r = clampInt(extractJsonInt(body, "r", deviceState.ledR), 0, 255);
  r = clampInt(extractJsonInt(body, "ledStripR", r), 0, 255);
  int g = clampInt(extractJsonInt(body, "g", deviceState.ledG), 0, 255);
  g = clampInt(extractJsonInt(body, "ledStripG", g), 0, 255);
  int b = clampInt(extractJsonInt(body, "b", deviceState.ledB), 0, 255);
  b = clampInt(extractJsonInt(body, "ledStripB", b), 0, 255);

  if (r > 0 || g > 0 || b > 0) {
    on = true;
  }

  // Manual panel action should remain controllable; disable schedule override.
  deviceState.lightScheduleEnabled = false;
  setLedStripState(on, r, g, b);
  setLedEffectMode(parsedMode, speedMs);
  updateLedEffects();
  state.lastUpdateMs = millis();

  sendJson(200, "{\"ok\":true,\"message\":\"ok\",\"ledStripOn\":" + String(deviceState.ledStripOn ? "true" : "false") + ",\"effect\":\"" + String(ledEffectModeToString(deviceState.ledEffectMode)) + "\",\"effectSpeedMs\":" + String(deviceState.ledEffectSpeedMs) + ",\"r\":" + String(deviceState.ledR) + ",\"g\":" + String(deviceState.ledG) + ",\"b\":" + String(deviceState.ledB) + "}");
}

void handlePostDuration() {
  String body = server.arg("plain");
  int durationMs = clampInt(extractJsonInt(body, "pumpDurationMs", deviceState.pumpDurationMs), PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
  deviceState.pumpDurationMs = durationMs;
  state.lastUpdateMs = millis();
  sendJson(200, "{\"ok\":true,\"message\":\"ok\",\"pumpDurationMs\":" + String(deviceState.pumpDurationMs) + "}");
}

void handlePostSettings() {
  String body = server.arg("plain");
  state.timezone = extractJsonString(body, "timezone", state.timezone);
  state.liveRateEnabled = extractJsonBool(body, "liveRateEnabled", state.liveRateEnabled);
  state.liveIntervalMs = clampInt(extractJsonInt(body, "liveIntervalMs", state.liveIntervalMs), 250, 60000);
  state.lastUpdateMs = millis();

  Serial.println("[API] POST /api/settings");
  Serial.print("[API] timezone: ");
  Serial.println(state.timezone);
  Serial.print("[API] liveRateEnabled: ");
  Serial.println(state.liveRateEnabled ? "true" : "false");
  Serial.print("[API] liveIntervalMs: ");
  Serial.println(state.liveIntervalMs);

  sendJson(200, "{\"ok\":true,\"message\":\"settings updated\"}");
}

void handlePostTime() {
  String body = server.arg("plain");
  state.timezone = extractJsonString(body, "timezone", state.timezone);
  state.mode = extractJsonString(body, "mode", state.mode);
  state.localTime = extractJsonString(body, "localTime", state.localTime);
  syncLocalClockFromState();
  state.lastUpdateMs = millis();

  Serial.println("[API] POST /api/time");
  Serial.print("[API] timezone: ");
  Serial.println(state.timezone);
  Serial.print("[API] mode: ");
  Serial.println(state.mode);
  Serial.print("[API] localTime: ");
  Serial.println(state.localTime);

  sendJson(200, "{\"ok\":true,\"message\":\"time updated\"}");
}

void handleCaptiveRedirect() {
  addCorsHeaders();
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  server.send(302, "text/plain", "Redirecting to setup portal");
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

  sendJson(404, "{\"error\":\"not found\"}");
}

void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/time", HTTP_OPTIONS, handleOptions);
  server.on("/api/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/sensors", HTTP_OPTIONS, handleOptions);
  server.on("/api/pump", HTTP_OPTIONS, handleOptions);
  server.on("/api/led", HTTP_OPTIONS, handleOptions);
  server.on("/api/led/effect", HTTP_OPTIONS, handleOptions);
  server.on("/api/pump-duration", HTTP_OPTIONS, handleOptions);
  server.on("/api/garden/settings", HTTP_OPTIONS, handleOptions);

  server.on("/api/time", HTTP_GET, handleGetTime);
  server.on("/api/time", HTTP_POST, handlePostTime);

  server.on("/api/settings", HTTP_POST, handlePostSettings);

  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/api/sensors", HTTP_GET, handleGetSensors);
  server.on("/api/garden/settings", HTTP_GET, handleGetGardenSettings);

  server.on("/api/pump", HTTP_POST, handlePostPump);
  server.on("/api/led", HTTP_POST, handlePostLed);
  server.on("/api/led/effect", HTTP_POST, handlePostLedEffect);
  server.on("/api/pump-duration", HTTP_POST, handlePostDuration);
  server.on("/api/garden/settings", HTTP_POST, handlePostGardenSettings);

  server.on("/generate_204", HTTP_GET, handleCaptiveRedirect);
  server.on("/gen_204", HTTP_GET, handleCaptiveRedirect);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptiveRedirect);
  server.on("/connecttest.txt", HTTP_GET, handleCaptiveRedirect);
  server.on("/ncsi.txt", HTTP_GET, handleCaptiveRedirect);
  server.on("/redirect", HTTP_GET, handleCaptiveRedirect);
  server.on("/fwlink", HTTP_GET, handleCaptiveRedirect);

  server.onNotFound(handleNotFound);
}

void setup() {
  Serial.begin(115200);
  delay(120);

  Serial.println();
  Serial.println("[BOOT] Vertical Garden backend starting");
  Serial.println("[BOOT] setup() begin");

  state.lastUpdateMs = millis();

  pinMode(PIN_PUMP, OUTPUT);
  setPumpState(false);

  ledStrip.begin();
  ledStrip.clear();
  ledStrip.show();
  setLedStripState(false, 0, 0, 0);

  pinMode(PIN_BTN_MORE, INPUT_PULLUP);
  pinMode(PIN_BTN_LESS, INPUT_PULLUP);
  pinMode(PIN_BTN_ENTER, INPUT_PULLUP);

  state.localTime = buildIsoFromCompileTime();
  refreshSensorValues();
  syncLocalClockFromState();

  if (TEST_BUTTON_SYSTEM_ENABLED) {
    Serial.println("[TEST] Button test system is ENABLED");
    Serial.println("[TEST] D5: pump toggle | D6: light off/white/rgb | D7: moisture print");
  }

  startAccessPoint();

  setupRoutes();
  server.begin();
  Serial.println("[BOOT] HTTP server started on port " + String(HTTP_PORT));
}

void loop() {
  updateLocalTimeFromClock();

  if (dnsStarted) {
    dnsServer.processNextRequest();
  }

  if (TEST_BUTTON_SYSTEM_ENABLED) {
    handleButtonTestActions();
  } else {
    handleButtonActions();
  }

  updateAutoPumpControl();

  updateLightScheduleControl();

  if (deviceState.pumpOn && deviceState.pumpStopAtMs != 0 && millis() >= deviceState.pumpStopAtMs) {
    setPumpState(false);
    state.lastUpdateMs = millis();
  }

  updateLedEffects();

  server.handleClient();

  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > 10000UL) {
    lastHealthCheck = millis();

    Serial.print("[HEALTH] uptimeMs=");
    Serial.print(millis());
    Serial.print(" apStarted=");
    Serial.print(apStarted ? "true" : "false");
    Serial.print(" apIp=");
    Serial.print(WiFi.softAPIP());
    Serial.print(" stations=");
    Serial.println(WiFi.softAPgetStationNum());

    if (!isAccessPointHealthy()) {
      Serial.println("[HEALTH] AP unhealthy, restarting AP");
      apStarted = false;
      dnsStarted = false;
      startAccessPoint();
    }
  }
}
