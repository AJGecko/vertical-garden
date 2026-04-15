#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_INTERRUPT_RETRY_COUNT 0

// Keep this enum above includes so Arduino auto-generated prototypes can
// reference LedEffect reliably.
enum LedEffect {
    LED_EFFECT_STATIC,
    LED_EFFECT_BLINK,
    LED_EFFECT_BREATHE,
    LED_EFFECT_WAVES,
    LED_EFFECT_RAINBOW
};

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#if ARDUINOJSON_VERSION_MAJOR >= 7
#define VG_JSON_DOC(name, capacity) JsonDocument name
#else
#define VG_JSON_DOC(name, capacity) StaticJsonDocument<capacity> name
#endif

// -----------------------------
// Network
// -----------------------------
const char *AP_PASSWORD = "vertical123";
const uint16_t HTTP_PORT = 80;
const byte DNS_PORT = 53;

const uint8_t AP_CHANNEL = 6;
const bool AP_HIDDEN = false;
const uint8_t AP_MAX_CLIENTS = 4;

IPAddress AP_IP(192, 168, 4, 1);
IPAddress AP_GATEWAY(192, 168, 4, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);

// -----------------------------
// Hardware
// -----------------------------
const uint8_t PIN_PUMP = D8;
const uint8_t PIN_MOISTURE = A0;
const uint8_t PIN_LED_STRIP = D1;
// Adjust to the physical strip length if needed.
const uint16_t LED_COUNT = 18;

// Creative reliability workaround:
// The first WS2812 pixel can act as a noisy "guard" pixel on long/noisy data lines.
// Set to 0 to use all pixels as visible LEDs.
const uint8_t LED_HEAD_GUARD_PIXELS = 1;
// If true, guard pixels mirror the first active pixel; otherwise they stay black.
const bool LED_GUARD_MIRROR_FIRST_ACTIVE = false;
// OFF/STATIC are sent multiple times to improve latch reliability on unstable strips.
const uint8_t LED_STATIC_OFF_SHOW_PASSES = 2;

const bool PUMP_ACTIVE_HIGH = true;

// Moisture calibration (adjust to your sensor).
const int MOISTURE_RAW_DRY = 860;
const int MOISTURE_RAW_WET = 420;

// -----------------------------
// Runtime timing
// -----------------------------
const uint32_t SENSOR_SAMPLE_INTERVAL_MS = 1000;
const uint32_t AP_HEALTH_CHECK_INTERVAL_MS = 5000;
const uint8_t AP_HEALTH_MISS_LIMIT = 6;
const uint32_t AP_RECOVERY_COOLDOWN_MS = 45000;

const uint32_t HTTP_STALL_CHECK_INTERVAL_MS = 5000;
const uint32_t HTTP_STALL_WINDOW_MS = 90000;
const uint32_t HTTP_STALL_RECOVERY_COOLDOWN_MS = 45000;
const uint8_t HTTP_STALL_CONFIRMATIONS = 3;
const uint32_t HTTP_STALL_MIN_REQUESTS = 10;
const bool ENABLE_HTTP_STALL_RECOVERY = false;
const size_t MAX_JSON_BODY_BYTES = 1536;

const uint32_t LED_MIN_FRAME_MS = 20;
const uint32_t LED_MAX_FRAME_MS = 140;
const uint32_t LED_STATIC_KEEPALIVE_MS = 1200;
const uint32_t LED_SHOW_MIN_INTERVAL_US = 350;

const uint32_t PUMP_DURATION_MIN_MS = 1000;
const uint32_t PUMP_DURATION_MAX_MS = 120000;
const uint32_t AUTO_COOLDOWN_MIN_MS = 1000;
const uint32_t AUTO_COOLDOWN_MAX_MS = 600000;

const uint32_t LED_EFFECT_SPEED_MIN_MS = 120;
const uint32_t LED_EFFECT_SPEED_MAX_MS = 10000;

AsyncWebServer server(HTTP_PORT);
DNSServer dnsServer;
CRGB leds[LED_COUNT];

String apSsid;
bool apStarted = false;
bool dnsStarted = false;

struct ControllerState {
    String timezone = "Europe/Berlin";
    String mode = "now";
    bool liveRateEnabled = false;
    uint32_t liveIntervalMs = 4000;
};

struct ClockState {
    bool valid = false;
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

ControllerState controllerState;
ClockState clockState;
PumpState pumpState;
LedState ledState;

int moistureRaw = 0;
uint8_t moisturePercent = 0;

uint32_t lastMoistureSampleMs = 0;
uint32_t lastLedFrameMs = 0;
uint32_t lastLedShowUs = 0;
bool ledDirty = true;

uint32_t lastHttpRequestMs = 0;
uint32_t httpRequestCount = 0;

uint32_t lastApHealthCheckMs = 0;
uint8_t apHealthMisses = 0;
uint32_t lastApRecoverMs = 0;

uint32_t lastHttpStallCheckMs = 0;
uint8_t httpStallDetections = 0;
uint32_t lastHttpStallRecoverMs = 0;

bool elapsedSince(uint32_t fromMs, uint32_t intervalMs) {
    return (uint32_t)(millis() - fromMs) >= intervalMs;
}

int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

uint32_t clampU32(uint32_t value, uint32_t minValue, uint32_t maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

bool parseBoolOrDefault(JsonVariantConst value, bool fallback) {
    if (value.isNull()) return fallback;
    if (value.is<bool>()) return value.as<bool>();
    if (value.is<int>()) return value.as<int>() != 0;
    if (value.is<long>()) return value.as<long>() != 0;
    if (value.is<const char *>()) {
        String normalized = String(value.as<const char *>());
        normalized.toLowerCase();
        if (normalized == "true" || normalized == "1" || normalized == "on") return true;
        if (normalized == "false" || normalized == "0" || normalized == "off") return false;
    }
    return fallback;
}

int parseIntOrDefault(JsonVariantConst value, int fallback) {
    if (value.isNull()) return fallback;
    if (value.is<int>()) return value.as<int>();
    if (value.is<long>()) return (int)value.as<long>();
    if (value.is<unsigned int>()) return (int)value.as<unsigned int>();
    if (value.is<unsigned long>()) return (int)value.as<unsigned long>();
    if (value.is<float>()) return (int)value.as<float>();
    if (value.is<const char *>()) return atoi(value.as<const char *>());
    return fallback;
}

String parseStringOrDefault(JsonVariantConst value, const String &fallback) {
    if (value.isNull()) return fallback;
    if (value.is<const char *>()) return String(value.as<const char *>());
    if (value.is<String>()) return value.as<String>();
    return fallback;
}

void markHttpActivity() {
    lastHttpRequestMs = millis();
    if (httpRequestCount < 0xFFFFFFFFUL) {
        httpRequestCount++;
    }
}

void addCorsHeaders(AsyncWebServerRequest *request, AsyncWebServerResponse *response) {
    String origin = "";
    if (request != nullptr && request->hasHeader("Origin")) {
        origin = request->getHeader("Origin")->value();
    }

    if (origin.length() > 0) {
        response->addHeader("Access-Control-Allow-Origin", origin);
        response->addHeader("Vary", "Origin");
    } else {
        response->addHeader("Access-Control-Allow-Origin", "*");
    }
    response->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    response->addHeader("Access-Control-Max-Age", "600");
    response->addHeader("Access-Control-Allow-Private-Network", "true");
    response->addHeader("Cache-Control", "no-store");
    response->addHeader("Connection", "close");
}

void sendJsonRaw(AsyncWebServerRequest *request, int statusCode, const String &body) {
    AsyncWebServerResponse *response = request->beginResponse(statusCode, "application/json", body);
    addCorsHeaders(request, response);
    markHttpActivity();
    request->send(response);
}

void sendJsonDocument(AsyncWebServerRequest *request, int statusCode, const JsonDocument &doc) {
    String body;
    serializeJson(doc, body);
    sendJsonRaw(request, statusCode, body);
}

void sendError(AsyncWebServerRequest *request, int statusCode, const char *message) {
    VG_JSON_DOC(doc, 192);
    doc["ok"] = false;
    doc["message"] = message;
    sendJsonDocument(request, statusCode, doc);
}

template <typename TDoc>
bool parseJsonPayload(const String &payload, TDoc &doc) {
    if (payload.length() == 0) return false;
    DeserializationError err = deserializeJson(doc, payload);
    return !err;
}

String *ensureBodyBuffer(AsyncWebServerRequest *request, size_t index, size_t total) {
    if (index == 0) {
        if (request->_tempObject != nullptr) {
            delete reinterpret_cast<String *>(request->_tempObject);
            request->_tempObject = nullptr;
        }

        String *buffer = new String();
        if (buffer == nullptr) {
            return nullptr;
        }
        buffer->reserve(total + 1);
        request->_tempObject = buffer;
        return buffer;
    }

    return reinterpret_cast<String *>(request->_tempObject);
}

void clearBodyBuffer(AsyncWebServerRequest *request) {
    if (request->_tempObject != nullptr) {
        delete reinterpret_cast<String *>(request->_tempObject);
        request->_tempObject = nullptr;
    }
}

const char *ledEffectToString(LedEffect effect) {
    switch (effect) {
        case LED_EFFECT_BLINK:
            return "blink";
        case LED_EFFECT_BREATHE:
            return "breathe";
        case LED_EFFECT_WAVES:
            return "waves";
        case LED_EFFECT_RAINBOW:
            return "rainbow";
        case LED_EFFECT_STATIC:
        default:
            return "static";
    }
}

LedEffect parseLedEffect(const String &value, LedEffect fallback) {
    String normalized = value;
    normalized.toLowerCase();
    normalized.trim();
    if (normalized == "static" || normalized == "solid" || normalized == "statisch" || normalized == "0") return LED_EFFECT_STATIC;
    if (normalized == "blink" || normalized == "flash" || normalized == "blinken" || normalized == "1") return LED_EFFECT_BLINK;
    if (normalized == "breathe" || normalized == "breath" || normalized == "atmen" || normalized == "2") return LED_EFFECT_BREATHE;
    if (normalized == "waves" || normalized == "wave" || normalized == "wellen" || normalized == "3") return LED_EFFECT_WAVES;
    if (normalized == "rainbow" || normalized == "regenbogen" || normalized == "4") return LED_EFFECT_RAINBOW;
    return fallback;
}

uint8_t scaleChannel(uint8_t channel, uint8_t brightnessPercent) {
    return (uint8_t)(((uint16_t)channel * (uint16_t)brightnessPercent) / 100U);
}

bool isLeapYear(int year) {
    if (year % 400 == 0) return true;
    if (year % 100 == 0) return false;
    return year % 4 == 0;
}

int daysInMonth(int year, int month) {
    static const int DAYS[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 30;
    if (month == 2) return isLeapYear(year) ? 29 : 28;
    return DAYS[month - 1];
}

bool parseIsoLocalTime(const String &value, int &year, int &month, int &day, int &hour, int &minute, int &second) {
    if (value.length() < 19) return false;

    int y = 0;
    int mo = 0;
    int d = 0;
    int h = 0;
    int mi = 0;
    int s = 0;

    int parsed = sscanf(value.c_str(), "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s);
    if (parsed != 6) {
        parsed = sscanf(value.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s);
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

int monthNameToNumber(const String &monthName) {
    if (monthName == "Jan") return 1;
    if (monthName == "Feb") return 2;
    if (monthName == "Mar") return 3;
    if (monthName == "Apr") return 4;
    if (monthName == "May") return 5;
    if (monthName == "Jun") return 6;
    if (monthName == "Jul") return 7;
    if (monthName == "Aug") return 8;
    if (monthName == "Sep") return 9;
    if (monthName == "Oct") return 10;
    if (monthName == "Nov") return 11;
    if (monthName == "Dec") return 12;
    return 1;
}

String buildIsoFromCompileTime() {
    String datePart = String(__DATE__); // Mmm dd yyyy
    String timePart = String(__TIME__); // HH:MM:SS

    int month = monthNameToNumber(datePart.substring(0, 3));
    int day = datePart.substring(4, 6).toInt();
    int year = datePart.substring(7, 11).toInt();

    if (day <= 0) day = 1;

    char out[20];
    snprintf(out, sizeof(out), "%04d-%02d-%02dT%s", year, month, day, timePart.c_str());
    return String(out);
}

void setClockFromIso(const String &isoTime) {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;

    if (!parseIsoLocalTime(isoTime, year, month, day, hour, minute, second)) {
        return;
    }

    clockState.year = year;
    clockState.month = month;
    clockState.day = day;
    clockState.hour = hour;
    clockState.minute = minute;
    clockState.second = second;
    clockState.valid = true;
    clockState.lastTickMs = millis();
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

    int monthDays = daysInMonth(clockState.year, clockState.month);
    if (clockState.day <= monthDays) return;

    clockState.day = 1;
    clockState.month++;
    if (clockState.month <= 12) return;

    clockState.month = 1;
    clockState.year++;
}

void updateClock() {
    if (!clockState.valid) return;
    while (elapsedSince(clockState.lastTickMs, 1000)) {
        clockState.lastTickMs += 1000;
        tickClockOneSecond();
    }
}

String currentLocalTimeIso() {
    if (!clockState.valid) return String("-");

    char buf[20];
    snprintf(
        buf,
        sizeof(buf),
        "%04d-%02d-%02dT%02d:%02d:%02d",
        clockState.year,
        clockState.month,
        clockState.day,
        clockState.hour,
        clockState.minute,
        clockState.second
    );
    return String(buf);
}

int currentMinuteOfDay() {
    if (!clockState.valid) return -1;
    return clockState.hour * 60 + clockState.minute;
}

int moistureRawToPercent(int rawValue) {
    int constrained = clampInt(rawValue, MOISTURE_RAW_WET, MOISTURE_RAW_DRY);
    int span = MOISTURE_RAW_DRY - MOISTURE_RAW_WET;
    if (span <= 0) return 0;

    int numerator = (MOISTURE_RAW_DRY - constrained) * 100 + (span / 2);
    int percent = numerator / span;
    return clampInt(percent, 0, 100);
}

void sampleMoistureIfNeeded() {
    if (!elapsedSince(lastMoistureSampleMs, SENSOR_SAMPLE_INTERVAL_MS)) return;
    lastMoistureSampleMs = millis();

    long sum = 0;
    const int sampleCount = 4;
    for (int i = 0; i < sampleCount; i++) {
        sum += analogRead(PIN_MOISTURE);
        yield();
    }

    moistureRaw = (int)(sum / sampleCount);
    moisturePercent = (uint8_t)moistureRawToPercent(moistureRaw);
}

void setPumpOutput(bool on) {
    uint8_t activeLevel = PUMP_ACTIVE_HIGH ? HIGH : LOW;
    uint8_t inactiveLevel = PUMP_ACTIVE_HIGH ? LOW : HIGH;
    digitalWrite(PIN_PUMP, on ? activeLevel : inactiveLevel);
}

void stopPump() {
    pumpState.on = false;
    pumpState.endAtMs = 0;
    setPumpOutput(false);
}

void startPump(uint32_t durationMs, bool autoTriggered) {
    uint32_t safeDuration = clampU32(durationMs, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
    pumpState.on = true;
    pumpState.endAtMs = millis() + safeDuration;
    setPumpOutput(true);
    if (autoTriggered) {
        pumpState.lastAutoTriggerMs = millis();
    }
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

void updateAutoPump() {
    if (pumpState.manualEnabled) return;
    if (pumpState.on) return;

    bool moistureTooLow = moisturePercent < pumpState.thresholdPercent;
    bool cooldownOver = elapsedSince(pumpState.lastAutoTriggerMs, pumpState.autoCooldownMs);

    if (moistureTooLow && cooldownOver) {
        startPump(pumpState.autoDurationMs, true);
    }
}

bool isMinuteInWindow(int nowMinute, int onMinute, int offMinute) {
    if (onMinute == offMinute) return true;
    if (onMinute < offMinute) {
        return nowMinute >= onMinute && nowMinute < offMinute;
    }
    return nowMinute >= onMinute || nowMinute < offMinute;
}

void requestLedRender() {
    ledDirty = true;
}

void showLedsSafely() {
    uint32_t nowUs = micros();
    uint32_t elapsedUs = nowUs - lastLedShowUs;
    if (elapsedUs < LED_SHOW_MIN_INTERVAL_US) {
        delayMicroseconds(LED_SHOW_MIN_INTERVAL_US - elapsedUs);
    }
    FastLED.show();
    lastLedShowUs = micros();
}

void showLedsCommitted(uint8_t passes) {
    uint8_t safePasses = passes < 1 ? 1 : passes;
    for (uint8_t i = 0; i < safePasses; i++) {
        showLedsSafely();
        if (i + 1 < safePasses) {
            delayMicroseconds(120);
        }
    }
}

uint16_t activeLedCount() {
    if (LED_COUNT <= LED_HEAD_GUARD_PIXELS) return 0;
    return (uint16_t)(LED_COUNT - LED_HEAD_GUARD_PIXELS);
}

void applyGuardPixels() {
    if (LED_HEAD_GUARD_PIXELS == 0 || LED_COUNT == 0) return;

    CRGB guardColor = CRGB::Black;
    uint16_t activeCount = activeLedCount();
    if (LED_GUARD_MIRROR_FIRST_ACTIVE && activeCount > 0) {
        guardColor = leds[LED_HEAD_GUARD_PIXELS];
    }

    uint8_t guardCount = LED_HEAD_GUARD_PIXELS;
    if (guardCount > LED_COUNT) guardCount = LED_COUNT;
    for (uint8_t i = 0; i < guardCount; i++) {
        leds[i] = guardColor;
    }
}

void fillLeds(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t activeCount = activeLedCount();
    if (activeCount == 0) {
        fill_solid(leds, LED_COUNT, CRGB::Black);
        return;
    }

    fill_solid(leds + LED_HEAD_GUARD_PIXELS, activeCount, CRGB(r, g, b));
    applyGuardPixels();
}

void renderLedFrame(bool forceRender) {
    bool scheduleAllowsOn = true;
    if (ledState.scheduleEnabled) {
        int nowMinute = currentMinuteOfDay();
        if (nowMinute >= 0) {
            scheduleAllowsOn = isMinuteInWindow(nowMinute, ledState.onMinute, ledState.offMinute);
        }
    }

    bool shouldBeOn = ledState.scheduleEnabled ? scheduleAllowsOn : ledState.userOn;
    bool stateChanged = (ledState.effectiveOn != shouldBeOn);
    if (stateChanged) {
        ledState.effectiveOn = shouldBeOn;
        forceRender = true;  // Force immediate render on state change
    }

    uint32_t intervalMs = LED_STATIC_KEEPALIVE_MS;
    if (ledState.effectiveOn && ledState.effect != LED_EFFECT_STATIC) {
        // Calculate frame interval: smooth animations need ~30+ fps for effects
        // speedMs is the full cycle duration; divide into ~30 frames for smooth rendering
        intervalMs = clampU32(ledState.effectSpeedMs / 30U, LED_MIN_FRAME_MS, LED_MAX_FRAME_MS);
    }

    // Skip frame if not dirty and interval hasn't elapsed
    if (!forceRender && !ledDirty && !elapsedSince(lastLedFrameMs, intervalMs)) {
        return;
    }

    lastLedFrameMs = millis();
    ledDirty = false;

    if (!ledState.effectiveOn || ledState.brightnessPercent == 0) {
        // Light is off: ensure all LEDs are dark and show multiple times for reliability
        fillLeds(0, 0, 0);
        showLedsCommitted(LED_STATIC_OFF_SHOW_PASSES);
        return;
    }

    // Scale base color by brightness percentage for all effects
    uint8_t baseR = scaleChannel(ledState.r, ledState.brightnessPercent);
    uint8_t baseG = scaleChannel(ledState.g, ledState.brightnessPercent);
    uint8_t baseB = scaleChannel(ledState.b, ledState.brightnessPercent);

    uint32_t speedMs = clampU32(ledState.effectSpeedMs, LED_EFFECT_SPEED_MIN_MS, LED_EFFECT_SPEED_MAX_MS);
    uint32_t nowMs = millis();

    if (ledState.effect == LED_EFFECT_STATIC) {
        // Static color: solid fill, show multiple times for reliability
        fillLeds(baseR, baseG, baseB);
        showLedsCommitted(LED_STATIC_OFF_SHOW_PASSES);
        return;
    } else if (ledState.effect == LED_EFFECT_BLINK) {
        // Blink effect: 50% on, 50% off over the speed cycle
        uint32_t halfCycle = speedMs / 2U;
        if (halfCycle < 50U) halfCycle = 50U;  // Minimum 50ms half-cycle for stability
        bool onPhase = ((nowMs / halfCycle) % 2U) == 0U;
        if (onPhase) {
            fillLeds(baseR, baseG, baseB);
        } else {
            fillLeds(0, 0, 0);
        }
    } else if (ledState.effect == LED_EFFECT_BREATHE) {
        // Natural breathing: phase 0->255 creates one smooth sine cycle
        uint8_t phase = (uint8_t)(((nowMs % speedMs) * 255UL) / speedMs);
        uint8_t wave = sin8(phase);  // Natural sine: starts at 0, peaks at 127, ends near 0
        uint8_t r = (uint8_t)(((uint16_t)baseR * wave) / 255U);
        uint8_t g = (uint8_t)(((uint16_t)baseG * wave) / 255U);
        uint8_t b = (uint8_t)(((uint16_t)baseB * wave) / 255U);
        fillLeds(r, g, b);
    } else if (ledState.effect == LED_EFFECT_WAVES) {
        uint16_t activeCount = activeLedCount();
        if (activeCount == 0) {
            fillLeds(0, 0, 0);
            showLedsCommitted(LED_STATIC_OFF_SHOW_PASSES);
            return;
        }

        // Waves: smooth sine wave propagates across strip, phase shifts over speed cycle
        uint8_t basePhase = (uint8_t)(((nowMs % speedMs) * 255UL) / speedMs);
        for (uint16_t i = 0; i < activeCount; i++) {
            // Each LED offset by (255/count) creates smooth wave pattern
            uint8_t wave = sin8(basePhase + (uint8_t)((i * 255U) / activeCount));
            uint16_t physical = (uint16_t)(i + LED_HEAD_GUARD_PIXELS);
            leds[physical].r = (uint8_t)(((uint16_t)baseR * wave) / 255U);
            leds[physical].g = (uint8_t)(((uint16_t)baseG * wave) / 255U);
            leds[physical].b = (uint8_t)(((uint16_t)baseB * wave) / 255U);
        }
        applyGuardPixels();
    } else if (ledState.effect == LED_EFFECT_RAINBOW) {
        uint16_t activeCount = activeLedCount();
        if (activeCount == 0) {
            fillLeds(0, 0, 0);
            showLedsCommitted(LED_STATIC_OFF_SHOW_PASSES);
            return;
        }

        // Rainbow: full hue spectrum distributed across strip, rotates over speed cycle
        uint8_t value = (uint8_t)(((uint16_t)ledState.brightnessPercent * 255U) / 100U);
        uint8_t baseHue = (uint8_t)(((nowMs % speedMs) * 255UL) / speedMs);
        for (uint16_t i = 0; i < activeCount; i++) {
            // Each LED offset by hue creates full spectrum; hue shifts per baseHue
            uint8_t hue = baseHue + (uint8_t)((i * 255U) / activeCount);
            uint16_t physical = (uint16_t)(i + LED_HEAD_GUARD_PIXELS);
            leds[physical] = CHSV(hue, 255, value);
        }
        applyGuardPixels();
    }

    showLedsSafely();
    yield();
}

bool isAccessPointHealthy() {
    if (!apStarted) return false;

    WiFiMode_t mode = WiFi.getMode();
    bool apModeEnabled = (mode == WIFI_AP || mode == WIFI_AP_STA);
    if (!apModeEnabled) return false;

    IPAddress ip = WiFi.softAPIP();
    if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) return false;
    if (WiFi.softAPSSID().length() == 0 && apSsid.length() == 0) return false;
    return true;
}

bool startAccessPoint() {
    WiFi.persistent(false);
    WiFi.disconnect(false);
    WiFi.softAPdisconnect(true);
    delay(60);

    WiFi.mode(WIFI_AP);
    delay(40);

    apSsid = String("VerticalGarden-") + String(ESP.getChipId(), HEX);
    apSsid.toUpperCase();

    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    apStarted = WiFi.softAP(apSsid.c_str(), AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CLIENTS);
    if (!apStarted) {
        apStarted = WiFi.softAP(apSsid.c_str());
    }

    if (!apStarted) {
        dnsStarted = false;
        return false;
    }

    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsStarted = true;
    return true;
}

bool restartAccessPoint() {
    if (dnsStarted) {
        dnsServer.stop();
        dnsStarted = false;
    }

    WiFi.softAPdisconnect(true);
    WiFi.disconnect(false);
    delay(60);

    WiFi.mode(WIFI_OFF);
    delay(120);

    return startAccessPoint();
}

void maintainAccessPointHealth() {
    if (elapsedSince(lastApHealthCheckMs, AP_HEALTH_CHECK_INTERVAL_MS)) {
        lastApHealthCheckMs = millis();
        if (!isAccessPointHealthy()) {
            if (apHealthMisses < 255) apHealthMisses++;
        } else {
            apHealthMisses = 0;
        }

        if (apHealthMisses >= AP_HEALTH_MISS_LIMIT && elapsedSince(lastApRecoverMs, AP_RECOVERY_COOLDOWN_MS)) {
            lastApRecoverMs = millis();
            restartAccessPoint();
            apHealthMisses = 0;
        }
    }

    if (!ENABLE_HTTP_STALL_RECOVERY) {
        httpStallDetections = 0;
        return;
    }

    if (!elapsedSince(lastHttpStallCheckMs, HTTP_STALL_CHECK_INTERVAL_MS)) {
        return;
    }
    lastHttpStallCheckMs = millis();

    uint8_t stations = WiFi.softAPgetStationNum();
    bool enoughHistory = httpRequestCount >= HTTP_STALL_MIN_REQUESTS;
    bool stalled = stations > 0 && enoughHistory && elapsedSince(lastHttpRequestMs, HTTP_STALL_WINDOW_MS);

    if (!stalled) {
        httpStallDetections = 0;
        return;
    }

    if (httpStallDetections < 255) httpStallDetections++;

    if (httpStallDetections >= HTTP_STALL_CONFIRMATIONS
        && elapsedSince(lastHttpStallRecoverMs, HTTP_STALL_RECOVERY_COOLDOWN_MS)) {
        lastHttpStallRecoverMs = millis();
        restartAccessPoint();
        httpStallDetections = 0;
    }
}

void fillStatusDocument(JsonDocument &doc) {
    doc["timezone"] = controllerState.timezone;
    doc["mode"] = controllerState.mode;
    doc["localTime"] = currentLocalTimeIso();
    doc["liveRateEnabled"] = controllerState.liveRateEnabled;
    doc["liveIntervalMs"] = controllerState.liveIntervalMs;
    doc["apSsid"] = apSsid;
    doc["apIp"] = WiFi.softAPIP().toString();
    doc["httpPort"] = HTTP_PORT;
    doc["uptimeMs"] = millis();

    JsonObject pump = doc.createNestedObject("pump");
    pump["on"] = pumpState.on;
    pump["durationMs"] = pumpState.durationMs;
    pump["remainingMs"] = pumpRemainingMs();
    pump["manualEnabled"] = pumpState.manualEnabled;
    pump["autoEnabled"] = !pumpState.manualEnabled;
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
    led["brightnessPercent"] = ledState.brightnessPercent;
    led["effect"] = ledEffectToString(ledState.effect);
    led["effectSpeedMs"] = ledState.effectSpeedMs;
    led["scheduleEnabled"] = ledState.scheduleEnabled;
    led["scheduleOnMinute"] = ledState.onMinute;
    led["scheduleOffMinute"] = ledState.offMinute;

    // Flat compatibility fields used by the current frontend.
    doc["pumpOn"] = pumpState.on;
    doc["pumpDurationMs"] = pumpState.durationMs;
    doc["pumpRemainingMs"] = pumpRemainingMs();
    doc["manualPumpControlEnabled"] = pumpState.manualEnabled;
    doc["autoPumpEnabled"] = !pumpState.manualEnabled;
    doc["moistureThresholdPercent"] = pumpState.thresholdPercent;
    doc["autoPumpDurationMs"] = pumpState.autoDurationMs;
    doc["autoPumpCooldownMs"] = pumpState.autoCooldownMs;

    doc["moistureRaw"] = moistureRaw;
    doc["moisturePercent"] = moisturePercent;

    doc["ledStripOn"] = ledState.effectiveOn;
    doc["ledStripR"] = ledState.r;
    doc["ledStripG"] = ledState.g;
    doc["ledStripB"] = ledState.b;
    doc["ledStripBrightnessPercent"] = ledState.brightnessPercent;
    doc["ledBrightnessPercent"] = ledState.brightnessPercent;
    doc["ledEffect"] = ledEffectToString(ledState.effect);
    doc["ledEffectSpeedMs"] = ledState.effectSpeedMs;
    doc["lightScheduleEnabled"] = ledState.scheduleEnabled;
    doc["lightOnMinute"] = ledState.onMinute;
    doc["lightOffMinute"] = ledState.offMinute;
}

void fillGardenSettingsDocument(JsonDocument &doc) {
    doc["ok"] = true;
    doc["manualPumpControlEnabled"] = pumpState.manualEnabled;
    doc["autoPumpEnabled"] = !pumpState.manualEnabled;
    doc["moistureThresholdPercent"] = pumpState.thresholdPercent;
    doc["autoPumpDurationMs"] = pumpState.autoDurationMs;
    doc["autoPumpCooldownMs"] = pumpState.autoCooldownMs;
    doc["lightScheduleEnabled"] = ledState.scheduleEnabled;
    doc["lightOnMinute"] = ledState.onMinute;
    doc["lightOffMinute"] = ledState.offMinute;
    doc["ledEffect"] = ledEffectToString(ledState.effect);
    doc["ledEffectSpeedMs"] = ledState.effectSpeedMs;
    doc["brightnessPercent"] = ledState.brightnessPercent;
    doc["ledStripBrightnessPercent"] = ledState.brightnessPercent;
    doc["ledBrightnessPercent"] = ledState.brightnessPercent;

    doc["ledStripOn"] = ledState.effectiveOn;
    doc["ledStripR"] = ledState.r;
    doc["ledStripG"] = ledState.g;
    doc["ledStripB"] = ledState.b;

    doc["on"] = ledState.effectiveOn;
    doc["r"] = ledState.r;
    doc["g"] = ledState.g;
    doc["b"] = ledState.b;
    doc["effect"] = ledEffectToString(ledState.effect);
    doc["effectSpeedMs"] = ledState.effectSpeedMs;
}

void handleOptions(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(204, "text/plain", "");
    addCorsHeaders(request, response);
    markHttpActivity();
    request->send(response);
}

void handleRoot(AsyncWebServerRequest *request) {
    String page = "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Vertical Garden Backend</title></head><body style=\"font-family:Arial,sans-serif;padding:16px\"><h2>Vertical Garden Backend</h2><p>API ready</p><ul><li>SSID: ";
    page += apSsid;
    page += "</li><li>IP: ";
    page += WiFi.softAPIP().toString();
    page += "</li><li>Port: 80</li></ul></body></html>";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=utf-8", page);
    addCorsHeaders(request, response);
    markHttpActivity();
    request->send(response);
}

void handleStatusLike(AsyncWebServerRequest *request) {
    VG_JSON_DOC(doc, 1600);
    fillStatusDocument(doc);
    sendJsonDocument(request, 200, doc);
}

void handleGetTime(AsyncWebServerRequest *request) {
    VG_JSON_DOC(doc, 256);
    doc["timezone"] = controllerState.timezone;
    doc["mode"] = controllerState.mode;
    doc["localTime"] = currentLocalTimeIso();
    sendJsonDocument(request, 200, doc);
}

void handlePostSettings(AsyncWebServerRequest *request, const String &payload) {
    VG_JSON_DOC(body, 512);
    if (!parseJsonPayload(payload, body)) {
        sendError(request, 400, "invalid json body");
        return;
    }

    controllerState.timezone = parseStringOrDefault(body["timezone"], controllerState.timezone);
    controllerState.liveRateEnabled = parseBoolOrDefault(body["liveRateEnabled"], controllerState.liveRateEnabled);
    controllerState.liveIntervalMs = clampU32(
        (uint32_t)parseIntOrDefault(body["liveIntervalMs"], (int)controllerState.liveIntervalMs),
        250,
        60000
    );

    VG_JSON_DOC(response, 320);
    response["ok"] = true;
    response["timezone"] = controllerState.timezone;
    response["liveRateEnabled"] = controllerState.liveRateEnabled;
    response["liveIntervalMs"] = controllerState.liveIntervalMs;
    sendJsonDocument(request, 200, response);
}

void handlePostTime(AsyncWebServerRequest *request, const String &payload) {
    VG_JSON_DOC(body, 512);
    if (!parseJsonPayload(payload, body)) {
        sendError(request, 400, "invalid json body");
        return;
    }

    String timezone = parseStringOrDefault(body["timezone"], controllerState.timezone);
    String mode = parseStringOrDefault(body["mode"], controllerState.mode);
    String localTime = parseStringOrDefault(body["localTime"], "");

    if (localTime.length() == 0) {
        sendError(request, 400, "localTime missing");
        return;
    }

    int y = 0;
    int mo = 0;
    int d = 0;
    int h = 0;
    int mi = 0;
    int s = 0;
    if (!parseIsoLocalTime(localTime, y, mo, d, h, mi, s)) {
        sendError(request, 400, "invalid localTime format");
        return;
    }

    controllerState.timezone = timezone;
    controllerState.mode = (mode == "manual") ? "manual" : "now";
    setClockFromIso(localTime);
    requestLedRender();

    VG_JSON_DOC(response, 320);
    response["ok"] = true;
    response["timezone"] = controllerState.timezone;
    response["mode"] = controllerState.mode;
    response["localTime"] = currentLocalTimeIso();
    sendJsonDocument(request, 200, response);
}

void handlePostPump(AsyncWebServerRequest *request, const String &payload) {
    if (!pumpState.manualEnabled) {
        sendError(request, 403, "manual control disabled");
        return;
    }

    VG_JSON_DOC(body, 512);
    if (!parseJsonPayload(payload, body)) {
        sendError(request, 400, "invalid json body");
        return;
    }

    String action = parseStringOrDefault(body["action"], "toggle");
    action.toLowerCase();

    uint32_t durationMs = clampU32(
        (uint32_t)parseIntOrDefault(body["durationMs"], (int)pumpState.durationMs),
        PUMP_DURATION_MIN_MS,
        PUMP_DURATION_MAX_MS
    );

    if (action == "toggle") {
        if (pumpState.on) {
            stopPump();
        } else {
            startPump(durationMs, false);
        }
    } else if (action == "start" || action == "run" || action == "on") {
        startPump(durationMs, false);
    } else if (action == "stop" || action == "off") {
        stopPump();
    } else {
        bool onFlag = parseBoolOrDefault(body["on"], false);
        if (onFlag) {
            startPump(durationMs, false);
        } else {
            stopPump();
        }
    }

    pumpState.durationMs = durationMs;

    VG_JSON_DOC(response, 320);
    response["ok"] = true;
    response["pumpOn"] = pumpState.on;
    response["pumpDurationMs"] = pumpState.durationMs;
    response["pumpRemainingMs"] = pumpRemainingMs();
    sendJsonDocument(request, 200, response);
}

void handlePostPumpDuration(AsyncWebServerRequest *request, const String &payload) {
    VG_JSON_DOC(body, 256);
    if (!parseJsonPayload(payload, body)) {
        sendError(request, 400, "invalid json body");
        return;
    }

    pumpState.durationMs = clampU32(
        (uint32_t)parseIntOrDefault(body["pumpDurationMs"], (int)pumpState.durationMs),
        PUMP_DURATION_MIN_MS,
        PUMP_DURATION_MAX_MS
    );

    VG_JSON_DOC(response, 192);
    response["ok"] = true;
    response["pumpDurationMs"] = pumpState.durationMs;
    sendJsonDocument(request, 200, response);
}

void applyLedPayload(const JsonDocument &body) {
    bool hasR = !body["r"].isNull() || !body["ledStripR"].isNull();
    bool hasG = !body["g"].isNull() || !body["ledStripG"].isNull();
    bool hasB = !body["b"].isNull() || !body["ledStripB"].isNull();
    bool hasColorPayload = hasR || hasG || hasB;
    bool hasEffectPayload = !body["effect"].isNull() || !body["ledEffect"].isNull() || !body["mode"].isNull();
    bool hasSpeedPayload = !body["effectSpeedMs"].isNull() || !body["ledEffectSpeedMs"].isNull() || !body["speedMs"].isNull();

    bool hasOn = !body["on"].isNull() || !body["ledStripOn"].isNull();
    bool targetOn = parseBoolOrDefault(body["on"], parseBoolOrDefault(body["ledStripOn"], ledState.userOn));
    if (hasOn) {
        ledState.userOn = targetOn;
    }

    // Manual light actions should not stay blocked by schedule overrides.
    if (hasOn || hasColorPayload || hasEffectPayload || hasSpeedPayload) {
        ledState.scheduleEnabled = false;
    }

    int baseR = parseIntOrDefault(body["r"], parseIntOrDefault(body["ledStripR"], ledState.r));
    int baseG = parseIntOrDefault(body["g"], parseIntOrDefault(body["ledStripG"], ledState.g));
    int baseB = parseIntOrDefault(body["b"], parseIntOrDefault(body["ledStripB"], ledState.b));

    ledState.r = (uint8_t)clampInt(baseR, 0, 255);
    ledState.g = (uint8_t)clampInt(baseG, 0, 255);
    ledState.b = (uint8_t)clampInt(baseB, 0, 255);

    int brightness = parseIntOrDefault(
        body["brightnessPercent"],
        parseIntOrDefault(
            body["ledBrightnessPercent"],
            parseIntOrDefault(body["ledStripBrightnessPercent"], parseIntOrDefault(body["brightness"], ledState.brightnessPercent))
        )
    );
    ledState.brightnessPercent = (uint8_t)clampInt(brightness, 0, 100);

    bool hasVisibleColor = ledState.r > 0 || ledState.g > 0 || ledState.b > 0;

    // Tolerate payloads that update color without explicitly setting "on".
    if (!hasOn && hasColorPayload && hasVisibleColor) {
        ledState.userOn = true;
    }

    // Effect/speed updates from panel should also wake the light unless explicitly turned off.
    if (!hasOn && (hasEffectPayload || hasSpeedPayload) && ledState.brightnessPercent > 0) {
        ledState.userOn = true;
    }

    // If ON is requested with zero RGB, fall back to white so the change is visible.
    if (ledState.userOn && ledState.brightnessPercent > 0 && !hasVisibleColor) {
        ledState.r = 255;
        ledState.g = 255;
        ledState.b = 255;
    }

    if (hasEffectPayload) {
        String effectName = parseStringOrDefault(
            body["effect"],
            parseStringOrDefault(body["ledEffect"], parseStringOrDefault(body["mode"], String(ledEffectToString(ledState.effect))))
        );
        // If a client explicitly sends an unknown effect value, default to static.
        ledState.effect = parseLedEffect(effectName, LED_EFFECT_STATIC);
    }

    if (!body["effectSpeedMs"].isNull() || !body["ledEffectSpeedMs"].isNull() || !body["speedMs"].isNull()) {
        uint32_t speed = (uint32_t)parseIntOrDefault(body["effectSpeedMs"], parseIntOrDefault(body["ledEffectSpeedMs"], parseIntOrDefault(body["speedMs"], (int)ledState.effectSpeedMs)));
        ledState.effectSpeedMs = clampU32(speed, LED_EFFECT_SPEED_MIN_MS, LED_EFFECT_SPEED_MAX_MS);
    }

    requestLedRender();
}

void handlePostLedEffect(AsyncWebServerRequest *request, const String &payload) {
    VG_JSON_DOC(body, 768);
    if (!parseJsonPayload(payload, body)) {
        sendError(request, 400, "invalid json body");
        return;
    }

    applyLedPayload(body);
    renderLedFrame(true);

    VG_JSON_DOC(response, 512);
    response["ok"] = true;
    response["on"] = ledState.effectiveOn;
    response["r"] = ledState.r;
    response["g"] = ledState.g;
    response["b"] = ledState.b;
    response["brightnessPercent"] = ledState.brightnessPercent;
    response["effect"] = ledEffectToString(ledState.effect);
    response["effectSpeedMs"] = ledState.effectSpeedMs;
    response["ledStripOn"] = ledState.effectiveOn;
    response["ledStripR"] = ledState.r;
    response["ledStripG"] = ledState.g;
    response["ledStripB"] = ledState.b;
    response["ledEffect"] = ledEffectToString(ledState.effect);
    response["ledEffectSpeedMs"] = ledState.effectSpeedMs;
    sendJsonDocument(request, 200, response);
}

void handleGetGardenSettings(AsyncWebServerRequest *request) {
    VG_JSON_DOC(doc, 512);
    fillGardenSettingsDocument(doc);
    sendJsonDocument(request, 200, doc);
}

void handlePostGardenSettings(AsyncWebServerRequest *request, const String &payload) {
    VG_JSON_DOC(body, 768);
    if (!parseJsonPayload(payload, body)) {
        sendError(request, 400, "invalid json body");
        return;
    }

    if (!body["manualPumpControlEnabled"].isNull()) {
        pumpState.manualEnabled = parseBoolOrDefault(body["manualPumpControlEnabled"], pumpState.manualEnabled);
    } else if (!body["autoPumpEnabled"].isNull()) {
        bool autoEnabled = parseBoolOrDefault(body["autoPumpEnabled"], !pumpState.manualEnabled);
        pumpState.manualEnabled = !autoEnabled;
    }

    pumpState.thresholdPercent = (uint8_t)clampInt(
        parseIntOrDefault(body["moistureThresholdPercent"], pumpState.thresholdPercent),
        0,
        100
    );

    pumpState.autoDurationMs = clampU32(
        (uint32_t)parseIntOrDefault(body["autoPumpDurationMs"], (int)pumpState.autoDurationMs),
        PUMP_DURATION_MIN_MS,
        PUMP_DURATION_MAX_MS
    );

    pumpState.autoCooldownMs = clampU32(
        (uint32_t)parseIntOrDefault(body["autoPumpCooldownMs"], (int)pumpState.autoCooldownMs),
        AUTO_COOLDOWN_MIN_MS,
        AUTO_COOLDOWN_MAX_MS
    );

    if (!body["lightScheduleEnabled"].isNull()) {
        ledState.scheduleEnabled = parseBoolOrDefault(body["lightScheduleEnabled"], ledState.scheduleEnabled);
    }

    ledState.onMinute = (uint16_t)clampInt(parseIntOrDefault(body["lightOnMinute"], ledState.onMinute), 0, 1439);
    ledState.offMinute = (uint16_t)clampInt(parseIntOrDefault(body["lightOffMinute"], ledState.offMinute), 0, 1439);

    if (!body["ledEffect"].isNull() || !body["effect"].isNull()) {
        String effectName = parseStringOrDefault(body["ledEffect"], parseStringOrDefault(body["effect"], String(ledEffectToString(ledState.effect))));
        ledState.effect = parseLedEffect(effectName, ledState.effect);
    }

    if (!body["ledEffectSpeedMs"].isNull() || !body["effectSpeedMs"].isNull()) {
        uint32_t speed = (uint32_t)parseIntOrDefault(body["ledEffectSpeedMs"], parseIntOrDefault(body["effectSpeedMs"], (int)ledState.effectSpeedMs));
        ledState.effectSpeedMs = clampU32(speed, LED_EFFECT_SPEED_MIN_MS, LED_EFFECT_SPEED_MAX_MS);
    }

    requestLedRender();

    VG_JSON_DOC(response, 512);
    fillGardenSettingsDocument(response);
    sendJsonDocument(request, 200, response);
}

void handleConnectivityProbe(AsyncWebServerRequest *request) {
    String location = String("http://") + WiFi.softAPIP().toString() + "/";
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting to setup portal");
    response->addHeader("Location", location);
    addCorsHeaders(request, response);
    markHttpActivity();
    request->send(response);
}

void handleNotFound(AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
        handleOptions(request);
        return;
    }

    if (request->method() == HTTP_GET && apStarted) {
        handleConnectivityProbe(request);
        return;
    }

    sendError(request, 404, "not found");
}

void registerJsonPostRoute(const char *path, void (*handler)(AsyncWebServerRequest *, const String &)) {
    server.on(
        path,
        HTTP_POST,
        [](AsyncWebServerRequest *request) {
            size_t contentLength = request->contentLength();
            if (contentLength == 0) {
                sendError(request, 400, "invalid json body");
                return;
            }
            if (contentLength > MAX_JSON_BODY_BYTES) {
                sendError(request, 413, "json body too large");
            }
        },
        nullptr,
        [handler](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            if (total > MAX_JSON_BODY_BYTES) {
                if (index == 0) {
                    sendError(request, 413, "json body too large");
                }
                clearBodyBuffer(request);
                return;
            }

            String *body = ensureBodyBuffer(request, index, total);
            if (body == nullptr) {
                sendError(request, 500, "body buffer allocation failed");
                clearBodyBuffer(request);
                return;
            }

            if (len > 0) {
                if (!body->concat(reinterpret_cast<const char *>(data), len)) {
                    sendError(request, 413, "json body too large");
                    clearBodyBuffer(request);
                    return;
                }
            }

            if ((index + len) < total) {
                return;
            }

            String payload = *body;
            clearBodyBuffer(request);
            handler(request, payload);
        }
    );
}

void setupRoutes() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { handleRoot(request); });

    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) { handleStatusLike(request); });
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) { handleStatusLike(request); });
    server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest *request) { handleStatusLike(request); });

    server.on("/api/time", HTTP_GET, [](AsyncWebServerRequest *request) { handleGetTime(request); });

    registerJsonPostRoute("/api/time", handlePostTime);
    registerJsonPostRoute("/api/settings", handlePostSettings);
    registerJsonPostRoute("/api/pump", handlePostPump);
    registerJsonPostRoute("/api/pump-duration", handlePostPumpDuration);
    registerJsonPostRoute("/api/led", handlePostLedEffect);
    registerJsonPostRoute("/api/led/effect", handlePostLedEffect);
    registerJsonPostRoute("/api/garden/settings", handlePostGardenSettings);

    server.on("/api/garden/settings", HTTP_GET, [](AsyncWebServerRequest *request) { handleGetGardenSettings(request); });

    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) { handleConnectivityProbe(request); });
    server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest *request) { handleConnectivityProbe(request); });
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) { handleConnectivityProbe(request); });
    server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) { handleConnectivityProbe(request); });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) { handleConnectivityProbe(request); });
    server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *request) { handleConnectivityProbe(request); });
    server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request) { handleConnectivityProbe(request); });

    server.onNotFound(handleNotFound);
}

void setup() {
    Serial.begin(115200);
    delay(120);

    pinMode(PIN_PUMP, OUTPUT);
    stopPump();

    pinMode(PIN_LED_STRIP, OUTPUT);
    digitalWrite(PIN_LED_STRIP, LOW);
    delay(2);

    FastLED.addLeds<NEOPIXEL, PIN_LED_STRIP>(leds, LED_COUNT);
    FastLED.setBrightness(255);
    FastLED.setDither(0);
    fillLeds(0, 0, 0);
    showLedsSafely();

    setClockFromIso(buildIsoFromCompileTime());
    sampleMoistureIfNeeded();
    requestLedRender();

    bool started = startAccessPoint();
    if (!started) {
        Serial.println("[BOOT] AP start failed");
    }

    setupRoutes();
    server.begin();

    lastHttpRequestMs = millis();
    lastLedFrameMs = millis();

    Serial.println("[BOOT] Vertical Garden backend started");
    Serial.print("[BOOT] AP SSID: ");
    Serial.println(apSsid);
    Serial.print("[BOOT] AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void loop() {
    updateClock();
    sampleMoistureIfNeeded();
    updatePumpRuntime();
    updateAutoPump();
    renderLedFrame(false);

    if (dnsStarted) {
        dnsServer.processNextRequest();
    }

    maintainAccessPointHealth();
    yield();
}
