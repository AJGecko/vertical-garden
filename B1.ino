#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

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

struct BackendState {
  String timezone = "Europe/Berlin";
  String mode = "now";
  String localTime = "1970-01-01T00:00:00";
  bool liveRateEnabled = false;
  int liveIntervalMs = 500;
  unsigned long lastUpdateMs = 0;
};

BackendState state;

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

String buildTimeJson() {
  String j = "{";
  j += "\"timezone\":\"" + jsonEscape(state.timezone) + "\",";
  j += "\"mode\":\"" + jsonEscape(state.mode) + "\",";
  j += "\"localTime\":\"" + jsonEscape(state.localTime) + "\"";
  j += "}";
  return j;
}

String buildStatusJson() {
  String j = "{";
  j += "\"timezone\":\"" + jsonEscape(state.timezone) + "\",";
  j += "\"mode\":\"" + jsonEscape(state.mode) + "\",";
  j += "\"localTime\":\"" + jsonEscape(state.localTime) + "\",";
  j += "\"liveRateEnabled\":" + String(state.liveRateEnabled ? "true" : "false") + ",";
  j += "\"liveIntervalMs\":" + String(state.liveIntervalMs) + ",";
  j += "\"uptimeMs\":" + String(millis()) + ",";
  j += "\"updatedAgoMs\":" + String(millis() - state.lastUpdateMs) + ",";
  j += "\"apStarted\":" + String(apStarted ? "true" : "false") + ",";
  j += "\"apSsid\":\"" + jsonEscape(apSsid) + "\",";
  j += "\"apIp\":\"" + (apStarted ? WiFi.softAPIP().toString() : String("0.0.0.0")) + "\"";
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

  server.on("/api/time", HTTP_GET, handleGetTime);
  server.on("/api/time", HTTP_POST, handlePostTime);

  server.on("/api/settings", HTTP_POST, handlePostSettings);

  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/status", HTTP_GET, handleGetStatus);

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

  startAccessPoint();

  setupRoutes();
  server.begin();
  Serial.println("[BOOT] HTTP server started on port " + String(HTTP_PORT));
}

void loop() {
  if (dnsStarted) {
    dnsServer.processNextRequest();
  }
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
