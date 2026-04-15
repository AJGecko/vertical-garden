enum LedEffect {
  LED_EFFECT_STATIC,
  LED_EFFECT_BLINK,
  LED_EFFECT_BREATHE,
  LED_EFFECT_RAINBOW
};

// Strikte FastLED Timing-Vorgaben für ESP8266 OHNE Retry.
// Retries verursachen auf dem ESP8266 ein massives visuelles Flackern.
#define FASTLED_ALLOW_INTERRUPTS 0 
#define FASTLED_INTERRUPT_RETRY_COUNT 0

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

const char* CONFIG_FILE = "/config.json";
bool pendingConfigSave = false;
uint32_t lastConfigChangeMs = 0;

void requestConfigSave() {
  pendingConfigSave = true;
  lastConfigChangeMs = millis();
}

// Network configuration
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";
const char* AP_SSID = "VerticalGarden";
const char* AP_PASSWORD = "garden123";

// Pin mapping
const uint8_t PIN_PUMP = D8;
const uint8_t PIN_MOISTURE = A0;
const uint8_t PIN_BUTTON_PUMP = D5;
const uint8_t PIN_BUTTON_LED = D6;
const uint8_t PIN_BUTTON_EFFECT = D7;
const uint8_t PIN_LED_STRIP = D1;

const bool PUMP_ACTIVE_HIGH = true;
const uint16_t HTTP_PORT = 80;
const uint16_t LED_COUNT = 18;
const uint8_t LED_GLOBAL_BRIGHTNESS = 48;
const size_t JSON_TX_BUFFER_SIZE = 2048;
const byte DNS_PORT = 53;
const bool ENABLE_CAPTIVE_DNS = false;
const bool ENABLE_STA = false;
const uint32_t SERIAL_BAUD = 115200;
const uint32_t RUNTIME_LOG_INTERVAL_MS = 10000;

const uint8_t AP_CHANNEL = 6;
const bool AP_HIDDEN = false;
const uint8_t AP_MAX_CONNECTIONS = 4;
const bool AP_PREFER_FIXED_CHANNEL = false;
const bool AP_ALLOW_OPEN_FALLBACK = false;
const uint32_t NETWORK_CHECK_INTERVAL_MS = 5000;
const uint32_t AP_RECOVER_MIN_INTERVAL_MS = 30000;
const uint8_t AP_HEALTH_MISS_LIMIT = 6;
const uint32_t AP_RECOVER_VERIFY_DELAY_MS = 180;
const uint8_t AP_RECOVER_FAIL_RESTART_LIMIT = 1;
const uint32_t WIFI_RF_RESET_DELAY_MS = 120;
const uint32_t AP_RECOVER_FAILURE_BACKOFF_MS = 120000;
const uint32_t HTTP_STALL_CHECK_INTERVAL_MS = 5000;
const uint32_t HTTP_STALL_WITH_STATION_MS = 70000;
const uint8_t HTTP_STALL_CONFIRMATIONS = 3;
const uint32_t HTTP_STALL_RECOVER_MIN_INTERVAL_MS = 45000;
const uint32_t HTTP_STALL_MIN_REQUESTS = 10;
const bool ENABLE_HTTP_STALL_RECOVERY = false;

const uint16_t BUTTON_DEBOUNCE_MS = 40;
const uint16_t SENSOR_SAMPLE_INTERVAL_MS = 1000;
const uint16_t LED_FRAME_INTERVAL_MS = 20;
const uint16_t LED_SHOW_MIN_INTERVAL_US = 350;
const uint16_t LED_KEEPALIVE_INTERVAL_MS = 1200;
const uint16_t STATION_COUNT_CACHE_INTERVAL_MS = 500;
const uint16_t STATUS_CACHE_INTERVAL_MS = 1000;
const uint32_t HEAP_GUARD_INTERVAL_MS = 3000;
const uint32_t HEAP_HARD_RESTART_THRESHOLD = 5000;

const int MOISTURE_RAW_DRY = 860;
const int MOISTURE_RAW_WET = 420;

// AsyncWebServer instead of ESP8266WebServer
AsyncWebServer server(HTTP_PORT);
DNSServer dnsServer;
CRGB leds[LED_COUNT];
char jsonTxBuffer[JSON_TX_BUFFER_SIZE];
char statusJsonCache[JSON_TX_BUFFER_SIZE];
StaticJsonDocument<2048> statusJsonDoc;

// Global buffer for POST body data
String jsonBodyBuffer;

bool staEnabled = false;
bool dnsStarted = false;
uint32_t lastNetworkCheckMs = 0;
uint32_t lastApRecoverMs = 0;
uint8_t apHealthMisses = 0;
uint32_t lastRuntimeLogMs = 0;
uint32_t lastStatusCacheMs = 0;
bool statusCacheReady = false;
uint32_t lastHeapGuardMs = 0;
uint32_t minHeapSeen = 0xFFFFFFFF;
uint32_t httpRequestCount = 0;
uint32_t lastHttpRequestMs = 0;
uint32_t lastHttpStallCheckMs = 0;
uint32_t lastHttpStallRecoverMs = 0;
uint8_t httpStallDetections = 0;
uint8_t apRecoverFailureCount = 0;
uint32_t lastApRecoverFailureMs = 0;
uint32_t lastStationCountSampleMs = 0;
uint8_t cachedStationCount = 0;

struct ControllerSettings {
  String timezone = "Europe/Berlin";
  bool liveRateEnabled = false;
  uint32_t liveIntervalMs = 4000;
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
  bool manualEnabled = true;
  uint8_t thresholdPercent = 35;
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
  uint8_t brightnessPercent = 100;
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
bool heapEmergencyMode = false;
uint32_t lastLedShowUs = 0;
uint32_t lastLedRefreshMs = 0;

bool isApHealthy();
void cycleLedEffect();

bool elapsedSince(uint32_t sinceMs, uint32_t durationMs) {
  return (uint32_t)(millis() - sinceMs) >= durationMs;
}

uint8_t getStationCountCached() {
  if (lastStationCountSampleMs == 0 || elapsedSince(lastStationCountSampleMs, STATION_COUNT_CACHE_INTERVAL_MS)) {
    lastStationCountSampleMs = millis();
    cachedStationCount = WiFi.softAPgetStationNum();
  }
  return cachedStationCount;
}

uint8_t breatheWave8(uint32_t nowMs, uint32_t speedMs) {
  uint32_t cyclePos = nowMs % speedMs;
  uint8_t idx = (uint8_t)(((uint64_t)cyclePos * 255ULL) / speedMs);
  return sin8((uint8_t)(idx - 64));
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

uint8_t scaleByBrightness(uint8_t channel, uint8_t brightnessPercent) {
  return (uint8_t)(((uint16_t)channel * (uint16_t)brightnessPercent) / 100U);
}

String parseStringFlexible(JsonVariantConst value, const String& fallback) {
  if (value.isNull()) return fallback;
  if (value.is<const char*>()) return String(value.as<const char*>());
  if (value.is<String>()) return value.as<String>();
  return fallback;
}

void formatIpAddress(const IPAddress& ip, char* out, size_t outLen) {
  snprintf(out, outLen, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

bool isIpConfigured(const IPAddress& ip) {
  return !(ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0);
}

void writeLocalTimeIso(char* out, size_t outLen) {
  if (!clockState.synced) {
    snprintf(out, outLen, "-");
    return;
  }
  snprintf(out, outLen, "%04d-%02d-%02dT%02d:%02d:%02d",
           clockState.year, clockState.month, clockState.day,
           clockState.hour, clockState.minute, clockState.second);
}

void invalidateStatusCache() {
  statusCacheReady = false;
  lastStatusCacheMs = 0;
}

bool isLeapYear(int year) {
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  return year % 4 == 0;
}

int daysInMonth(int year, int month) {
  static const int daysPerMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2) return isLeapYear(year) ? 29 : 28;
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
  int dim = daysInMonth(clockState.year, clockState.month);
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

void maintainDnsState() {
  if (!ENABLE_CAPTIVE_DNS) return;
  if (!dnsStarted && isApHealthy()) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsStarted = true;
    Serial.println("DNS restored");
  }
}

void handleButtonPress(uint8_t index) {
  if (index == 0) {
    if (pumpState.on) stopPump(); else startPump(pumpState.durationMs, false);
    return;
  }
  if (index == 1) {
    ledState.userOn = !ledState.userOn;
    invalidateStatusCache();
    requestConfigSave();
    return;
  }
  if (index == 2) {
    ledState.userOn = true;
    cycleLedEffect();
    invalidateStatusCache();
    requestConfigSave();
    return;
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

uint8_t moistureRawToPercent(int rawValue) {
  int constrained = clampI32(rawValue, MOISTURE_RAW_WET, MOISTURE_RAW_DRY);
  int span = MOISTURE_RAW_DRY - MOISTURE_RAW_WET;
  if (span <= 0) return 0;
  int numerator = (MOISTURE_RAW_DRY - constrained) * 100 + (span / 2);
  int percent = numerator / span;
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

void updatePumpRuntime() {
  if (!pumpState.on) return;
  if ((int32_t)(millis() - pumpState.endAtMs) >= 0) {
    stopPump();
  }
}

void updateAutoPump() {
  if (pumpState.manualEnabled) return;
  if (pumpState.on) return;
  bool isDry = moisturePercent < pumpState.thresholdPercent;
  if (isDry && elapsedSince(pumpState.lastAutoTriggerMs, pumpState.autoCooldownMs)) {
    startPump(pumpState.autoDurationMs, true);
  }
}

uint16_t currentMinuteOfDay() {
  return (uint16_t)(clockState.hour * 60 + clockState.minute);
}

bool isWithinScheduleWindow(uint16_t nowMinute, uint16_t onMinute, uint16_t offMinute) {
  if (onMinute == offMinute) return true;
  if (onMinute < offMinute) {
    return nowMinute >= onMinute && nowMinute < offMinute;
  }
  return nowMinute >= onMinute || nowMinute < offMinute;
}

CRGB wheelColor(uint8_t wheelPos, uint8_t brightnessPercent = 100) {
  wheelPos = 255 - wheelPos;
  uint8_t r, g, b;
  if (wheelPos < 85) {
    r = 255 - wheelPos * 3;
    g = 0;
    b = wheelPos * 3;
  } else if (wheelPos < 170) {
    wheelPos -= 85;
    r = 0;
    g = wheelPos * 3;
    b = 255 - wheelPos * 3;
  } else {
    wheelPos -= 170;
    r = wheelPos * 3;
    g = 255 - wheelPos * 3;
    b = 0;
  }
  if (brightnessPercent < 100) {
    r = (uint8_t)((uint16_t)r * brightnessPercent / 100U);
    g = (uint8_t)((uint16_t)g * brightnessPercent / 100U);
    b = (uint8_t)((uint16_t)b * brightnessPercent / 100U);
  }
  return CRGB(r, g, b);
}

void fillStrip(uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    leds[i] = CRGB(r, g, b);
  }
}

void showStripSafely() {
  yield(); 
  
  uint32_t nowUs = micros();
  uint32_t elapsedUs = (uint32_t)(nowUs - lastLedShowUs);
  if (elapsedUs < LED_SHOW_MIN_INTERVAL_US) {
    delayMicroseconds(LED_SHOW_MIN_INTERVAL_US - elapsedUs);
  }
  FastLED.show();
  lastLedShowUs = micros();
  lastLedRefreshMs = millis();
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
  bool offFrameIsCurrent = ledOutputInitialized && !lastRenderedOn;
  if (offFrameIsCurrent && !elapsedSince(lastLedRefreshMs, LED_KEEPALIVE_INTERVAL_MS)) {
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
  if (unchanged && !elapsedSince(lastLedRefreshMs, LED_KEEPALIVE_INTERVAL_MS)) {
    return;
  }
  fillStrip(r, g, b);
  showStripSafely();
  rememberSolidFrame(true, effect, r, g, b);
}

void renderLedEffect() {
  if (heapEmergencyMode) {
    clearStripIfNeeded();
    return;
  }
  
  uint32_t realNow = millis();
  if (!elapsedSince(lastLedFrameMs, LED_FRAME_INTERVAL_MS)) return;
  
  uint32_t dt = realNow - lastLedFrameMs;
  lastLedFrameMs = realNow;
  if (dt > 30) dt = 30; 
  
  static uint32_t animTimeMs = 0;
  animTimeMs += dt;

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
  switch (ledState.effect) {
    case LED_EFFECT_BLINK: {
      uint32_t halfCycle = speedMs / 2;
      if (halfCycle < 60) halfCycle = 60;
      bool onPhase = ((animTimeMs / halfCycle) % 2) == 0;
      if (!onPhase) {
        frameR = 0;
        frameG = 0;
        frameB = 0;
      }
      break;
    }
    case LED_EFFECT_BREATHE: {
      uint8_t wave8 = breatheWave8(animTimeMs, speedMs);
      frameR = (uint8_t)(((uint16_t)ledState.r * wave8) / 255U);
      frameG = (uint8_t)(((uint16_t)ledState.g * wave8) / 255U);
      frameB = (uint8_t)(((uint16_t)ledState.b * wave8) / 255U);
      break;
    }
    case LED_EFFECT_RAINBOW: {
      uint32_t base = (animTimeMs * 256UL) / speedMs;
      for (uint16_t i = 0; i < LED_COUNT; i++) {
        uint8_t idx = (uint8_t)((i * 256 / LED_COUNT + base) & 255);
        leds[i] = wheelColor(idx, ledState.brightnessPercent);
      }
      showStripSafely();
      rememberSolidFrame(true, LED_EFFECT_RAINBOW, 0, 0, 0);
      return;
    }
    case LED_EFFECT_STATIC:
    default:
      break;
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

// AsyncWebServer helper functions
void addCorsHeaders(AsyncWebServerResponse *response) {
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  response->addHeader("Access-Control-Allow-Headers", "Content-Type");
  response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  response->addHeader("Pragma", "no-cache");
  response->addHeader("Expires", "0");
  response->addHeader("Connection", "close");
}

void sendJsonAsync(AsyncWebServerRequest *request, int statusCode, JsonDocument& doc) {
  httpRequestCount++;
  lastHttpRequestMs = millis();
  
  size_t needed = measureJson(doc);
  if (needed >= JSON_TX_BUFFER_SIZE) {
    AsyncWebServerResponse *response = request->beginResponse(500, "application/json", "{\"ok\":false,\"message\":\"json overflow\"}");
    addCorsHeaders(response);
    request->send(response);
    return;
  }
  serializeJson(doc, jsonTxBuffer, JSON_TX_BUFFER_SIZE);
  AsyncWebServerResponse *response = request->beginResponse(statusCode, "application/json", jsonTxBuffer);
  addCorsHeaders(response);
  request->send(response);
}

void sendErrorJsonAsync(AsyncWebServerRequest *request, int statusCode, const String& message) {
  StaticJsonDocument<256> doc;
  doc["ok"] = false;
  doc["message"] = message;
  sendJsonAsync(request, statusCode, doc);
}

bool parseJsonBodyAsync(const String& bodyStr, JsonDocument& doc) {
  doc.clear();
  DeserializationError error = deserializeJson(doc, bodyStr);
  return !error;
}

void buildStatusJson(JsonDocument& doc) {
  char localTimeBuf[20];
  writeLocalTimeIso(localTimeBuf, sizeof(localTimeBuf));
  char ipBuf[16];
  IPAddress ip = (staEnabled && WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : WiFi.softAPIP();
  formatIpAddress(ip, ipBuf, sizeof(ipBuf));
  const char* effectName = effectToString(ledState.effect);
  uint32_t now = millis();
  uint32_t pumpRemaining = pumpState.on ? ((now >= pumpState.endAtMs) ? 0 : pumpState.endAtMs - now) : 0;

  doc["timezone"] = settings.timezone;
  doc["localTime"] = localTimeBuf;
  if (staEnabled && WiFi.status() == WL_CONNECTED) {
    String connectedSsid = WiFi.SSID();
    doc["apSsid"] = connectedSsid.length() > 0 ? connectedSsid : AP_SSID;
  } else {
    doc["apSsid"] = AP_SSID;
  }
  doc["apIp"] = ipBuf;
  doc["httpPort"] = HTTP_PORT;

  JsonObject pump = doc.createNestedObject("pump");
  pump["on"] = pumpState.on;
  pump["durationMs"] = pumpState.durationMs;
  pump["remainingMs"] = pumpRemaining;
  pump["manualEnabled"] = pumpState.manualEnabled;
  pump["thresholdPercent"] = pumpState.thresholdPercent;
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
  led["effect"] = effectName;
  led["effectSpeedMs"] = ledState.effectSpeedMs;
  led["scheduleEnabled"] = ledState.scheduleEnabled;
  led["scheduleOnMinute"] = ledState.onMinute;
  led["scheduleOffMinute"] = ledState.offMinute;

  doc["pumpOn"] = pumpState.on;
  doc["pumpDurationMs"] = pumpState.durationMs;
  doc["pumpRemainingMs"] = pumpRemaining;
  doc["manualPumpControlEnabled"] = pumpState.manualEnabled;
  doc["moistureThresholdPercent"] = pumpState.thresholdPercent;
  doc["autoPumpDurationMs"] = pumpState.autoDurationMs;
  doc["autoPumpCooldownMs"] = pumpState.autoCooldownMs;
  doc["moistureRaw"] = moistureRaw;
  doc["moisturePercent"] = moisturePercent;
  doc["ledStripOn"] = ledState.effectiveOn;
  doc["ledStripR"] = ledState.r;
  doc["ledStripG"] = ledState.g;
  doc["ledStripB"] = ledState.b;
  doc["ledEffect"] = effectName;
  doc["ledEffectSpeedMs"] = ledState.effectSpeedMs;
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

void handleOptions(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(204, "text/plain", "");
  addCorsHeaders(response);
  request->send(response);
}

void handleApiStatusLike(AsyncWebServerRequest *request) {
  httpRequestCount++;
  lastHttpRequestMs = millis();
  refreshStatusCacheIfNeeded();
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", statusJsonCache);
  addCorsHeaders(response);
  request->send(response);
}

void handleRoot(AsyncWebServerRequest *request) {
  httpRequestCount++;
  lastHttpRequestMs = millis();
  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Vertical Garden backend is running.");
  addCorsHeaders(response);
  request->send(response);
}

void handleApiSettingsPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    jsonBodyBuffer = "";
  }
  jsonBodyBuffer += String((char*)data).substring(0, len);
  
  if (index + len == total) {
    httpRequestCount++;
    lastHttpRequestMs = millis();
    
    StaticJsonDocument<768> body;
    if (!parseJsonBodyAsync(jsonBodyBuffer, body)) {
      sendErrorJsonAsync(request, 400, "Invalid JSON body.");
      return;
    }
    String timezone = parseStringFlexible(body["timezone"], settings.timezone);
    if (timezone.length() > 0 && timezone.length() < 80) {
      settings.timezone = timezone;
    }
    settings.liveRateEnabled = parseBoolFlexible(body["liveRateEnabled"], settings.liveRateEnabled);
    settings.liveIntervalMs = clampU32((uint32_t)parseIntFlexible(body["liveIntervalMs"], settings.liveIntervalMs), 4000, 20000);
    invalidateStatusCache();
    requestConfigSave();
    StaticJsonDocument<384> response;
    response["ok"] = true;
    response["timezone"] = settings.timezone;
    response["liveRateEnabled"] = settings.liveRateEnabled;
    response["liveIntervalMs"] = settings.liveIntervalMs;
    sendJsonAsync(request, 200, response);
  }
}

void handleApiTimePost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    jsonBodyBuffer = "";
  }
  jsonBodyBuffer += String((char*)data).substring(0, len);
  
  if (index + len == total) {
    httpRequestCount++;
    lastHttpRequestMs = millis();
    
    StaticJsonDocument<768> body;
    if (!parseJsonBodyAsync(jsonBodyBuffer, body)) {
      sendErrorJsonAsync(request, 400, "Invalid JSON body.");
      return;
    }
    String localTime = parseStringFlexible(body["localTime"], "");
    String timezone = parseStringFlexible(body["timezone"], settings.timezone);
    if (localTime.length() == 0) {
      sendErrorJsonAsync(request, 400, "Missing localTime.");
      return;
    }
    int y, mo, d, h, mi, s;
    if (!parseIsoLocalTime(localTime, y, mo, d, h, mi, s)) {
      sendErrorJsonAsync(request, 400, "Invalid localTime format.");
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
    requestConfigSave();
    StaticJsonDocument<320> response;
    response["ok"] = true;
    response["timezone"] = settings.timezone;
    char localTimeBuf[20];
    writeLocalTimeIso(localTimeBuf, sizeof(localTimeBuf));
    response["localTime"] = localTimeBuf;
    sendJsonAsync(request, 200, response);
  }
}

void handleApiPumpPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    jsonBodyBuffer = "";
  }
  jsonBodyBuffer += String((char*)data).substring(0, len);
  
  if (index + len == total) {
    httpRequestCount++;
    lastHttpRequestMs = millis();
    
    StaticJsonDocument<768> body;
    if (!parseJsonBodyAsync(jsonBodyBuffer, body)) {
      sendErrorJsonAsync(request, 400, "Invalid JSON body.");
      return;
    }
    String action = toLowerCopy(parseStringFlexible(body["action"], "toggle"));
    uint32_t durationMs = clampU32((uint32_t)parseIntFlexible(body["durationMs"], pumpState.durationMs), 1000, 120000);
    if (action == "toggle") {
      if (pumpState.on) stopPump(); else startPump(durationMs, false);
    } else if (action == "on") {
      startPump(durationMs, false);
    } else if (action == "off") {
      stopPump();
    } else {
      sendErrorJsonAsync(request, 400, "Invalid action. Use toggle/on/off.");
      return;
    }
    StaticJsonDocument<256> response;
    response["ok"] = true;
    response["pumpOn"] = pumpState.on;
    response["remainingMs"] = pumpState.on ? ((millis() >= pumpState.endAtMs) ? 0 : pumpState.endAtMs - millis()) : 0;
    sendJsonAsync(request, 200, response);
  }
}

void handleApiPumpDurationPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    jsonBodyBuffer = "";
  }
  jsonBodyBuffer += String((char*)data).substring(0, len);
  
  if (index + len == total) {
    httpRequestCount++;
    lastHttpRequestMs = millis();
    
    StaticJsonDocument<512> body;
    if (!parseJsonBodyAsync(jsonBodyBuffer, body)) {
      sendErrorJsonAsync(request, 400, "Invalid JSON body.");
      return;
    }
    pumpState.durationMs = clampU32((uint32_t)parseIntFlexible(body["pumpDurationMs"], pumpState.durationMs), 1000, 120000);
    invalidateStatusCache();
    requestConfigSave();
    StaticJsonDocument<256> response;
    response["ok"] = true;
    response["pumpDurationMs"] = pumpState.durationMs;
    sendJsonAsync(request, 200, response);
  }
}

void handleApiLedPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    jsonBodyBuffer = "";
  }
  jsonBodyBuffer += String((char*)data).substring(0, len);
  
  if (index + len == total) {
    httpRequestCount++;
    lastHttpRequestMs = millis();
    
    StaticJsonDocument<768> body;
    if (!parseJsonBodyAsync(jsonBodyBuffer, body)) {
      sendErrorJsonAsync(request, 400, "Invalid JSON body.");
      return;
    }
    bool requestedOn = parseBoolFlexible(body["on"], ledState.userOn);
    uint8_t requestedR = (uint8_t)clampI32(parseIntFlexible(body["r"], ledState.r), 0, 255);
    uint8_t requestedG = (uint8_t)clampI32(parseIntFlexible(body["g"], ledState.g), 0, 255);
    uint8_t requestedB = (uint8_t)clampI32(parseIntFlexible(body["b"], ledState.b), 0, 255);
    uint8_t brightnessPercent = (uint8_t)clampI32(parseIntFlexible(body["brightnessPercent"], 100), 0, 100);

    ledState.userOn = requestedOn;
    ledState.brightnessPercent = brightnessPercent;
    if (requestedOn && brightnessPercent > 0 && requestedR == 0 && requestedG == 0 && requestedB == 0) {
      uint8_t neutral = (uint8_t)((255U * brightnessPercent) / 100U);
      ledState.r = neutral;
      ledState.g = neutral;
      ledState.b = neutral;
    } else {
      ledState.r = scaleByBrightness(requestedR, brightnessPercent);
      ledState.g = scaleByBrightness(requestedG, brightnessPercent);
      ledState.b = scaleByBrightness(requestedB, brightnessPercent);
    }

    invalidateStatusCache();
    requestConfigSave();
    StaticJsonDocument<320> response;
    response["ok"] = true;
    response["on"] = ledState.userOn;
    response["r"] = ledState.r;
    response["g"] = ledState.g;
    response["b"] = ledState.b;
    response["brightnessPercent"] = brightnessPercent;
    sendJsonAsync(request, 200, response);
  }
}

void handleApiLedEffectPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    jsonBodyBuffer = "";
  }
  jsonBodyBuffer += String((char*)data).substring(0, len);
  
  if (index + len == total) {
    httpRequestCount++;
    lastHttpRequestMs = millis();
    
    StaticJsonDocument<768> body;
    if (!parseJsonBodyAsync(jsonBodyBuffer, body)) {
      sendErrorJsonAsync(request, 400, "Invalid JSON body.");
      return;
    }
    bool requestedOn = parseBoolFlexible(body["on"], ledState.userOn);
    uint8_t requestedR = (uint8_t)clampI32(parseIntFlexible(body["r"], ledState.r), 0, 255);
    uint8_t requestedG = (uint8_t)clampI32(parseIntFlexible(body["g"], ledState.g), 0, 255);
    uint8_t requestedB = (uint8_t)clampI32(parseIntFlexible(body["b"], ledState.b), 0, 255);
    uint8_t brightnessPercent = (uint8_t)clampI32(parseIntFlexible(body["brightnessPercent"], 100), 0, 100);

    ledState.userOn = requestedOn;
    ledState.brightnessPercent = brightnessPercent;
    if (requestedOn && brightnessPercent > 0 && requestedR == 0 && requestedG == 0 && requestedB == 0) {
      uint8_t neutral = (uint8_t)((255U * brightnessPercent) / 100U);
      ledState.r = neutral;
      ledState.g = neutral;
      ledState.b = neutral;
    } else {
      ledState.r = scaleByBrightness(requestedR, brightnessPercent);
      ledState.g = scaleByBrightness(requestedG, brightnessPercent);
      ledState.b = scaleByBrightness(requestedB, brightnessPercent);
    }

    ledState.effect = parseEffect(parseStringFlexible(body["effect"], effectToString(ledState.effect)));
    ledState.effectSpeedMs = clampU32((uint32_t)parseIntFlexible(body["effectSpeedMs"], ledState.effectSpeedMs), 120, 10000);
    invalidateStatusCache();
    requestConfigSave();
    StaticJsonDocument<384> response;
    response["ok"] = true;
    response["on"] = ledState.userOn;
    response["r"] = ledState.r;
    response["g"] = ledState.g;
    response["b"] = ledState.b;
    response["brightnessPercent"] = brightnessPercent;
    response["effect"] = effectToString(ledState.effect);
    response["effectSpeedMs"] = ledState.effectSpeedMs;
    sendJsonAsync(request, 200, response);
  }
}

void handleGardenSettingsGet(AsyncWebServerRequest *request) {
  httpRequestCount++;
  lastHttpRequestMs = millis();
  
  StaticJsonDocument<512> doc;
  doc["manualPumpControlEnabled"] = pumpState.manualEnabled;
  doc["autoPumpEnabled"] = !pumpState.manualEnabled;
  doc["moistureThresholdPercent"] = pumpState.thresholdPercent;
  doc["autoPumpDurationMs"] = pumpState.autoDurationMs;
  doc["autoPumpCooldownMs"] = pumpState.autoCooldownMs;
  doc["lightScheduleEnabled"] = ledState.scheduleEnabled;
  doc["lightOnMinute"] = ledState.onMinute;
  doc["lightOffMinute"] = ledState.offMinute;
  doc["ledEffect"] = effectToString(ledState.effect);
  doc["ledEffectSpeedMs"] = ledState.effectSpeedMs;
  sendJsonAsync(request, 200, doc);
}

void handleGardenSettingsPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  if (index == 0) {
    jsonBodyBuffer = "";
  }
  jsonBodyBuffer += String((char*)data).substring(0, len);
  
  if (index + len == total) {
    httpRequestCount++;
    lastHttpRequestMs = millis();
    
    StaticJsonDocument<768> body;
    if (!parseJsonBodyAsync(jsonBodyBuffer, body)) {
      sendErrorJsonAsync(request, 400, "Invalid JSON body.");
      return;
    }
    if (body.containsKey("manualPumpControlEnabled")) {
      pumpState.manualEnabled = parseBoolFlexible(body["manualPumpControlEnabled"], pumpState.manualEnabled);
    } else if (body.containsKey("autoPumpEnabled")) {
      pumpState.manualEnabled = !parseBoolFlexible(body["autoPumpEnabled"], !pumpState.manualEnabled);
    }
    if (body.containsKey("lightScheduleEnabled")) {
      ledState.scheduleEnabled = parseBoolFlexible(body["lightScheduleEnabled"], ledState.scheduleEnabled);
    }
    if (body.containsKey("lightOnMinute")) {
      ledState.onMinute = (uint16_t)clampI32(parseIntFlexible(body["lightOnMinute"], ledState.onMinute), 0, 24 * 60 - 1);
    }
    if (body.containsKey("lightOffMinute")) {
      ledState.offMinute = (uint16_t)clampI32(parseIntFlexible(body["lightOffMinute"], ledState.offMinute), 0, 24 * 60 - 1);
    }
    pumpState.thresholdPercent = (uint8_t)clampI32(parseIntFlexible(body["moistureThresholdPercent"], pumpState.thresholdPercent), 0, 100);
    pumpState.autoDurationMs = clampU32((uint32_t)parseIntFlexible(body["autoPumpDurationMs"], pumpState.autoDurationMs), 1000, 120000);
    pumpState.autoCooldownMs = clampU32((uint32_t)parseIntFlexible(body["autoPumpCooldownMs"], pumpState.autoCooldownMs), 1000, 600000);
    invalidateStatusCache();
    requestConfigSave();
    StaticJsonDocument<512> response;
    response["ok"] = true;
    response["manualPumpControlEnabled"] = pumpState.manualEnabled;
    response["autoPumpEnabled"] = !pumpState.manualEnabled;
    response["moistureThresholdPercent"] = pumpState.thresholdPercent;
    response["autoPumpDurationMs"] = pumpState.autoDurationMs;
    response["autoPumpCooldownMs"] = pumpState.autoCooldownMs;
    response["lightScheduleEnabled"] = ledState.scheduleEnabled;
    response["lightOnMinute"] = ledState.onMinute;
    response["lightOffMinute"] = ledState.offMinute;
    response["ledEffect"] = effectToString(ledState.effect);
    response["ledEffectSpeedMs"] = ledState.effectSpeedMs;
    sendJsonAsync(request, 200, response);
  }
}

void handleNotFound(AsyncWebServerRequest *request) {
  if (request->method() == HTTP_OPTIONS) {
    handleOptions(request);
    return;
  }
  sendErrorJsonAsync(request, 404, "Endpoint not found.");
}

void handleConnectivityProbe(AsyncWebServerRequest *request) {
  httpRequestCount++;
  lastHttpRequestMs = millis();
  AsyncWebServerResponse *response = request->beginResponse(204, "text/plain", "");
  addCorsHeaders(response);
  request->send(response);
}

bool isApHealthy() {
  WiFiMode_t mode = WiFi.getMode();
  bool apModeActive = mode == WIFI_AP || mode == WIFI_AP_STA;
  if (!apModeActive) return false;
  IPAddress apIp = WiFi.softAPIP();
  if (!isIpConfigured(apIp)) {
    return false;
  }
  if (WiFi.softAPSSID().length() == 0) {
    return false;
  }
  return true;
}

bool startAccessPointBestEffort() {
  IPAddress apIp(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apSubnet(255, 255, 255, 0);

  WiFi.mode(staEnabled ? WIFI_AP_STA : WIFI_AP);
  delay(30);
  yield();

  bool configOk = WiFi.softAPConfig(apIp, apGateway, apSubnet);
  if (!configOk) {
    Serial.println("AP IP config failed, will verify after AP start");
  }

  bool apOk = false;
  if (AP_PREFER_FIXED_CHANNEL) {
    apOk = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTIONS);
  }
  if (!apOk) {
    apOk = WiFi.softAP(AP_SSID, AP_PASSWORD);
  }
  if (!apOk && AP_ALLOW_OPEN_FALLBACK) {
    apOk = WiFi.softAP(AP_SSID);
  }

  if (!apOk) {
    return false;
  }

  delay(AP_RECOVER_VERIFY_DELAY_MS);
  yield();

  IPAddress currentIp = WiFi.softAPIP();
  if (!isIpConfigured(currentIp)) {
    Serial.println("AP started but IP unset, retrying AP startup");
    WiFi.softAPdisconnect(true);
    delay(80);
    yield();
    WiFi.mode(staEnabled ? WIFI_AP_STA : WIFI_AP);
    delay(40);
    yield();
    WiFi.softAPConfig(apIp, apGateway, apSubnet);

    apOk = false;
    if (AP_PREFER_FIXED_CHANNEL) {
      apOk = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTIONS);
    }
    if (!apOk) {
      apOk = WiFi.softAP(AP_SSID, AP_PASSWORD);
    }
    if (!apOk && AP_ALLOW_OPEN_FALLBACK) {
      apOk = WiFi.softAP(AP_SSID);
    }

    delay(AP_RECOVER_VERIFY_DELAY_MS);
    yield();
    currentIp = WiFi.softAPIP();
    if (!apOk || !isIpConfigured(currentIp)) {
      Serial.println("AP startup failed: IP still unset");
      return false;
    }
  }

  if (apOk) {
    Serial.print("AP started on ");
    Serial.print(currentIp);
    Serial.print(" channel=");
    Serial.println(WiFi.channel());
  }
  return apOk;
}

bool restartAccessPoint() {
  if (ENABLE_CAPTIVE_DNS && dnsStarted) {
    dnsServer.stop();
    dnsStarted = false;
  }

  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  delay(60);
  WiFi.mode(WIFI_OFF);
  delay(WIFI_RF_RESET_DELAY_MS);

  WiFi.forceSleepBegin();
  delay(WIFI_RF_RESET_DELAY_MS);
  WiFi.forceSleepWake();
  delay(WIFI_RF_RESET_DELAY_MS);
  yield();

  WiFi.mode(staEnabled ? WIFI_AP_STA : WIFI_AP);
  delay(WIFI_RF_RESET_DELAY_MS);
  yield();

  bool apOk = startAccessPointBestEffort();
  if (!apOk || !isApHealthy()) {
    if (apRecoverFailureCount < 255) {
      apRecoverFailureCount++;
    }
    lastApRecoverFailureMs = millis();
    Serial.print("AP recovery failed count=");
    Serial.println(apRecoverFailureCount);
    if (apRecoverFailureCount >= AP_RECOVER_FAIL_RESTART_LIMIT) {
      Serial.println("AP recovery repeatedly failed, staying in safe retry mode");
    }
    return false;
  }

  apRecoverFailureCount = 0;
  lastApRecoverFailureMs = 0;

  if (ENABLE_CAPTIVE_DNS && !dnsStarted) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsStarted = true;
  }
  Serial.print("AP recovered on ");
  Serial.println(WiFi.softAPIP());
  invalidateStatusCache();
  return true;
}

bool shouldRecoverFromHttpStall() {
  if (!elapsedSince(lastHttpStallCheckMs, HTTP_STALL_CHECK_INTERVAL_MS)) {
    return false;
  }
  lastHttpStallCheckMs = millis();

  if (httpRequestCount < HTTP_STALL_MIN_REQUESTS) {
    httpStallDetections = 0;
    return false;
  }

  uint8_t stations = getStationCountCached();
  if (stations == 0) {
    httpStallDetections = 0;
    return false;
  }

  uint32_t idleMs = lastHttpRequestMs == 0 ? 0 : (uint32_t)(millis() - lastHttpRequestMs);
  if (idleMs < HTTP_STALL_WITH_STATION_MS) {
    httpStallDetections = 0;
    return false;
  }

  if (httpStallDetections < 255) {
    httpStallDetections++;
  }

  Serial.print("HTTP stall suspect: stations=");
  Serial.print(stations);
  Serial.print(" idleMs=");
  Serial.print(idleMs);
  Serial.print(" detect=");
  Serial.print(httpStallDetections);
  Serial.print("/");
  Serial.println(HTTP_STALL_CONFIRMATIONS);

  if (httpStallDetections < HTTP_STALL_CONFIRMATIONS) {
    return false;
  }

  if (!elapsedSince(lastHttpStallRecoverMs, HTTP_STALL_RECOVER_MIN_INTERVAL_MS)) {
    return false;
  }

  lastHttpStallRecoverMs = millis();
  httpStallDetections = 0;
  return true;
}

void maintainNetwork() {
  if (!elapsedSince(lastNetworkCheckMs, NETWORK_CHECK_INTERVAL_MS)) {
    return;
  }
  lastNetworkCheckMs = millis();
  bool apOk = isApHealthy();
  if (!apOk) {
    if (apHealthMisses < 255) apHealthMisses++;
    Serial.print("AP health miss: ");
    Serial.println(apHealthMisses);
  } else {
    apHealthMisses = 0;
  }
  if (apHealthMisses >= AP_HEALTH_MISS_LIMIT && elapsedSince(lastApRecoverMs, AP_RECOVER_MIN_INTERVAL_MS)) {
    if (lastApRecoverFailureMs != 0 && !elapsedSince(lastApRecoverFailureMs, AP_RECOVER_FAILURE_BACKOFF_MS)) {
      Serial.println("AP recover backoff active");
      return;
    }
    lastApRecoverMs = millis();
    Serial.println("AP unhealthy -> restarting AP");
    bool recovered = restartAccessPoint();
    apHealthMisses = recovered ? 0 : AP_HEALTH_MISS_LIMIT;
    if (recovered) {
      lastHttpRequestMs = millis();
    }
    return;
  }

  if (shouldRecoverFromHttpStall()) {
    if (!ENABLE_HTTP_STALL_RECOVERY) {
      Serial.println("HTTP stall detected -> recovery disabled (log only)");
      return;
    }
    Serial.println("HTTP stalled with active station -> restarting AP");
    if (restartAccessPoint()) {
      lastHttpRequestMs = millis();
    }
  }
}

void logRuntimeHealth() {
  if (!elapsedSince(lastRuntimeLogMs, RUNTIME_LOG_INTERVAL_MS)) return;
  lastRuntimeLogMs = millis();
  uint32_t heap = ESP.getFreeHeap();
  uint32_t httpIdleMs = lastHttpRequestMs == 0 ? 0 : (uint32_t)(millis() - lastHttpRequestMs);
  if (heap < minHeapSeen) {
    minHeapSeen = heap;
  }
  Serial.print("[HEALTH] uptimeMs=");
  Serial.print(millis());
  Serial.print(" heap=");
  Serial.print(heap);
  Serial.print(" minHeap=");
  Serial.print(minHeapSeen);
  Serial.print(" httpReq=");
  Serial.print(httpRequestCount);
  Serial.print(" httpIdleMs=");
  Serial.print(httpIdleMs);
  Serial.print(" httpStalls=");
  Serial.print(httpStallDetections);
  Serial.print(" apIP=");
  Serial.print(WiFi.softAPIP());
  Serial.print(" staStatus=");
  Serial.print((int)WiFi.status());
  Serial.print(" stations=");
  Serial.print(getStationCountCached());
  Serial.print(" apRecoverFails=");
  Serial.print(apRecoverFailureCount);
  Serial.print(" heapSafe=");
  Serial.println(heapEmergencyMode ? "on" : "off");
}

void guardHeapAndRecover() {
  if (!elapsedSince(lastHeapGuardMs, HEAP_GUARD_INTERVAL_MS)) return;
  lastHeapGuardMs = millis();
  uint32_t heap = ESP.getFreeHeap();
  if (heap <= HEAP_HARD_RESTART_THRESHOLD) {
    if (!heapEmergencyMode) {
      heapEmergencyMode = true;
      Serial.print("Heap critically low, entering safe mode. heap=");
      Serial.println(heap);
      ledState.userOn = false;
      ledState.effectiveOn = false;
      invalidateStatusCache();
      clearStripIfNeeded();
    }
    return;
  }

  if (heapEmergencyMode && heap > (HEAP_HARD_RESTART_THRESHOLD + 4000)) {
    heapEmergencyMode = false;
    Serial.print("Heap recovered, leaving safe mode. heap=");
    Serial.println(heap);
  }
}

void loadConfig() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    return;
  }
  if (!LittleFS.exists(CONFIG_FILE)) {
    Serial.println("No config exists (using defaults).");
    return;
  }
  File f = LittleFS.open(CONFIG_FILE, "r");
  if (!f) return;
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, f);
  f.close();
  if (error) {
    Serial.println("Failed to read config");
    return;
  }

  settings.timezone = doc["timezone"] | "Europe/Berlin";
  settings.liveRateEnabled = doc["liveRateEnabled"] | false;
  settings.liveIntervalMs = doc["liveIntervalMs"] | 4000;

  pumpState.durationMs = doc["pumpDurationMs"] | 5000;
  pumpState.manualEnabled = doc["manualPumpControlEnabled"] | true;
  pumpState.thresholdPercent = doc["moistureThresholdPercent"] | 35;
  pumpState.autoDurationMs = doc["autoPumpDurationMs"] | 5000;
  pumpState.autoCooldownMs = doc["autoPumpCooldownMs"] | 15000;

  ledState.userOn = doc["ledStripOn"] | false;
  ledState.r = doc["ledStripR"] | 255;
  ledState.g = doc["ledStripG"] | 64;
  ledState.b = doc["ledStripB"] | 12;
  ledState.brightnessPercent = doc["ledBrightnessPercent"] | 100;
  ledState.effect = parseEffect(doc["ledEffect"] | "static");
  ledState.effectSpeedMs = doc["ledEffectSpeedMs"] | 1200;
  ledState.scheduleEnabled = doc["lightScheduleEnabled"] | false;
  ledState.onMinute = doc["lightOnMinute"] | (18 * 60);
  ledState.offMinute = doc["lightOffMinute"] | (23 * 60);

  if (doc.containsKey("lastYear")) {
    clockState.year = doc["lastYear"];
    clockState.month = doc["lastMonth"];
    clockState.day = doc["lastDay"];
    clockState.hour = doc["lastHour"];
    clockState.minute = doc["lastMinute"];
    clockState.second = doc["lastSecond"];
    clockState.synced = true;
    clockState.lastTickMs = millis();
  }
  Serial.println("Persistent settings loaded.");
}

void saveConfig() {
  StaticJsonDocument<1024> doc;
  doc["timezone"] = settings.timezone;
  doc["liveRateEnabled"] = settings.liveRateEnabled;
  doc["liveIntervalMs"] = settings.liveIntervalMs;

  doc["pumpDurationMs"] = pumpState.durationMs;
  doc["manualPumpControlEnabled"] = pumpState.manualEnabled;
  doc["moistureThresholdPercent"] = pumpState.thresholdPercent;
  doc["autoPumpDurationMs"] = pumpState.autoDurationMs;
  doc["autoPumpCooldownMs"] = pumpState.autoCooldownMs;

  doc["ledStripOn"] = ledState.userOn;
  doc["ledStripR"] = ledState.r;
  doc["ledStripG"] = ledState.g;
  doc["ledStripB"] = ledState.b;
  doc["ledBrightnessPercent"] = ledState.brightnessPercent;
  doc["ledEffect"] = effectToString(ledState.effect);
  doc["ledEffectSpeedMs"] = ledState.effectSpeedMs;
  doc["lightScheduleEnabled"] = ledState.scheduleEnabled;
  doc["lightOnMinute"] = ledState.onMinute;
  doc["lightOffMinute"] = ledState.offMinute;

  if (clockState.synced) {
    doc["lastYear"] = clockState.year;
    doc["lastMonth"] = clockState.month;
    doc["lastDay"] = clockState.day;
    doc["lastHour"] = clockState.hour;
    doc["lastMinute"] = clockState.minute;
    doc["lastSecond"] = clockState.second;
  }

  File f = LittleFS.open(CONFIG_FILE, "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
    Serial.println("Persistent settings saved.");
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
  server.on("/api/time", HTTP_POST, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    handleApiTimePost(request, data, len, index, total);
  });
  server.on("/api/time", HTTP_OPTIONS, handleOptions);
  
  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    handleApiSettingsPost(request, data, len, index, total);
  });
  server.on("/api/settings", HTTP_OPTIONS, handleOptions);
  
  server.on("/api/pump", HTTP_POST, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    handleApiPumpPost(request, data, len, index, total);
  });
  server.on("/api/pump", HTTP_OPTIONS, handleOptions);
  
  server.on("/api/pump-duration", HTTP_POST, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    handleApiPumpDurationPost(request, data, len, index, total);
  });
  server.on("/api/pump-duration", HTTP_OPTIONS, handleOptions);
  
  server.on("/api/led", HTTP_POST, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    handleApiLedPost(request, data, len, index, total);
  });
  server.on("/api/led", HTTP_OPTIONS, handleOptions);
  
  server.on("/api/led/effect", HTTP_POST, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    handleApiLedEffectPost(request, data, len, index, total);
  });
  server.on("/api/led/effect", HTTP_OPTIONS, handleOptions);
  
  server.on("/api/garden/settings", HTTP_GET, handleGardenSettingsGet);
  server.on("/api/garden/settings", HTTP_POST, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    handleGardenSettingsPost(request, data, len, index, total);
  });
  server.on("/api/garden/settings", HTTP_OPTIONS, handleOptions);
  
  server.on("/generate_204", HTTP_GET, handleConnectivityProbe);
  server.on("/gen_204", HTTP_GET, handleConnectivityProbe);
  server.on("/hotspot-detect.html", HTTP_GET, handleConnectivityProbe);
  server.on("/connecttest.txt", HTTP_GET, handleConnectivityProbe);
  server.on("/ncsi.txt", HTTP_GET, handleConnectivityProbe);
  server.on("/fwlink", HTTP_GET, handleConnectivityProbe);
  server.on("/redirect", HTTP_GET, handleConnectivityProbe);
  
  server.onNotFound(handleNotFound);
}

void setupWifi() {
  WiFi.persistent(false);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setOutputPower(20.5f);
  staEnabled = ENABLE_STA && strlen(WIFI_SSID) > 0;
  WiFi.mode(staEnabled ? WIFI_AP_STA : WIFI_AP);
  if (staEnabled) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && !elapsedSince(start, 12000)) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
  }
  bool apOk = startAccessPointBestEffort();
  if (!apOk) {
    Serial.println("Failed to start AP.");
  }
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupHardware() {
  pinMode(PIN_PUMP, OUTPUT);
  stopPump();
  pinMode(PIN_LED_STRIP, OUTPUT);
  digitalWrite(PIN_LED_STRIP, LOW);
  delay(2);
  
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400); 
  FastLED.addLeds<NEOPIXEL, PIN_LED_STRIP>(leds, LED_COUNT);
  FastLED.setBrightness(LED_GLOBAL_BRIGHTNESS);
  FastLED.setDither(0);
  fill_solid(leds, LED_COUNT, CRGB::Black);
  showStripSafely();
  setupButtons();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println();
  Serial.println("=== Vertical Garden backend boot (AsyncWebServer) ===");
  Serial.print("Free heap at boot: ");
  Serial.println(ESP.getFreeHeap());
  loadConfig();
  setupHardware();
  setupWifi();
  setupRoutes();
  server.begin();
  Serial.print("HTTP async server listening on port ");
  Serial.println(HTTP_PORT);
  lastSensorSampleMs = millis() - SENSOR_SAMPLE_INTERVAL_MS;
  clockState.lastTickMs = millis();
  lastRuntimeLogMs = millis();
  lastHttpRequestMs = millis();
  lastHttpStallCheckMs = millis();
  lastHttpStallRecoverMs = millis();
  lastStationCountSampleMs = millis();
  cachedStationCount = WiFi.softAPgetStationNum();
  invalidateStatusCache();
}

void loop() {
  // AsyncWebServer handles requests automatically; no server.handleClient() needed!
  
  maintainNetwork();
  maintainDnsState();
  if (ENABLE_CAPTIVE_DNS && dnsStarted) {
    dnsServer.processNextRequest();
  }
  guardHeapAndRecover();
  updateClock();
  sampleMoisture();
  pollButtons();
  updatePumpRuntime();
  updateAutoPump();
  renderLedEffect();
  logRuntimeHealth();
  if (pendingConfigSave && elapsedSince(lastConfigChangeMs, 5000)) {
    saveConfig();
    pendingConfigSave = false;
  }
  yield();
}
