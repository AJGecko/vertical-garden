#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <math.h>

// -----------------------------
// Network configuration
// -----------------------------
const char* WIFI_SSID = "";        // Optional: set to join your router
const char* WIFI_PASSWORD = "";    // Optional: set to join your router
const char* AP_SSID = "VerticalGarden";
const char* AP_PASSWORD = "garden123";

// -----------------------------
// Pin mapping
// -----------------------------
const uint8_t PIN_PUMP = D8;
const uint8_t PIN_MOISTURE = A0;
const uint8_t PIN_BUTTON_PUMP = D5;
const uint8_t PIN_BUTTON_LED = D6;
const uint8_t PIN_BUTTON_EFFECT = D7;
const uint8_t PIN_LED_STRIP = D1;

// Keep true for common relay modules where HIGH means inactive.
// Set false if your pump driver is active LOW.
const bool PUMP_ACTIVE_HIGH = true;

const uint16_t HTTP_PORT = 80;
const uint16_t LED_COUNT = 18;
const uint8_t LED_GLOBAL_BRIGHTNESS = 48;
const size_t JSON_TX_BUFFER_SIZE = 2304;
const byte DNS_PORT = 53;
const bool ENABLE_CAPTIVE_DNS = false;
const bool ENABLE_STA = false;
const uint32_t SERIAL_BAUD = 115200;
const uint32_t RUNTIME_LOG_INTERVAL_MS = 10000;

const uint8_t AP_CHANNEL = 6;
const bool AP_HIDDEN = false;
const uint8_t AP_MAX_CONNECTIONS = 4;
const uint32_t NETWORK_CHECK_INTERVAL_MS = 5000;
const uint32_t STA_RETRY_INTERVAL_MS = 15000;
const uint8_t AP_HEALTH_MISS_LIMIT = 6;
const uint32_t AP_RECOVER_MIN_INTERVAL_MS = 30000;

const uint16_t BUTTON_DEBOUNCE_MS = 40;
const uint16_t SENSOR_SAMPLE_INTERVAL_MS = 300;
const uint16_t LED_FRAME_INTERVAL_MS = 80;
const uint16_t LED_IDLE_FRAME_INTERVAL_MS = 200;
const uint16_t STATUS_CACHE_INTERVAL_MS = 1000;
const uint32_t HEAP_GUARD_INTERVAL_MS = 3000;
const uint32_t HEAP_RECOVER_COOLDOWN_MS = 60000;
const uint16_t HEAP_SOFT_RECOVER_THRESHOLD = 9000;
const uint16_t HEAP_HARD_RESTART_THRESHOLD = 5000;

// Adjust these values to calibrate your sensor.
const int MOISTURE_RAW_DRY = 860;
const int MOISTURE_RAW_WET = 420;

ESP8266WebServer server(HTTP_PORT);
DNSServer dnsServer;
Adafruit_NeoPixel strip(LED_COUNT, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);
char jsonTxBuffer[JSON_TX_BUFFER_SIZE];
char statusJsonCache[JSON_TX_BUFFER_SIZE];
StaticJsonDocument<2048> statusJsonDoc;
bool staEnabled = false;
bool dnsStarted = false;
uint32_t lastNetworkCheckMs = 0;
uint32_t lastStaRetryMs = 0;
uint32_t lastApRecoverMs = 0;
uint8_t apHealthMisses = 0;
uint32_t lastRuntimeLogMs = 0;
uint32_t lastStatusCacheMs = 0;
bool statusCacheReady = false;
uint32_t lastHeapGuardMs = 0;
uint32_t lastHeapRecoverMs = 0;

enum LedEffect {
  LED_EFFECT_STATIC,
  LED_EFFECT_BLINK,
  LED_EFFECT_BREATHE,
  LED_EFFECT_RAINBOW
};

struct ControllerSettings {
  String timezone = "Europe/Berlin";
  bool liveRateEnabled = false;
  uint32_t liveIntervalMs = 500;
};

struct ClockState {
  bool synced = false;
  int year = 2026;
  int month = 1;
  int day = 1;
  int hour = 0;
  int minute = 0;
  int second = 0;
  uint32_t lastTickMs = 0;
};

struct PumpState {
  bool on = false;
  uint32_t endAtMs = 0;
  uint32_t durationMs = 5000;
  bool manualPumpControlEnabled = true;
  uint8_t moistureThresholdPercent = 35;
  uint32_t autoDurationMs = 5000;
  uint32_t autoCooldownMs = 15000;
  uint32_t lastAutoTriggerMs = 0;
};

struct LedState {
  bool userOn = false;
  bool effectiveOn = false;
  uint8_t r = 255;
  uint8_t g = 64;
  uint8_t b = 12;
  LedEffect effect = LED_EFFECT_STATIC;
  uint32_t effectSpeedMs = 1200;
  bool scheduleEnabled = false;
  uint16_t onMinute = 18 * 60;
  uint16_t offMinute = 23 * 60;
};

struct ButtonState {
  uint8_t pin;
  bool stableState;
  bool lastReading;
  uint32_t changedAtMs;
};

ControllerSettings settings;
ClockState clockState;
PumpState pumpState;
LedState ledState;

ButtonState buttons[3] = {
  {PIN_BUTTON_PUMP, HIGH, HIGH, 0},
  {PIN_BUTTON_LED, HIGH, HIGH, 0},
  {PIN_BUTTON_EFFECT, HIGH, HIGH, 0}
};

int moistureRaw = 0;
uint8_t moisturePercent = 0;
uint32_t lastSensorSampleMs = 0;
uint32_t lastLedFrameMs = 0;
bool ledOutputInitialized = false;
bool lastRenderedOn = false;
LedEffect lastRenderedEffect = LED_EFFECT_STATIC;
uint8_t lastRenderedR = 0;
uint8_t lastRenderedG = 0;
uint8_t lastRenderedB = 0;

// -----------------------------
// Utility helpers
// -----------------------------
bool elapsedSince(uint32_t sinceMs, uint32_t durationMs) {
  return (uint32_t)(millis() - sinceMs) >= durationMs;
}

uint32_t clampU32(uint32_t value, uint32_t minV, uint32_t maxV) {
  if (value < minV) return minV;
  if (value > maxV) return maxV;
  return value;
}

int clampI32(int value, int minV, int maxV) {
  if (value < minV) return minV;
  if (value > maxV) return maxV;
  return value;
}

String toLowerCopy(const String& input) {
  String out = input;
  out.toLowerCase();
  return out;
}

bool parseBoolFlexible(JsonVariantConst value, bool fallback) {
  if (value.isNull()) return fallback;
  if (value.is<bool>()) return value.as<bool>();
  if (value.is<int>()) return value.as<int>() != 0;
  if (value.is<long>()) return value.as<long>() != 0;
  if (value.is<const char*>()) {
    String v = toLowerCopy(String(value.as<const char*>()));
    if (v == "true" || v == "1" || v == "on") return true;
    if (v == "false" || v == "0" || v == "off") return false;
  }
  return fallback;
}

int parseIntFlexible(JsonVariantConst value, int fallback) {
  if (value.isNull()) return fallback;
  if (value.is<int>()) return value.as<int>();
  if (value.is<long>()) return (int)value.as<long>();
  if (value.is<float>()) return (int)value.as<float>();
  if (value.is<const char*>()) return atoi(value.as<const char*>());
  return fallback;
}

String parseStringFlexible(JsonVariantConst value, const String& fallback) {
  if (value.isNull()) return fallback;
  if (value.is<const char*>()) return String(value.as<const char*>());
  if (value.is<String>()) return value.as<String>();
  return fallback;
}

void invalidateStatusCache() {
  statusCacheReady = false;
  lastStatusCacheMs = 0;
}

// -----------------------------
// Clock handling
// -----------------------------
bool isLeapYear(int year) {
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  return year % 4 == 0;
}

int daysInMonth(int year, int month) {
  static const int daysPerMonth[12] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
  };
  if (month == 2) {
    return isLeapYear(year) ? 29 : 28;
  }
  if (month < 1 || month > 12) return 30;
  return daysPerMonth[month - 1];
}

void tickClockOneSecond() {
  clockState.second++;
  if (clockState.second < 60) return;

  clockState.second = 0;
  clockState.minute++;
  if (clockState.minute < 60) return;

  clockState.minute = 0;
  clockState.hour++;
  if (clockState.hour < 24) return;

  clockState.hour = 0;
  clockState.day++;

  const int dim = daysInMonth(clockState.year, clockState.month);
  if (clockState.day <= dim) return;

  clockState.day = 1;
  clockState.month++;
  if (clockState.month <= 12) return;

  clockState.month = 1;
  clockState.year++;
}

void updateClock() {
  if (!clockState.synced) return;
  while (elapsedSince(clockState.lastTickMs, 1000)) {
    clockState.lastTickMs += 1000;
    tickClockOneSecond();
  }
}

bool parseIsoLocalTime(const String& localTime,
                       int& year,
                       int& month,
                       int& day,
                       int& hour,
                       int& minute,
                       int& second) {
  // Expected: YYYY-MM-DDTHH:MM:SS
  if (localTime.length() < 19) return false;

  int y, mo, d, h, mi, s;
  int parsed = sscanf(localTime.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s);
  if (parsed != 6) {
    parsed = sscanf(localTime.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s);
    if (parsed != 6) return false;
  }

  if (mo < 1 || mo > 12) return false;
  if (d < 1 || d > daysInMonth(y, mo)) return false;
  if (h < 0 || h > 23) return false;
  if (mi < 0 || mi > 59) return false;
  if (s < 0 || s > 59) return false;

  year = y;
  month = mo;
  day = d;
  hour = h;
  minute = mi;
  second = s;
  return true;
}

String getLocalTimeIso() {
  if (!clockState.synced) return "-";
  char buffer[20];
  snprintf(
    buffer,
    sizeof(buffer),
    "%04d-%02d-%02dT%02d:%02d:%02d",
    clockState.year,
    clockState.month,
    clockState.day,
    clockState.hour,
    clockState.minute,
    clockState.second
  );
  return String(buffer);
}

uint16_t currentMinuteOfDay() {
  if (!clockState.synced) return 0;
  return (uint16_t)(clockState.hour * 60 + clockState.minute);
}

// -----------------------------
// Pump and moisture handling
// -----------------------------
void setPumpOutput(bool on) {
  const uint8_t activeLevel = PUMP_ACTIVE_HIGH ? HIGH : LOW;
  const uint8_t inactiveLevel = PUMP_ACTIVE_HIGH ? LOW : HIGH;
  digitalWrite(PIN_PUMP, on ? activeLevel : inactiveLevel);
}

void stopPump() {
  pumpState.on = false;
  pumpState.endAtMs = 0;
  setPumpOutput(false);
  invalidateStatusCache();
}

void startPump(uint32_t durationMs, bool isAutoTrigger) {
  uint32_t safeDuration = clampU32(durationMs, 1000, 120000);
  pumpState.on = true;
  pumpState.endAtMs = millis() + safeDuration;
  setPumpOutput(true);
  if (isAutoTrigger) {
    pumpState.lastAutoTriggerMs = millis();
  }
  invalidateStatusCache();
}

uint32_t pumpRemainingMs() {
  if (!pumpState.on) return 0;
  if ((int32_t)(millis() - pumpState.endAtMs) >= 0) return 0;
  return (uint32_t)(pumpState.endAtMs - millis());
}

void updatePumpRuntime() {
  if (!pumpState.on) return;
  if ((int32_t)(millis() - pumpState.endAtMs) >= 0) {
    stopPump();
  }
}

uint8_t moistureRawToPercent(int rawValue) {
  int constrained = clampI32(rawValue, MOISTURE_RAW_WET, MOISTURE_RAW_DRY);
  float normalized = (float)(MOISTURE_RAW_DRY - constrained) / (float)(MOISTURE_RAW_DRY - MOISTURE_RAW_WET);
  int percent = (int)roundf(normalized * 100.0f);
  return (uint8_t)clampI32(percent, 0, 100);
}

void sampleMoisture() {
  if (!elapsedSince(lastSensorSampleMs, SENSOR_SAMPLE_INTERVAL_MS)) return;
  lastSensorSampleMs = millis();

  long total = 0;
  const int samples = 4;
  for (int i = 0; i < samples; i++) {
    total += analogRead(PIN_MOISTURE);
    yield();
  }

  moistureRaw = (int)(total / samples);
  moisturePercent = moistureRawToPercent(moistureRaw);
}

void updateAutoPump() {
  if (pumpState.manualPumpControlEnabled) return;
  if (pumpState.on) return;

  bool isDry = moisturePercent < pumpState.moistureThresholdPercent;
  bool cooldownReady = elapsedSince(pumpState.lastAutoTriggerMs, pumpState.autoCooldownMs);

  if (isDry && cooldownReady) {
    startPump(pumpState.autoDurationMs, true);
  }
}

// -----------------------------
// LED handling
// -----------------------------
bool isWithinScheduleWindow(uint16_t nowMinute, uint16_t onMinute, uint16_t offMinute) {
  if (onMinute == offMinute) return true;
  if (onMinute < offMinute) {
    return nowMinute >= onMinute && nowMinute < offMinute;
  }
  return nowMinute >= onMinute || nowMinute < offMinute;
}

LedEffect parseEffect(const String& effectName) {
  String normalized = toLowerCopy(effectName);
  if (normalized == "blink") return LED_EFFECT_BLINK;
  if (normalized == "breathe") return LED_EFFECT_BREATHE;
  if (normalized == "rainbow") return LED_EFFECT_RAINBOW;
  return LED_EFFECT_STATIC;
}

const char* effectToString(LedEffect effect) {
  switch (effect) {
    case LED_EFFECT_BLINK:
      return "blink";
    case LED_EFFECT_BREATHE:
      return "breathe";
    case LED_EFFECT_RAINBOW:
      return "rainbow";
    case LED_EFFECT_STATIC:
    default:
      return "static";
  }
}

uint32_t wheelColor(uint8_t wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}

void fillStrip(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
}

void showStripSafely() {
  strip.show();
  yield();
}

void rememberSolidFrame(bool on, LedEffect effect, uint8_t r, uint8_t g, uint8_t b) {
  ledOutputInitialized = true;
  lastRenderedOn = on;
  lastRenderedEffect = effect;
  lastRenderedR = r;
  lastRenderedG = g;
  lastRenderedB = b;
}

void clearStripIfNeeded() {
  if (ledOutputInitialized && !lastRenderedOn) {
    return;
  }

  fillStrip(0, 0, 0);
  showStripSafely();
  rememberSolidFrame(false, LED_EFFECT_STATIC, 0, 0, 0);
}

void applySolidIfChanged(LedEffect effect, uint8_t r, uint8_t g, uint8_t b) {
  bool unchanged =
    ledOutputInitialized &&
    lastRenderedOn &&
    lastRenderedEffect == effect &&
    lastRenderedR == r &&
    lastRenderedG == g &&
    lastRenderedB == b;

  if (unchanged) {
    return;
  }

  fillStrip(r, g, b);
  showStripSafely();
  rememberSolidFrame(true, effect, r, g, b);
}

void renderLedEffect() {
  uint16_t frameInterval = LED_FRAME_INTERVAL_MS;
  if (WiFi.softAPgetStationNum() == 0) {
    frameInterval = LED_IDLE_FRAME_INTERVAL_MS;
  }

  if (!elapsedSince(lastLedFrameMs, frameInterval)) return;
  lastLedFrameMs = millis();

  bool shouldBeOn = ledState.userOn;
  if (ledState.scheduleEnabled && clockState.synced) {
    shouldBeOn = isWithinScheduleWindow(currentMinuteOfDay(), ledState.onMinute, ledState.offMinute);
  }
  ledState.effectiveOn = shouldBeOn;

  if (!ledState.effectiveOn) {
    clearStripIfNeeded();
    return;
  }

  uint32_t speedMs = clampU32(ledState.effectSpeedMs, 120, 10000);
  uint8_t frameR = ledState.r;
  uint8_t frameG = ledState.g;
  uint8_t frameB = ledState.b;
  bool frameOn = true;

  switch (ledState.effect) {
    case LED_EFFECT_BLINK: {
      uint32_t halfCycle = speedMs / 2;
      if (halfCycle < 60) {
        halfCycle = 60;
      }
      bool onPhase = ((millis() / halfCycle) % 2) == 0;
      frameOn = onPhase;
      break;
    }
    case LED_EFFECT_BREATHE: {
      float phase = (float)(millis() % speedMs) / (float)speedMs;
      float wave = (sinf((phase * 2.0f * PI) - (PI / 2.0f)) + 1.0f) * 0.5f;
      frameR = (uint8_t)(ledState.r * wave);
      frameG = (uint8_t)(ledState.g * wave);
      frameB = (uint8_t)(ledState.b * wave);
      break;
    }
    case LED_EFFECT_RAINBOW: {
      uint32_t base = (millis() * 256UL) / speedMs;
      for (uint16_t i = 0; i < LED_COUNT; i++) {
        uint8_t idx = (uint8_t)((i * 256 / LED_COUNT + base) & 255);
        strip.setPixelColor(i, wheelColor(idx));
      }
      showStripSafely();
      rememberSolidFrame(true, LED_EFFECT_RAINBOW, 0, 0, 0);
      break;
    }
    case LED_EFFECT_STATIC:
    default:
      break;
  }

  if (ledState.effect == LED_EFFECT_RAINBOW) {
    return;
  }

  if (!frameOn) {
    clearStripIfNeeded();
    return;
  }

  applySolidIfChanged(ledState.effect, frameR, frameG, frameB);
}

void cycleLedEffect() {
  switch (ledState.effect) {
    case LED_EFFECT_STATIC:
      ledState.effect = LED_EFFECT_BLINK;
      break;
    case LED_EFFECT_BLINK:
      ledState.effect = LED_EFFECT_BREATHE;
      break;
    case LED_EFFECT_BREATHE:
      ledState.effect = LED_EFFECT_RAINBOW;
      break;
    case LED_EFFECT_RAINBOW:
    default:
      ledState.effect = LED_EFFECT_STATIC;
      break;
  }
}

// -----------------------------
// Button handling
// -----------------------------
void handleButtonPress(uint8_t index) {
  if (index == 0) {
    if (pumpState.on) {
      stopPump();
    } else {
      startPump(pumpState.durationMs, false);
    }
    return;
  }

  if (index == 1) {
    ledState.userOn = !ledState.userOn;
    invalidateStatusCache();
    return;
  }

  if (index == 2) {
    ledState.userOn = true;
    cycleLedEffect();
    invalidateStatusCache();
  }
}

void setupButtons() {
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
    bool current = digitalRead(buttons[i].pin);
    buttons[i].stableState = current;
    buttons[i].lastReading = current;
    buttons[i].changedAtMs = millis();
  }
}

void pollButtons() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < 3; i++) {
    bool reading = digitalRead(buttons[i].pin);
    if (reading != buttons[i].lastReading) {
      buttons[i].lastReading = reading;
      buttons[i].changedAtMs = now;
    }

    if ((uint32_t)(now - buttons[i].changedAtMs) >= BUTTON_DEBOUNCE_MS && reading != buttons[i].stableState) {
      buttons[i].stableState = reading;
      if (buttons[i].stableState == LOW) {
        handleButtonPress(i);
      }
    }
  }
}

// -----------------------------
// HTTP helpers
// -----------------------------
void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Connection", "close");
}

void sendJson(int statusCode, JsonDocument& doc) {
  addCorsHeaders();
  size_t needed = measureJson(doc);
  if (needed >= JSON_TX_BUFFER_SIZE) {
    server.send(500, "application/json", "{\"ok\":false,\"message\":\"json overflow\"}");
    return;
  }

  serializeJson(doc, jsonTxBuffer, JSON_TX_BUFFER_SIZE);
  server.send(statusCode, "application/json", jsonTxBuffer);
}

void sendErrorJson(int statusCode, const String& message) {
  DynamicJsonDocument doc(256);
  doc["ok"] = false;
  doc["message"] = message;
  sendJson(statusCode, doc);
}

bool parseJsonBody(JsonDocument& doc) {
  if (!server.hasArg("plain")) {
    return false;
  }
  doc.clear();
  DeserializationError error = deserializeJson(doc, server.arg("plain"));
  return !error;
}

String activeSsid() {
  if (WiFi.status() == WL_CONNECTED && WiFi.SSID().length() > 0) {
    return WiFi.SSID();
  }
  return String(AP_SSID);
}

String activeIp() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return WiFi.softAPIP().toString();
}

void buildStatusJson(JsonDocument& doc) {
  doc["timezone"] = settings.timezone;
  doc["localTime"] = getLocalTimeIso();
  doc["apSsid"] = activeSsid();
  doc["apIp"] = activeIp();
  doc["httpPort"] = HTTP_PORT;

  JsonObject pump = doc.createNestedObject("pump");
  pump["on"] = pumpState.on;
  pump["durationMs"] = pumpState.durationMs;
  pump["remainingMs"] = pumpRemainingMs();
  pump["manualEnabled"] = pumpState.manualPumpControlEnabled;
  pump["autoEnabled"] = !pumpState.manualPumpControlEnabled;
  pump["thresholdPercent"] = pumpState.moistureThresholdPercent;
  pump["autoDurationMs"] = pumpState.autoDurationMs;
  pump["autoCooldownMs"] = pumpState.autoCooldownMs;

  JsonObject moisture = doc.createNestedObject("moisture");
  moisture["raw"] = moistureRaw;
  moisture["percent"] = moisturePercent;

  JsonObject led = doc.createNestedObject("ledStrip");
  led["on"] = ledState.effectiveOn;
  led["r"] = ledState.r;
  led["g"] = ledState.g;
  led["b"] = ledState.b;
  led["effect"] = effectToString(ledState.effect);
  led["effectSpeedMs"] = ledState.effectSpeedMs;
  led["scheduleEnabled"] = ledState.scheduleEnabled;
  led["scheduleOnMinute"] = ledState.onMinute;
  led["scheduleOffMinute"] = ledState.offMinute;

  // Flat compatibility fields for frontend mapping.
  doc["pumpOn"] = pumpState.on;
  doc["pumpDurationMs"] = pumpState.durationMs;
  doc["pumpRemainingMs"] = pumpRemainingMs();
  doc["manualPumpControlEnabled"] = pumpState.manualPumpControlEnabled;
  doc["autoPumpEnabled"] = !pumpState.manualPumpControlEnabled;
  doc["moistureThresholdPercent"] = pumpState.moistureThresholdPercent;
  doc["autoPumpDurationMs"] = pumpState.autoDurationMs;
  doc["autoPumpCooldownMs"] = pumpState.autoCooldownMs;
  doc["moistureRaw"] = moistureRaw;
  doc["moisturePercent"] = moisturePercent;
  doc["ledStripOn"] = ledState.effectiveOn;
  doc["ledStripR"] = ledState.r;
  doc["ledStripG"] = ledState.g;
  doc["ledStripB"] = ledState.b;
  doc["ledEffect"] = effectToString(ledState.effect);
  doc["ledEffectSpeedMs"] = ledState.effectSpeedMs;
  doc["lightScheduleEnabled"] = ledState.scheduleEnabled;
  doc["lightOnMinute"] = ledState.onMinute;
  doc["lightOffMinute"] = ledState.offMinute;
}

void handleOptions() {
  addCorsHeaders();
  server.send(204, "text/plain", "");
}

void refreshStatusCacheIfNeeded() {
  if (statusCacheReady && !elapsedSince(lastStatusCacheMs, STATUS_CACHE_INTERVAL_MS)) {
    return;
  }

  statusJsonDoc.clear();
  buildStatusJson(statusJsonDoc);

  size_t needed = measureJson(statusJsonDoc);
  if (needed >= JSON_TX_BUFFER_SIZE) {
    snprintf(statusJsonCache, JSON_TX_BUFFER_SIZE, "{\"ok\":false,\"message\":\"status overflow\"}");
  } else {
    serializeJson(statusJsonDoc, statusJsonCache, JSON_TX_BUFFER_SIZE);
  }

  lastStatusCacheMs = millis();
  statusCacheReady = true;
}

void handleApiStatusLike() {
  addCorsHeaders();
  refreshStatusCacheIfNeeded();
  server.send(200, "application/json", statusJsonCache);
}

void handleRoot() {
  addCorsHeaders();
  server.send(200, "text/plain", "Vertical Garden backend is running.");
}

void handleApiSettingsPost() {
  StaticJsonDocument<768> body;
  if (!parseJsonBody(body)) {
    sendErrorJson(400, "Invalid JSON body.");
    return;
  }

  String timezone = parseStringFlexible(body["timezone"], settings.timezone);
  if (timezone.length() > 0 && timezone.length() < 80) {
    settings.timezone = timezone;
  }

  settings.liveRateEnabled = parseBoolFlexible(body["liveRateEnabled"], settings.liveRateEnabled);
  settings.liveIntervalMs = clampU32((uint32_t)parseIntFlexible(body["liveIntervalMs"], settings.liveIntervalMs), 250, 20000);
  invalidateStatusCache();

  DynamicJsonDocument response(384);
  response["ok"] = true;
  response["timezone"] = settings.timezone;
  response["liveRateEnabled"] = settings.liveRateEnabled;
  response["liveIntervalMs"] = settings.liveIntervalMs;
  sendJson(200, response);
}

void handleApiTimePost() {
  StaticJsonDocument<768> body;
  if (!parseJsonBody(body)) {
    sendErrorJson(400, "Invalid JSON body.");
    return;
  }

  String timezone = parseStringFlexible(body["timezone"], settings.timezone);
  String localTime = parseStringFlexible(body["localTime"], "");

  if (localTime.length() == 0) {
    sendErrorJson(400, "Missing localTime.");
    return;
  }

  int y, mo, d, h, mi, s;
  if (!parseIsoLocalTime(localTime, y, mo, d, h, mi, s)) {
    sendErrorJson(400, "Invalid localTime format.");
    return;
  }

  settings.timezone = timezone;
  clockState.year = y;
  clockState.month = mo;
  clockState.day = d;
  clockState.hour = h;
  clockState.minute = mi;
  clockState.second = s;
  clockState.synced = true;
  clockState.lastTickMs = millis();
  invalidateStatusCache();

  DynamicJsonDocument response(320);
  response["ok"] = true;
  response["timezone"] = settings.timezone;
  response["localTime"] = getLocalTimeIso();
  sendJson(200, response);
}

void handleApiPumpPost() {
  StaticJsonDocument<768> body;
  if (!parseJsonBody(body)) {
    sendErrorJson(400, "Invalid JSON body.");
    return;
  }

  String action = toLowerCopy(parseStringFlexible(body["action"], "toggle"));
  uint32_t durationMs = clampU32((uint32_t)parseIntFlexible(body["durationMs"], pumpState.durationMs), 1000, 120000);

  if (action == "toggle") {
    if (pumpState.on) {
      stopPump();
    } else {
      startPump(durationMs, false);
    }
  } else if (action == "on") {
    startPump(durationMs, false);
  } else if (action == "off") {
    stopPump();
  } else {
    sendErrorJson(400, "Invalid action. Use toggle/on/off.");
    return;
  }

  DynamicJsonDocument response(256);
  response["ok"] = true;
  response["pumpOn"] = pumpState.on;
  response["remainingMs"] = pumpRemainingMs();
  sendJson(200, response);
}

void handleApiPumpDurationPost() {
  StaticJsonDocument<512> body;
  if (!parseJsonBody(body)) {
    sendErrorJson(400, "Invalid JSON body.");
    return;
  }

  uint32_t newDuration = clampU32((uint32_t)parseIntFlexible(body["pumpDurationMs"], pumpState.durationMs), 1000, 120000);
  pumpState.durationMs = newDuration;
  invalidateStatusCache();

  DynamicJsonDocument response(256);
  response["ok"] = true;
  response["pumpDurationMs"] = pumpState.durationMs;
  sendJson(200, response);
}

void handleApiLedPost() {
  StaticJsonDocument<768> body;
  if (!parseJsonBody(body)) {
    sendErrorJson(400, "Invalid JSON body.");
    return;
  }

  ledState.userOn = parseBoolFlexible(body["on"], ledState.userOn);
  ledState.r = (uint8_t)clampI32(parseIntFlexible(body["r"], ledState.r), 0, 255);
  ledState.g = (uint8_t)clampI32(parseIntFlexible(body["g"], ledState.g), 0, 255);
  ledState.b = (uint8_t)clampI32(parseIntFlexible(body["b"], ledState.b), 0, 255);
  invalidateStatusCache();

  DynamicJsonDocument response(320);
  response["ok"] = true;
  response["on"] = ledState.userOn;
  response["r"] = ledState.r;
  response["g"] = ledState.g;
  response["b"] = ledState.b;
  sendJson(200, response);
}

void handleApiLedEffectPost() {
  StaticJsonDocument<768> body;
  if (!parseJsonBody(body)) {
    sendErrorJson(400, "Invalid JSON body.");
    return;
  }

  ledState.userOn = parseBoolFlexible(body["on"], ledState.userOn);
  ledState.r = (uint8_t)clampI32(parseIntFlexible(body["r"], ledState.r), 0, 255);
  ledState.g = (uint8_t)clampI32(parseIntFlexible(body["g"], ledState.g), 0, 255);
  ledState.b = (uint8_t)clampI32(parseIntFlexible(body["b"], ledState.b), 0, 255);
  ledState.effect = parseEffect(parseStringFlexible(body["effect"], effectToString(ledState.effect)));
  ledState.effectSpeedMs = clampU32((uint32_t)parseIntFlexible(body["effectSpeedMs"], ledState.effectSpeedMs), 120, 10000);
  invalidateStatusCache();

  DynamicJsonDocument response(384);
  response["ok"] = true;
  response["on"] = ledState.userOn;
  response["r"] = ledState.r;
  response["g"] = ledState.g;
  response["b"] = ledState.b;
  response["effect"] = effectToString(ledState.effect);
  response["effectSpeedMs"] = ledState.effectSpeedMs;
  sendJson(200, response);
}

void handleGardenSettingsGet() {
  StaticJsonDocument<512> doc;
  doc["manualPumpControlEnabled"] = pumpState.manualPumpControlEnabled;
  doc["autoPumpEnabled"] = !pumpState.manualPumpControlEnabled;
  doc["moistureThresholdPercent"] = pumpState.moistureThresholdPercent;
  doc["autoPumpDurationMs"] = pumpState.autoDurationMs;
  doc["autoPumpCooldownMs"] = pumpState.autoCooldownMs;
  doc["lightScheduleEnabled"] = ledState.scheduleEnabled;
  doc["lightOnMinute"] = ledState.onMinute;
  doc["lightOffMinute"] = ledState.offMinute;
  doc["ledEffect"] = effectToString(ledState.effect);
  doc["ledEffectSpeedMs"] = ledState.effectSpeedMs;
  sendJson(200, doc);
}

void handleGardenSettingsPost() {
  StaticJsonDocument<768> body;
  if (!parseJsonBody(body)) {
    sendErrorJson(400, "Invalid JSON body.");
    return;
  }

  if (body.containsKey("manualPumpControlEnabled")) {
    pumpState.manualPumpControlEnabled = parseBoolFlexible(body["manualPumpControlEnabled"], pumpState.manualPumpControlEnabled);
  } else if (body.containsKey("autoPumpEnabled")) {
    bool autoEnabled = parseBoolFlexible(body["autoPumpEnabled"], !pumpState.manualPumpControlEnabled);
    pumpState.manualPumpControlEnabled = !autoEnabled;
  }

  pumpState.moistureThresholdPercent = (uint8_t)clampI32(parseIntFlexible(body["moistureThresholdPercent"], pumpState.moistureThresholdPercent), 0, 100);
  pumpState.autoDurationMs = clampU32((uint32_t)parseIntFlexible(body["autoPumpDurationMs"], pumpState.autoDurationMs), 1000, 120000);
  pumpState.autoCooldownMs = clampU32((uint32_t)parseIntFlexible(body["autoPumpCooldownMs"], pumpState.autoCooldownMs), 1000, 600000);

  ledState.scheduleEnabled = parseBoolFlexible(body["lightScheduleEnabled"], ledState.scheduleEnabled);
  ledState.onMinute = (uint16_t)clampI32(parseIntFlexible(body["lightOnMinute"], ledState.onMinute), 0, 1439);
  ledState.offMinute = (uint16_t)clampI32(parseIntFlexible(body["lightOffMinute"], ledState.offMinute), 0, 1439);
  invalidateStatusCache();

  DynamicJsonDocument response(512);
  response["ok"] = true;
  response["manualPumpControlEnabled"] = pumpState.manualPumpControlEnabled;
  response["autoPumpEnabled"] = !pumpState.manualPumpControlEnabled;
  response["moistureThresholdPercent"] = pumpState.moistureThresholdPercent;
  response["autoPumpDurationMs"] = pumpState.autoDurationMs;
  response["autoPumpCooldownMs"] = pumpState.autoCooldownMs;
  response["lightScheduleEnabled"] = ledState.scheduleEnabled;
  response["lightOnMinute"] = ledState.onMinute;
  response["lightOffMinute"] = ledState.offMinute;
  sendJson(200, response);
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }
  sendErrorJson(404, "Endpoint not found.");
}

void handleConnectivityProbe() {
  addCorsHeaders();
  server.send(204, "text/plain", "");
}

bool isApHealthy() {
  WiFiMode_t mode = WiFi.getMode();
  bool hasApMode = (mode == WIFI_AP || mode == WIFI_AP_STA);
  if (!hasApMode) {
    return false;
  }

  IPAddress ip = WiFi.softAPIP();
  if (ip[0] == 0) {
    return false;
  }

  return true;
}

void maintainDnsState() {
  if (!ENABLE_CAPTIVE_DNS) {
    return;
  }

  if (!dnsStarted && isApHealthy()) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsStarted = true;
    Serial.println("DNS restored");
  }
}

bool restartAccessPoint() {
  WiFi.mode(staEnabled ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAPdisconnect(true);
  if (dnsStarted && ENABLE_CAPTIVE_DNS) {
    dnsServer.stop();
    dnsStarted = false;
  }
  delay(50);
  yield();

  IPAddress apIp(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apSubnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);

  bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTIONS);
  if (!apOk) {
    apOk = WiFi.softAP(AP_SSID);
  }

  if (apOk) {
    if (ENABLE_CAPTIVE_DNS) {
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      dnsStarted = true;
    } else {
      dnsStarted = false;
    }
    Serial.print("AP recovered on ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("AP recovery failed");
  }

  return apOk;
}

void maintainNetwork() {
  if (!elapsedSince(lastNetworkCheckMs, NETWORK_CHECK_INTERVAL_MS)) {
    return;
  }
  lastNetworkCheckMs = millis();

  bool apOk = isApHealthy();
  if (!apOk) {
    if (apHealthMisses < 255) {
      apHealthMisses++;
    }
    Serial.print("AP health miss: ");
    Serial.println(apHealthMisses);
  } else {
    apHealthMisses = 0;
  }

  if (apHealthMisses >= AP_HEALTH_MISS_LIMIT && elapsedSince(lastApRecoverMs, AP_RECOVER_MIN_INTERVAL_MS)) {
    lastApRecoverMs = millis();
    Serial.println("AP unhealthy -> restarting AP");
    restartAccessPoint();
    apHealthMisses = 0;
    return;
  }

  if (staEnabled && WiFi.status() != WL_CONNECTED && elapsedSince(lastStaRetryMs, STA_RETRY_INTERVAL_MS)) {
    lastStaRetryMs = millis();
    WiFi.reconnect();
    Serial.println("Retrying STA connection");
  }
}

void logRuntimeHealth() {
  if (!elapsedSince(lastRuntimeLogMs, RUNTIME_LOG_INTERVAL_MS)) {
    return;
  }
  lastRuntimeLogMs = millis();

  Serial.print("[HEALTH] uptimeMs=");
  Serial.print(millis());
  Serial.print(" heap=");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" apIP=");
  Serial.print(WiFi.softAPIP());
  Serial.print(" staStatus=");
  Serial.print((int)WiFi.status());
  Serial.print(" stations=");
  Serial.println(WiFi.softAPgetStationNum());
}


void guardHeapAndRecover() {
  if (!elapsedSince(lastHeapGuardMs, HEAP_GUARD_INTERVAL_MS)) {
    return;
  }
  lastHeapGuardMs = millis();

  uint32_t heap = ESP.getFreeHeap();
  if (!elapsedSince(lastHeapRecoverMs, HEAP_RECOVER_COOLDOWN_MS)) {
    return;
  }

  if (heap <= HEAP_HARD_RESTART_THRESHOLD) {
    lastHeapRecoverMs = millis();
    Serial.print("Heap critically low, restarting MCU. heap=");
    Serial.println(heap);
    ESP.restart();
  }
}

void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_OPTIONS, handleOptions);

  server.on("/status", HTTP_GET, handleApiStatusLike);
  server.on("/status", HTTP_OPTIONS, handleOptions);

  server.on("/api/status", HTTP_GET, handleApiStatusLike);
  server.on("/api/status", HTTP_OPTIONS, handleOptions);

  server.on("/api/sensors", HTTP_GET, handleApiStatusLike);
  server.on("/api/sensors", HTTP_OPTIONS, handleOptions);

  server.on("/api/time", HTTP_GET, handleApiStatusLike);
  server.on("/api/time", HTTP_POST, handleApiTimePost);
  server.on("/api/time", HTTP_OPTIONS, handleOptions);

  server.on("/api/settings", HTTP_POST, handleApiSettingsPost);
  server.on("/api/settings", HTTP_OPTIONS, handleOptions);

  server.on("/api/pump", HTTP_POST, handleApiPumpPost);
  server.on("/api/pump", HTTP_OPTIONS, handleOptions);

  server.on("/api/pump-duration", HTTP_POST, handleApiPumpDurationPost);
  server.on("/api/pump-duration", HTTP_OPTIONS, handleOptions);

  server.on("/api/led", HTTP_POST, handleApiLedPost);
  server.on("/api/led", HTTP_OPTIONS, handleOptions);

  server.on("/api/led/effect", HTTP_POST, handleApiLedEffectPost);
  server.on("/api/led/effect", HTTP_OPTIONS, handleOptions);

  server.on("/api/garden/settings", HTTP_GET, handleGardenSettingsGet);
  server.on("/api/garden/settings", HTTP_POST, handleGardenSettingsPost);
  server.on("/api/garden/settings", HTTP_OPTIONS, handleOptions);

  // Common connectivity probe endpoints from Android/iOS/Windows.
  server.on("/generate_204", HTTP_GET, handleConnectivityProbe);
  server.on("/gen_204", HTTP_GET, handleConnectivityProbe);
  server.on("/hotspot-detect.html", HTTP_GET, handleConnectivityProbe);
  server.on("/connecttest.txt", HTTP_GET, handleConnectivityProbe);
  server.on("/ncsi.txt", HTTP_GET, handleConnectivityProbe);
  server.on("/fwlink", HTTP_GET, handleConnectivityProbe);
  server.on("/redirect", HTTP_GET, handleConnectivityProbe);

  server.onNotFound(handleNotFound);
}

// -----------------------------
// Startup
// -----------------------------
void setupWifi() {
  WiFi.persistent(false);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setOutputPower(20.5f);
  staEnabled = ENABLE_STA && strlen(WIFI_SSID) > 0;
  WiFi.mode(staEnabled ? WIFI_AP_STA : WIFI_AP);

  if (staEnabled) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    uint32_t started = millis();
    while (WiFi.status() != WL_CONNECTED && (uint32_t)(millis() - started) < 12000) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
  }

  IPAddress apIp(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apSubnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);

  bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTIONS);
  if (!apOk) {
    apOk = WiFi.softAP(AP_SSID);
  }
  if (!apOk) {
    Serial.println("Failed to start AP.");
  } else if (ENABLE_CAPTIVE_DNS) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsStarted = true;
  }

  Serial.print("STA IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupHardware() {
  pinMode(PIN_PUMP, OUTPUT);
  stopPump();

  strip.begin();
  strip.setBrightness(LED_GLOBAL_BRIGHTNESS);
  strip.clear();
  strip.show();

  setupButtons();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println();
  Serial.println("=== Vertical Garden backend boot ===");
  Serial.print("Reset reason: ");
  Serial.println(ESP.getResetReason());
  Serial.print("SDK: ");
  Serial.println(ESP.getSdkVersion());
  Serial.print("CPU MHz: ");
  Serial.println(ESP.getCpuFreqMHz());
  Serial.print("Free heap at boot: ");
  Serial.println(ESP.getFreeHeap());

  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(60);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(60);
  }

  Serial.println();
  Serial.println("Vertical Garden backend booting...");

  setupHardware();
  setupWifi();
  setupRoutes();

  server.begin();
  Serial.print("HTTP server listening on port ");
  Serial.println(HTTP_PORT);

  lastSensorSampleMs = millis() - SENSOR_SAMPLE_INTERVAL_MS;
  clockState.lastTickMs = millis();
  lastRuntimeLogMs = millis();
  invalidateStatusCache();
}

void loop() {
  maintainNetwork();
  maintainDnsState();
  guardHeapAndRecover();
  if (ENABLE_CAPTIVE_DNS && dnsStarted) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
  updateClock();
  pollButtons();
  sampleMoisture();
  updatePumpRuntime();
  updateAutoPump();
  renderLedEffect();
  logRuntimeHealth();
  yield();
}
