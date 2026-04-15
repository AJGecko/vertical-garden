#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <ctype.h>
#include <string.h>

struct RuntimeState {
  String timezone = "Europe/Berlin";
  String mode = "now";
  String localTime = "1970-01-01T00:00:00";
  bool liveRateEnabled = false;
  int liveIntervalMs = 500;
  int moistureRaw = 0;
  int moisturePercent = 0;
  int pumpDurationMs = 5000;
  bool pumpOn = false;
  unsigned long pumpStopAtMs = 0;
  int ledR = 0;
  int ledG = 80;
  int ledB = 0;
  int ledBrightness = 64;
  bool actionAcknowledged = false;
  unsigned long actionAckAtMs = 0;
  unsigned long lastUpdateMs = 0;
};

ESP8266WebServer server(80);
RuntimeState state;

const char* AP_PASSWORD = "vertical123";
const byte DNS_PORT = 53;

// Pump output as used in your working hardware setup.
const uint8_t PIN_PUMP = D8;
const uint8_t PIN_BTN_MORE = D5;
const uint8_t PIN_BTN_LESS = D6;
const uint8_t PIN_BTN_ENTER = D7;
const uint8_t PIN_LED_STRIP = D1;
const uint8_t PIN_STATUS_LED = LED_BUILTIN;
const uint16_t LED_COUNT = 18;
const int BUTTON_DEBOUNCE_MS = 40;
const int PUMP_DURATION_STEP_MS = 1000;
const int PUMP_DURATION_MIN_MS = 1000;
const int PUMP_DURATION_MAX_MS = 600000;
const bool BUTTON_TEST_MODE_ENABLED = true;

String apSsid;
bool apStarted = false;
bool dnsStarted = false;

Adafruit_NeoPixel ledStrip(LED_COUNT, PIN_LED_STRIP, NEO_GRB + NEO_KHZ800);

struct ButtonState {
  uint8_t pin;
  bool stableState;
  bool lastRead;
  unsigned long lastChangeMs;
};

ButtonState btnMore = {PIN_BTN_MORE, HIGH, HIGH, 0};
ButtonState btnLess = {PIN_BTN_LESS, HIGH, HIGH, 0};
ButtonState btnEnter = {PIN_BTN_ENTER, HIGH, HIGH, 0};

unsigned long lastLogRootMs = 0;
unsigned long lastLogStatusMs = 0;
unsigned long lastLogRedirectMs = 0;
unsigned long lastLogNotFoundMs = 0;
unsigned long lastApRetryMs = 0;
int ledTestMode = 0;

IPAddress apIp(192, 168, 4, 1);
IPAddress apGateway(192, 168, 4, 1);
IPAddress apSubnet(255, 255, 255, 0);
DNSServer dnsServer;

const char STATUS_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Vertical Garden Access Point</title>
  <style>
    :root {
      --bg: #f2f5f7;
      --card: #ffffff;
      --ink: #1f2a35;
      --muted: #617385;
      --line: #d8e0e7;
      --brand: #167d52;
      --brand-ink: #ffffff;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: "Segoe UI", Tahoma, sans-serif;
      background: linear-gradient(180deg, #eef4f6, #e5edf0);
      color: var(--ink);
    }
    .wrap {
      max-width: 720px;
      margin: 24px auto;
      padding: 16px;
    }
    .card {
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 14px;
      padding: 18px;
      box-shadow: 0 8px 24px rgba(0,0,0,0.06);
    }
    h1 {
      margin: 0 0 8px;
      font-size: 1.4rem;
    }
    p { color: var(--muted); margin-top: 0; }
    .info {
      margin: 14px 0 0;
      padding: 14px;
      border-radius: 12px;
      background: #f7fafc;
      border: 1px solid var(--line);
    }
    .info strong { color: var(--ink); }
    button {
      display: inline-block;
      margin-top: 16px;
      padding: 12px 18px;
      border: none;
      border-radius: 10px;
      background: var(--brand);
      color: var(--brand-ink);
      font-size: 1rem;
      font-weight: 700;
      cursor: pointer;
    }
    .ok-status {
      margin-top: 14px;
      padding: 10px 12px;
      border-radius: 10px;
      background: #e6f4ea;
      border: 1px solid #b7dfc3;
      color: #166539;
      font-weight: 700;
    }
    .explain {
      margin-top: 14px;
      padding: 12px;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: #fbfdff;
      font-size: 0.95rem;
      line-height: 1.45;
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>Vertical Garden</h1>
      <div class="info">
        <p><strong>IP:</strong> <span id="ip">192.168.4.1</span></p>
        <p><strong>Port:</strong> <span id="port">80</span></p>
      </div>
      <div class="explain">
        <p><strong>DE:</strong> Diese Seite liefert die Backend-Daten fuer das Panel in app.html.</p>
        <p><strong>EN:</strong> This page provides backend data for the panel in app.html.</p>
      </div>
      <button id="okButton" type="button">OK</button>
      <div id="okStatus" class="ok-status" hidden>OK</div>
    </div>
  </div>
  <script>
    async function refreshInfo() {
      try {
        const response = await fetch('/api/status', { cache: 'no-store' });
        if (!response.ok) return;
        const data = await response.json();
        const ip = data.apIp || '192.168.4.1';
        document.getElementById('ip').textContent = ip;
      } catch (e) {
        console.warn('Status update failed', e);
      }
    }

    async function showOk() {
      const button = document.getElementById('okButton');
      const status = document.getElementById('okStatus');

      try {
        await fetch('/api/action/ack', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ ok: true })
        });
        button.disabled = true;
        button.textContent = 'Done';
        status.hidden = false;
        status.textContent = 'Action completed';
      } catch (e) {
        status.hidden = false;
        status.textContent = 'Ack failed';
      }

      refreshInfo();
    }

    document.getElementById('okButton').addEventListener('click', showOk);
    refreshInfo();
  </script>
</body>
</html>
)HTML";

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

String extractJsonString(const String& body, const String& key, const String& fallback) {
  String marker = "\"" + key + "\"";
  int keyPos = body.indexOf(marker);
  if (keyPos < 0) return fallback;

  int colonPos = body.indexOf(':', keyPos + marker.length());
  if (colonPos < 0) return fallback;

  int firstQuote = body.indexOf('"', colonPos + 1);
  if (firstQuote < 0) return fallback;

  int secondQuote = firstQuote + 1;
  while (true) {
    secondQuote = body.indexOf('"', secondQuote);
    if (secondQuote < 0) return fallback;
    if (body[secondQuote - 1] != '\\') break;
    secondQuote++;
  }

  return body.substring(firstQuote + 1, secondQuote);
}

bool extractJsonBool(const String& body, const String& key, bool fallback) {
  String marker = "\"" + key + "\"";
  int keyPos = body.indexOf(marker);
  if (keyPos < 0) return fallback;

  int colonPos = body.indexOf(':', keyPos + marker.length());
  if (colonPos < 0) return fallback;

  String tail = body.substring(colonPos + 1);
  tail.trim();

  if (tail.startsWith("true")) return true;
  if (tail.startsWith("false")) return false;
  return fallback;
}

int extractJsonInt(const String& body, const String& key, int fallback) {
  String marker = "\"" + key + "\"";
  int keyPos = body.indexOf(marker);
  if (keyPos < 0) return fallback;

  int colonPos = body.indexOf(':', keyPos + marker.length());
  if (colonPos < 0) return fallback;

  int start = colonPos + 1;
  while (start < (int)body.length() && isspace(body[start])) start++;

  int end = start;
  while (end < (int)body.length() && (isdigit(body[end]) || body[end] == '-')) end++;

  if (end <= start) return fallback;
  return body.substring(start, end).toInt();
}

void logThrottled(const String& message, unsigned long& lastMs, unsigned long intervalMs) {
  unsigned long now = millis();
  if (now - lastMs >= intervalMs) {
    Serial.println(message);
    lastMs = now;
  }
}

int clampInt(int value, int minValue, int maxValue) {
  if (value < minValue) return minValue;
  if (value > maxValue) return maxValue;
  return value;
}

void applyLedColor() {
  int brightness = clampInt(state.ledBrightness, 0, 255);
  int r = clampInt(state.ledR, 0, 255);
  int g = clampInt(state.ledG, 0, 255);
  int b = clampInt(state.ledB, 0, 255);

  ledStrip.setBrightness(brightness);
  uint32_t color = ledStrip.Color(r, g, b);
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    ledStrip.setPixelColor(i, color);
  }
  ledStrip.show();
}

void applyLedTestMode() {
  if (ledTestMode == 0) {
    state.ledR = 255;
    state.ledG = 255;
    state.ledB = 255;
    state.ledBrightness = 140;
    applyLedColor();
    Serial.println("[LED] Mode 1: WHITE");
    return;
  }

  if (ledTestMode == 1) {
    state.ledBrightness = 140;
    ledStrip.setBrightness(state.ledBrightness);
    for (uint16_t i = 0; i < LED_COUNT; i++) {
      if (i % 3 == 0) {
        ledStrip.setPixelColor(i, ledStrip.Color(255, 0, 0));
      } else if (i % 3 == 1) {
        ledStrip.setPixelColor(i, ledStrip.Color(0, 255, 0));
      } else {
        ledStrip.setPixelColor(i, ledStrip.Color(0, 0, 255));
      }
    }
    ledStrip.show();
    Serial.println("[LED] Mode 2: RGB");
    return;
  }

  state.ledR = 0;
  state.ledG = 0;
  state.ledB = 0;
  state.ledBrightness = 0;
  applyLedColor();
  Serial.println("[LED] Mode 3: OFF");
}

void setPumpOutput(bool on) {
  digitalWrite(PIN_PUMP, on ? HIGH : LOW);
  state.pumpOn = on;
  if (!on) {
    state.pumpStopAtMs = 0;
  }
}

void startPumpFor(int durationMs) {
  durationMs = clampInt(durationMs, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
  state.pumpDurationMs = durationMs;
  state.pumpStopAtMs = millis() + (unsigned long)durationMs;
  setPumpOutput(true);
}

void stopPump() {
  setPumpOutput(false);
}

void updatePumpTimeout() {
  if (!state.pumpOn || state.pumpStopAtMs == 0) return;
  unsigned long now = millis();
  if ((long)(now - state.pumpStopAtMs) >= 0) {
    stopPump();
  }
}

void updateMoisture() {
  int raw = analogRead(A0);
  state.moistureRaw = raw;
  state.moisturePercent = clampInt(100 - ((raw * 100) / 1023), 0, 100);
}

bool buttonPressed(ButtonState& button) {
  bool currentRead = digitalRead(button.pin);
  unsigned long now = millis();

  if (currentRead != button.lastRead) {
    button.lastChangeMs = now;
    button.lastRead = currentRead;
  }

  if (now - button.lastChangeMs < (unsigned long)BUTTON_DEBOUNCE_MS) {
    return false;
  }

  if (button.stableState != currentRead) {
    button.stableState = currentRead;
    if (button.stableState == LOW) {
      return true;
    }
  }

  return false;
}

void handleButtonActions() {
  if (!BUTTON_TEST_MODE_ENABLED) {
    if (buttonPressed(btnMore)) {
      state.pumpDurationMs = clampInt(state.pumpDurationMs + PUMP_DURATION_STEP_MS, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
      Serial.print("[BUTTON] MORE pressed, pumpDurationMs=");
      Serial.println(state.pumpDurationMs);
      state.lastUpdateMs = millis();
    }

    if (buttonPressed(btnLess)) {
      state.pumpDurationMs = clampInt(state.pumpDurationMs - PUMP_DURATION_STEP_MS, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);
      Serial.print("[BUTTON] LESS pressed, pumpDurationMs=");
      Serial.println(state.pumpDurationMs);
      state.lastUpdateMs = millis();
    }

    if (buttonPressed(btnEnter)) {
      startPumpFor(state.pumpDurationMs);
      Serial.println("[BUTTON] ENTER pressed, pump timer started");
      state.lastUpdateMs = millis();
    }
    return;
  }

  if (buttonPressed(btnMore)) {
    if (state.pumpOn) {
      stopPump();
      Serial.println("[BUTTON] MORE pressed, pump toggled OFF");
    } else {
      state.pumpStopAtMs = 0;
      setPumpOutput(true);
      Serial.println("[BUTTON] MORE pressed, pump toggled ON");
    }
    state.lastUpdateMs = millis();
  }

  if (buttonPressed(btnLess)) {
    ledTestMode = (ledTestMode + 1) % 3;
    Serial.print("[BUTTON] LESS pressed, LED test mode=");
    Serial.println(ledTestMode + 1);
    applyLedTestMode();
    state.lastUpdateMs = millis();
  }

  if (buttonPressed(btnEnter)) {
    updateMoisture();
    Serial.print("[BUTTON] ENTER pressed, moistureRaw=");
    Serial.print(state.moistureRaw);
    Serial.print(", moisturePercent=");
    Serial.println(state.moisturePercent);
    state.lastUpdateMs = millis();
  }
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

void sendJson(int statusCode, const String& body) {
  addCorsHeaders();
  server.send(statusCode, "application/json", body);
}

String buildStatusJson() {
  String apIpText = apStarted ? WiFi.softAPIP().toString() : "0.0.0.0";
  unsigned long remainingMs = 0;
  if (state.pumpOn && state.pumpStopAtMs > 0) {
    unsigned long now = millis();
    if ((long)(state.pumpStopAtMs - now) > 0) {
      remainingMs = state.pumpStopAtMs - now;
    }
  }

  String json = "{";
  json += "\"timezone\":\"" + jsonEscape(state.timezone) + "\",";
  json += "\"mode\":\"" + jsonEscape(state.mode) + "\",";
  json += "\"localTime\":\"" + jsonEscape(state.localTime) + "\",";
  json += "\"liveRateEnabled\":" + String(state.liveRateEnabled ? "true" : "false") + ",";
  json += "\"liveIntervalMs\":" + String(state.liveIntervalMs) + ",";
  json += "\"uptimeMs\":" + String(millis()) + ",";
  json += "\"updatedAgoMs\":" + String(millis() - state.lastUpdateMs) + ",";
  json += "\"wifiConnected\":false,";
  json += "\"apStarted\":" + String(apStarted ? "true" : "false") + ",";
  json += "\"ip\":\"0.0.0.0\",";
  json += "\"apIp\":\"" + jsonEscape(apIpText) + "\",";
  json += "\"apSsid\":\"" + jsonEscape(apSsid) + "\",";
  json += "\"moistureRaw\":" + String(state.moistureRaw) + ",";
  json += "\"moisturePercent\":" + String(state.moisturePercent) + ",";
  json += "\"pumpOn\":" + String(state.pumpOn ? "true" : "false") + ",";
  json += "\"pumpDurationMs\":" + String(state.pumpDurationMs) + ",";
  json += "\"pumpRemainingMs\":" + String(remainingMs) + ",";
  json += "\"led\":{";
  json += "\"r\":" + String(state.ledR) + ",";
  json += "\"g\":" + String(state.ledG) + ",";
  json += "\"b\":" + String(state.ledB) + ",";
  json += "\"brightness\":" + String(state.ledBrightness);
  json += "},";
  json += "\"buttons\":{";
  json += "\"more\":" + String(btnMore.stableState == LOW ? "true" : "false") + ",";
  json += "\"less\":" + String(btnLess.stableState == LOW ? "true" : "false") + ",";
  json += "\"enter\":" + String(btnEnter.stableState == LOW ? "true" : "false");
  json += "},";
  json += "\"actionAcknowledged\":" + String(state.actionAcknowledged ? "true" : "false") + ",";
  json += "\"actionAckAtMs\":" + String(state.actionAckAtMs);
  json += "}";
  return json;
}

void startAccessPoint() {
  if (apStarted) {
    Serial.println("[Wi-Fi] Access point is already running");
    return;
  }

  Serial.println("[Wi-Fi] Starting access point fallback");
  apSsid = String("VerticalGarden-") + String(ESP.getChipId(), HEX);
  apSsid.toUpperCase();

  // WICHTIG: Komplett sauberen Start erzwingen, keine gespeicherten Netze suchen
  WiFi.disconnect(true);
  delay(100);
  
  // Nur AP-Modus, KEIN STA-Modus! Wenn WIFI_AP_STA aktiv ist und er kein Netz findet,
  // macht der ESP8266 Channel-Hopping und der AP verschwindet!
  WiFi.mode(WIFI_AP);
  delay(100);

  WiFi.softAPConfig(apIp, apGateway, apSubnet);
  apStarted = WiFi.softAP(apSsid.c_str(), AP_PASSWORD);

  if (apStarted) {
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    dnsStarted = true;
    Serial.print("Access Point started: ");
    Serial.println(apSsid);
    Serial.print("AP password: ");
    Serial.println(AP_PASSWORD);
    Serial.print("[DNS] Captive portal DNS started on port ");
    Serial.println(DNS_PORT);
    Serial.print("Connect here: http://");
    Serial.print(WiFi.softAPIP());
    Serial.println("/");
    Serial.print("[Wi-Fi] AP SSID: ");
    Serial.println(WiFi.softAPSSID());
  } else {
    Serial.println("Access Point could not be started.");
  }
}

void handleOptions() {
  addCorsHeaders();
  server.send(204, "text/plain", "");
}

void handleRoot() {
  logThrottled("[HTTP] GET /", lastLogRootMs, 5000);
  addCorsHeaders();
  server.send_P(200, "text/html; charset=utf-8", STATUS_PAGE);
}

void handleGetStatus() {
  logThrottled("[HTTP] GET /api/status", lastLogStatusMs, 5000);
  sendJson(200, buildStatusJson());
}

void handleGetSensors() {
  updateMoisture();
  String json = "{";
  json += "\"moistureRaw\":" + String(state.moistureRaw) + ",";
  json += "\"moisturePercent\":" + String(state.moisturePercent) + ",";
  json += "\"buttons\":{";
  json += "\"more\":" + String(btnMore.stableState == LOW ? "true" : "false") + ",";
  json += "\"less\":" + String(btnLess.stableState == LOW ? "true" : "false") + ",";
  json += "\"enter\":" + String(btnEnter.stableState == LOW ? "true" : "false");
  json += "}";
  json += "}";
  sendJson(200, json);
}

void handleGetPump() {
  unsigned long remainingMs = 0;
  if (state.pumpOn && state.pumpStopAtMs > 0) {
    unsigned long now = millis();
    if ((long)(state.pumpStopAtMs - now) > 0) {
      remainingMs = state.pumpStopAtMs - now;
    }
  }

  String json = "{";
  json += "\"on\":" + String(state.pumpOn ? "true" : "false") + ",";
  json += "\"durationMs\":" + String(state.pumpDurationMs) + ",";
  json += "\"remainingMs\":" + String(remainingMs);
  json += "}";
  sendJson(200, json);
}

void handlePostPump() {
  String body = server.arg("plain");
  bool on = extractJsonBool(body, "on", state.pumpOn);
  int durationMs = extractJsonInt(body, "durationMs", state.pumpDurationMs);
  durationMs = clampInt(durationMs, PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);

  if (on) {
    startPumpFor(durationMs);
  } else {
    stopPump();
  }

  state.lastUpdateMs = millis();
  sendJson(200, "{\"ok\":true}");
}

void handlePostLed() {
  String body = server.arg("plain");
  state.ledR = clampInt(extractJsonInt(body, "r", state.ledR), 0, 255);
  state.ledG = clampInt(extractJsonInt(body, "g", state.ledG), 0, 255);
  state.ledB = clampInt(extractJsonInt(body, "b", state.ledB), 0, 255);
  state.ledBrightness = clampInt(extractJsonInt(body, "brightness", state.ledBrightness), 0, 255);
  applyLedColor();

  state.lastUpdateMs = millis();
  sendJson(200, "{\"ok\":true}");
}

void handlePostSettings() {
  String body = server.arg("plain");

  state.timezone = extractJsonString(body, "timezone", state.timezone);
  state.mode = extractJsonString(body, "mode", state.mode);
  state.localTime = extractJsonString(body, "localTime", state.localTime);
  state.liveRateEnabled = extractJsonBool(body, "liveRateEnabled", state.liveRateEnabled);
  state.liveIntervalMs = clampInt(extractJsonInt(body, "liveIntervalMs", state.liveIntervalMs), 250, 60000);
  state.pumpDurationMs = clampInt(extractJsonInt(body, "pumpDurationMs", state.pumpDurationMs), PUMP_DURATION_MIN_MS, PUMP_DURATION_MAX_MS);

  state.lastUpdateMs = millis();
  sendJson(200, "{\"ok\":true}");
}

void handlePostActionAck() {
  String body = server.arg("plain");
  bool ok = extractJsonBool(body, "ok", false);
  if (ok) {
    state.actionAcknowledged = true;
    state.actionAckAtMs = millis();
    state.lastUpdateMs = millis();
  }
  sendJson(200, "{\"ok\":true}");
}

void handleCaptivePortalRedirect() {
  logThrottled("[HTTP] Captive portal redirect", lastLogRedirectMs, 7000);
  addCorsHeaders();
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  server.send(302, "text/plain", "Redirecting to access portal");
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }

  logThrottled(String("[HTTP] 404 Not Found: ") + server.uri(), lastLogNotFoundMs, 8000);
  sendJson(404, "{\"error\":\"not found\"}");
}

void setupApiRoutes() {
  Serial.println("[HTTP] Registering routes");
  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/sensors", HTTP_OPTIONS, handleOptions);
  server.on("/api/pump", HTTP_OPTIONS, handleOptions);
  server.on("/api/led", HTTP_OPTIONS, handleOptions);
  server.on("/api/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/action/ack", HTTP_OPTIONS, handleOptions);

  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/api/sensors", HTTP_GET, handleGetSensors);
  server.on("/api/pump", HTTP_GET, handleGetPump);
  server.on("/api/pump", HTTP_POST, handlePostPump);
  server.on("/api/led", HTTP_POST, handlePostLed);
  server.on("/api/settings", HTTP_POST, handlePostSettings);
  server.on("/api/action/ack", HTTP_POST, handlePostActionAck);

  server.on("/generate_204", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/gen_204", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/connecttest.txt", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/ncsi.txt", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/redirect", HTTP_GET, handleCaptivePortalRedirect);
  server.on("/fwlink", HTTP_GET, handleCaptivePortalRedirect);

  server.onNotFound(handleNotFound);
}

void setup() {
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, HIGH);

  Serial.begin(115200);
  delay(120);

  Serial.println();
  Serial.println("[BOOT] Vertical Garden backend starting");
  Serial.println("[BOOT] Starting access point now");

  // Start AP as early as possible (old stable behavior).
  startAccessPoint();

  setupApiRoutes();
  server.begin();
  Serial.println("HTTP server started on port 80");

  pinMode(PIN_PUMP, OUTPUT);
  stopPump();

  pinMode(PIN_BTN_MORE, INPUT_PULLUP);
  pinMode(PIN_BTN_LESS, INPUT_PULLUP);
  pinMode(PIN_BTN_ENTER, INPUT_PULLUP);

  ledStrip.begin();
  applyLedColor();

  state.lastUpdateMs = millis();
  lastApRetryMs = millis();
}

void loop() {
  if (!apStarted && (millis() - lastApRetryMs) >= 10000UL) {
    lastApRetryMs = millis();
    Serial.println("[Wi-Fi] AP missing, retrying startup");
    startAccessPoint();
  }

  if (dnsStarted) {
    dnsServer.processNextRequest();
  }
  updateMoisture();
  handleButtonActions();
  updatePumpTimeout();
  server.handleClient();
}
