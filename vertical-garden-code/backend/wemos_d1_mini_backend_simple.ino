#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ctype.h>
#include <string.h>

const size_t EEPROM_BYTES = 256;
const uint8_t CFG_MAGIC = 0xA5;
const uint8_t CFG_VERSION = 1;
const size_t CFG_SSID_LEN = 32;
const size_t CFG_PASS_LEN = 64;

struct PersistentConfig {
  uint8_t magic;
  uint8_t version;
  char ssid[CFG_SSID_LEN];
  char password[CFG_PASS_LEN];
};

struct RuntimeState {
  String timezone = "Europe/Berlin";
  String mode = "now";
  String localTime = "1970-01-01T00:00:00";
  bool liveRateEnabled = false;
  int liveIntervalMs = 500;
  unsigned long lastUpdateMs = 0;
};

ESP8266WebServer server(80);
PersistentConfig config;
RuntimeState state;

const char* DEFAULT_WIFI_SSID = "YOUR_WIFI_SSID";
const char* DEFAULT_WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

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

void writeStringToFixed(char* target, size_t targetLen, const String& value) {
  if (targetLen == 0) return;
  memset(target, 0, targetLen);
  size_t maxCopy = targetLen - 1;
  size_t copyLen = value.length() < maxCopy ? value.length() : maxCopy;
  memcpy(target, value.c_str(), copyLen);
}

String fixedToString(const char* source, size_t maxLen) {
  size_t len = strnlen(source, maxLen);
  return String(source).substring(0, len);
}

void saveConfigToEeprom() {
  config.magic = CFG_MAGIC;
  config.version = CFG_VERSION;
  EEPROM.put(0, config);
  EEPROM.commit();
}

void loadConfigFromEeprom() {
  EEPROM.get(0, config);
  if (config.magic != CFG_MAGIC || config.version != CFG_VERSION) {
    memset(&config, 0, sizeof(config));
    config.magic = CFG_MAGIC;
    config.version = CFG_VERSION;
    writeStringToFixed(config.ssid, sizeof(config.ssid), DEFAULT_WIFI_SSID);
    writeStringToFixed(config.password, sizeof(config.password), DEFAULT_WIFI_PASSWORD);
    saveConfigToEeprom();
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
  // Needed by Chromium when HTTPS page accesses local network devices.
  server.sendHeader("Access-Control-Allow-Private-Network", "true");
}

void sendJson(int statusCode, const String& body) {
  addCorsHeaders();
  server.send(statusCode, "application/json", body);
}

void connectWifiWithCurrentConfig() {
  String ssid = fixedToString(config.ssid, sizeof(config.ssid));
  String pass = fixedToString(config.password, sizeof(config.password));

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(350);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi not connected (still running API for local update).");
  }
}

String buildStatusJson() {
  String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0";
  String json = "{";
  json += "\"timezone\":\"" + jsonEscape(state.timezone) + "\",";
  json += "\"mode\":\"" + jsonEscape(state.mode) + "\",";
  json += "\"localTime\":\"" + jsonEscape(state.localTime) + "\",";
  json += "\"liveRateEnabled\":" + String(state.liveRateEnabled ? "true" : "false") + ",";
  json += "\"liveIntervalMs\":" + String(state.liveIntervalMs) + ",";
  json += "\"uptimeMs\":" + String(millis()) + ",";
  json += "\"updatedAgoMs\":" + String(millis() - state.lastUpdateMs) + ",";
  json += "\"wifiConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ip\":\"" + jsonEscape(ip) + "\"";
  json += "}";
  return json;
}

void updateWifiCredentials(const String& ssid, const String& password) {
  writeStringToFixed(config.ssid, sizeof(config.ssid), ssid);
  writeStringToFixed(config.password, sizeof(config.password), password);
  saveConfigToEeprom();
}

void handleOptions() {
  addCorsHeaders();
  server.send(204, "text/plain", "");
}

void handleRoot() {
  addCorsHeaders();
  server.send(200, "text/plain", "Wemos D1 mini backend is running");
}

void handleGetStatus() {
  sendJson(200, buildStatusJson());
}

void handlePostSettings() {
  String body = server.arg("plain");

  state.timezone = extractJsonString(body, "timezone", state.timezone);
  state.liveRateEnabled = extractJsonBool(body, "liveRateEnabled", state.liveRateEnabled);
  state.liveIntervalMs = extractJsonInt(body, "liveIntervalMs", state.liveIntervalMs);
  if (state.liveIntervalMs < 250) state.liveIntervalMs = 250;
  if (state.liveIntervalMs > 60000) state.liveIntervalMs = 60000;
  state.lastUpdateMs = millis();

  sendJson(200, "{\"ok\":true}");
}

void handlePostTime() {
  String body = server.arg("plain");

  state.timezone = extractJsonString(body, "timezone", state.timezone);
  state.mode = extractJsonString(body, "mode", state.mode);
  state.localTime = extractJsonString(body, "localTime", state.localTime);
  state.lastUpdateMs = millis();

  sendJson(200, "{\"ok\":true}");
}

void handleGetWifiStatus() {
  String ssid = fixedToString(config.ssid, sizeof(config.ssid));
  String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0";
  String json = "{";
  json += "\"ssid\":\"" + jsonEscape(ssid) + "\",";
  json += "\"wifiConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"ip\":\"" + jsonEscape(ip) + "\"";
  json += "}";
  sendJson(200, json);
}

void handlePostWifi() {
  String body = server.arg("plain");
  String ssid = extractJsonString(body, "ssid", "");
  String password = extractJsonString(body, "password", "");

  if (ssid.length() == 0) {
    sendJson(400, "{\"ok\":false,\"error\":\"ssid required\"}");
    return;
  }

  updateWifiCredentials(ssid, password);
  connectWifiWithCurrentConfig();
  state.lastUpdateMs = millis();

  sendJson(200, "{\"ok\":true}");
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }
  sendJson(404, "{\"error\":\"not found\"}");
}

void setupApiRoutes() {
  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/time", HTTP_OPTIONS, handleOptions);
  server.on("/api/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/wifi", HTTP_OPTIONS, handleOptions);

  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/api/time", HTTP_GET, handleGetStatus);
  server.on("/api/time", HTTP_POST, handlePostTime);
  server.on("/api/settings", HTTP_POST, handlePostSettings);
  server.on("/api/wifi", HTTP_GET, handleGetWifiStatus);
  server.on("/api/wifi", HTTP_POST, handlePostWifi);

  server.onNotFound(handleNotFound);
}

void setup() {
  Serial.begin(115200);
  delay(120);

  EEPROM.begin(EEPROM_BYTES);
  loadConfigFromEeprom();

  connectWifiWithCurrentConfig();
  state.lastUpdateMs = millis();

  setupApiRoutes();
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();
}
