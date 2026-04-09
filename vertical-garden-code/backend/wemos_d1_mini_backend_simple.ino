#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
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

const char* DEFAULT_WIFI_SSID = "PROBIERWERK#OPEN";
const char* DEFAULT_WIFI_PASSWORD = "PROBIERWERK#2019";
const char* AP_PASSWORD = "vertical123";
const byte DNS_PORT = 53;

String apSsid;
bool apStarted = false;
bool dnsStarted = false;

IPAddress apIp(192, 168, 4, 1);
IPAddress apGateway(192, 168, 4, 1);
IPAddress apSubnet(255, 255, 255, 0);
DNSServer dnsServer;

const char WIFI_SETUP_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Vertical Garden Wi-Fi Setup</title>
  <style>
    :root {
      --bg: #f2f5f7;
      --card: #ffffff;
      --ink: #1f2a35;
      --muted: #617385;
      --line: #d8e0e7;
      --brand: #167d52;
      --brand-ink: #ffffff;
      --warn: #9a3412;
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
    label {
      display: block;
      margin-top: 12px;
      margin-bottom: 6px;
      font-weight: 600;
    }
    input, select, button {
      width: 100%;
      border-radius: 10px;
      border: 1px solid var(--line);
      padding: 11px 12px;
      font-size: 1rem;
      background: #fff;
    }
    button {
      cursor: pointer;
      border: none;
      background: var(--brand);
      color: var(--brand-ink);
      font-weight: 700;
    }
    .row {
      display: grid;
      gap: 10px;
      grid-template-columns: 1fr 1fr;
      margin-top: 12px;
    }
    .row button.secondary {
      background: #2a4a63;
    }
    .status {
      margin-top: 14px;
      padding: 10px 12px;
      border-radius: 10px;
      background: #f7fafc;
      border: 1px solid var(--line);
      white-space: pre-wrap;
      font-family: "Consolas", "Courier New", monospace;
      font-size: 0.9rem;
    }
    .warning {
      color: var(--warn);
      font-weight: 600;
      margin-top: 10px;
    }
    @media (max-width: 640px) {
      .row { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>Vertical Garden Wi-Fi Setup</h1>
      <p>Select a network, enter password, and save. The controller will reconnect immediately.</p>

      <label for="networkList">Nearby Wi-Fi Networks</label>
      <select id="networkList"></select>

      <label for="ssidInput">SSID</label>
      <input id="ssidInput" placeholder="Wi-Fi name">

      <label for="passwordInput">Password</label>
      <input id="passwordInput" type="password" placeholder="Wi-Fi password">

      <div class="row">
        <button id="saveButton" type="button">Save and Connect</button>
        <button id="scanButton" type="button" class="secondary">Refresh Network List</button>
      </div>

      <div class="warning">Note: ESP8266 supports 2.4 GHz Wi-Fi only.</div>
      <div id="status" class="status">Loading...</div>
    </div>
  </div>

  <script>
    const networkList = document.getElementById("networkList");
    const ssidInput = document.getElementById("ssidInput");
    const passwordInput = document.getElementById("passwordInput");
    const statusBox = document.getElementById("status");

    function setStatus(message) {
      statusBox.textContent = message;
    }

    async function getJson(path) {
      const response = await fetch(path, { cache: "no-store" });
      if (!response.ok) {
        throw new Error("Request failed: " + response.status + " " + path);
      }
      return response.json();
    }

    async function postJson(path, payload) {
      const response = await fetch(path, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
      const text = await response.text();
      if (!response.ok) {
        throw new Error(text || ("Request failed: " + response.status));
      }
      return text;
    }

    async function refreshNetworks() {
      setStatus("Scanning for Wi-Fi networks...");
      try {
        const data = await getJson("/api/wifi/scan");
        const networks = Array.isArray(data.networks) ? data.networks : [];

        networkList.innerHTML = "";
        if (networks.length === 0) {
          const option = document.createElement("option");
          option.value = "";
          option.textContent = "No networks found";
          networkList.appendChild(option);
        }

        networks.forEach(net => {
          const option = document.createElement("option");
          option.value = net.ssid || "";
          const lock = net.secure ? "locked" : "open";
          option.textContent = (net.ssid || "(hidden)") + "  [" + lock + ", RSSI " + net.rssi + "]";
          networkList.appendChild(option);
        });

        if (networkList.value) {
          ssidInput.value = networkList.value;
        }
        setStatus("Network scan done. Pick one and save.");
      } catch (error) {
        setStatus("Scan failed: " + error.message);
      }
    }

    async function refreshCurrentStatus() {
      try {
        const wifi = await getJson("/api/wifi");
        if (wifi && wifi.ssid) {
          ssidInput.value = wifi.ssid;
        }

        const info = [
          "Current SSID: " + (wifi.ssid || "-"),
          "Connected: " + (wifi.wifiConnected ? "yes" : "no"),
          "Station IP: " + (wifi.ip || "0.0.0.0"),
          "AP active: " + (wifi.apStarted ? "yes" : "no"),
          "AP SSID: " + (wifi.apSsid || "-"),
          "AP IP: " + (wifi.apIp || "0.0.0.0")
        ];
        setStatus(info.join("\n"));
      } catch (error) {
        setStatus("Could not load current status: " + error.message);
      }
    }

    networkList.addEventListener("change", () => {
      ssidInput.value = networkList.value || "";
    });

    document.getElementById("scanButton").addEventListener("click", refreshNetworks);
    document.getElementById("saveButton").addEventListener("click", async () => {
      const ssid = ssidInput.value.trim();
      if (!ssid) {
        setStatus("Please enter an SSID.");
        return;
      }

      try {
        setStatus("Saving credentials and reconnecting...");
        await postJson("/api/wifi", {
          ssid,
          password: passwordInput.value
        });
        setStatus("Saved. The controller is reconnecting now.\nReload this page in a few seconds.");
      } catch (error) {
        setStatus("Save failed: " + error.message);
      }
    });

    refreshCurrentStatus();
    refreshNetworks();
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
  Serial.println("[EEPROM] Saving configuration");
  config.magic = CFG_MAGIC;
  config.version = CFG_VERSION;
  EEPROM.put(0, config);
  EEPROM.commit();
}

void loadConfigFromEeprom() {
  Serial.println("[EEPROM] Loading configuration");
  EEPROM.get(0, config);
  if (config.magic != CFG_MAGIC || config.version != CFG_VERSION) {
    Serial.println("[EEPROM] Invalid or empty data, writing defaults");
    memset(&config, 0, sizeof(config));
    config.magic = CFG_MAGIC;
    config.version = CFG_VERSION;
    writeStringToFixed(config.ssid, sizeof(config.ssid), DEFAULT_WIFI_SSID);
    writeStringToFixed(config.password, sizeof(config.password), DEFAULT_WIFI_PASSWORD);
    saveConfigToEeprom();
  } else {
    Serial.println("[EEPROM] Configuration loaded successfully");
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

String getStationUrl() {
  return String("http://") + WiFi.localIP().toString() + "/";
}

String getAccessPointUrl() {
  return String("http://") + WiFi.softAPIP().toString() + "/";
}

void printAvailableNetworks() {
  Serial.println("[Wi-Fi] Scanning for nearby networks");
  int networkCount = WiFi.scanNetworks();

  if (networkCount <= 0) {
    Serial.println("[Wi-Fi] No networks found");
    return;
  }

  Serial.print("[Wi-Fi] Found networks: ");
  Serial.println(networkCount);

  for (int index = 0; index < networkCount; index++) {
    Serial.print("[Wi-Fi] ");
    Serial.print(index + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(index));
    Serial.print(" (RSSI: ");
    Serial.print(WiFi.RSSI(index));
    Serial.print(", secure: ");
    Serial.print(WiFi.encryptionType(index) != ENC_TYPE_NONE ? "yes" : "no");
    Serial.println(")");
  }

  WiFi.scanDelete();
}

String buildWifiScanJson() {
  int networkCount = WiFi.scanNetworks();
  String json = "{\"ok\":true,\"networks\":[";
  bool first = true;

  for (int index = 0; index < networkCount; index++) {
    String scannedSsid = WiFi.SSID(index);
    if (scannedSsid.length() == 0) {
      continue;
    }

    if (!first) {
      json += ",";
    }
    first = false;

    json += "{";
    json += "\"ssid\":\"" + jsonEscape(scannedSsid) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(index)) + ",";
    json += "\"secure\":" + String(WiFi.encryptionType(index) != ENC_TYPE_NONE ? "true" : "false");
    json += "}";
  }

  json += "]}";
  WiFi.scanDelete();
  return json;
}

void startAccessPointFallback() {
  if (apStarted) {
    Serial.println("[Wi-Fi] Access point is already running");
    return;
  }

  Serial.println("[Wi-Fi] Starting access point fallback");
  apSsid = String("VerticalGarden-") + String(ESP.getChipId(), HEX);
  apSsid.toUpperCase();

  WiFi.mode(WIFI_AP_STA);
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
  } else {
    Serial.println("Access Point could not be started.");
  }
}

void connectWifiWithCurrentConfig() {
  String ssid = fixedToString(config.ssid, sizeof(config.ssid));
  String pass = fixedToString(config.password, sizeof(config.password));

  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.disconnect();
  delay(100);

  printAvailableNetworks();

  Serial.print("[Wi-Fi] Connecting to SSID: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_AP_STA);
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
    Serial.print("Connect here: ");
    Serial.println(getStationUrl());
  } else {
    Serial.println("Wi-Fi not connected (still running API for local update).");
    startAccessPointFallback();
  }
}

String buildStatusJson() {
  String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0";
  String apIp = apStarted ? WiFi.softAPIP().toString() : "0.0.0.0";
  String json = "{";
  json += "\"timezone\":\"" + jsonEscape(state.timezone) + "\",";
  json += "\"mode\":\"" + jsonEscape(state.mode) + "\",";
  json += "\"localTime\":\"" + jsonEscape(state.localTime) + "\",";
  json += "\"liveRateEnabled\":" + String(state.liveRateEnabled ? "true" : "false") + ",";
  json += "\"liveIntervalMs\":" + String(state.liveIntervalMs) + ",";
  json += "\"uptimeMs\":" + String(millis()) + ",";
  json += "\"updatedAgoMs\":" + String(millis() - state.lastUpdateMs) + ",";
  json += "\"wifiConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"apStarted\":" + String(apStarted ? "true" : "false") + ",";
  json += "\"ip\":\"" + jsonEscape(ip) + "\",";
  json += "\"apIp\":\"" + jsonEscape(apIp) + "\",";
  json += "\"apSsid\":\"" + jsonEscape(apSsid) + "\"";
  json += "}";
  return json;
}

void updateWifiCredentials(const String& ssid, const String& password) {
  Serial.println("[Wi-Fi] Updating stored credentials");
  writeStringToFixed(config.ssid, sizeof(config.ssid), ssid);
  writeStringToFixed(config.password, sizeof(config.password), password);
  saveConfigToEeprom();
}

void handleOptions() {
  Serial.println("[HTTP] OPTIONS request");
  addCorsHeaders();
  server.send(204, "text/plain", "");
}

void handleRoot() {
  Serial.println("[HTTP] GET /");
  addCorsHeaders();
  server.send_P(200, "text/html; charset=utf-8", WIFI_SETUP_PAGE);
}

void handleGetStatus() {
  Serial.println("[HTTP] GET /api/status or /status");
  sendJson(200, buildStatusJson());
}

void handlePostSettings() {
  Serial.println("[HTTP] POST /api/settings");
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
  Serial.println("[HTTP] POST /api/time");
  String body = server.arg("plain");

  state.timezone = extractJsonString(body, "timezone", state.timezone);
  state.mode = extractJsonString(body, "mode", state.mode);
  state.localTime = extractJsonString(body, "localTime", state.localTime);
  state.lastUpdateMs = millis();

  sendJson(200, "{\"ok\":true}");
}

void handleGetWifiStatus() {
  Serial.println("[HTTP] GET /api/wifi");
  String ssid = fixedToString(config.ssid, sizeof(config.ssid));
  String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0";
  String apIp = apStarted ? WiFi.softAPIP().toString() : "0.0.0.0";
  String json = "{";
  json += "\"ssid\":\"" + jsonEscape(ssid) + "\",";
  json += "\"wifiConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"apStarted\":" + String(apStarted ? "true" : "false") + ",";
  json += "\"ip\":\"" + jsonEscape(ip) + "\",";
  json += "\"apIp\":\"" + jsonEscape(apIp) + "\",";
  json += "\"apSsid\":\"" + jsonEscape(apSsid) + "\"";
  json += "}";
  sendJson(200, json);
}

void handleGetWifiScan() {
  Serial.println("[HTTP] GET /api/wifi/scan");
  sendJson(200, buildWifiScanJson());
}

void handleCaptivePortalRedirect() {
  Serial.println("[HTTP] Captive portal redirect");
  addCorsHeaders();
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
  server.send(302, "text/plain", "Redirecting to setup portal");
}

void handlePostWifi() {
  Serial.println("[HTTP] POST /api/wifi");
  String body = server.arg("plain");
  String ssid = extractJsonString(body, "ssid", "");
  String password = extractJsonString(body, "password", "");

  if (ssid.length() == 0) {
    Serial.println("[HTTP] Wi-Fi update rejected: SSID missing");
    sendJson(400, "{\"ok\":false,\"error\":\"ssid required\"}");
    return;
  }

  updateWifiCredentials(ssid, password);
  connectWifiWithCurrentConfig();
  state.lastUpdateMs = millis();

  sendJson(200, "{\"ok\":true}");
}

void handleNotFound() {
  Serial.print("[HTTP] 404 Not Found: ");
  Serial.println(server.uri());
  if (server.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }

  if (apStarted && server.method() == HTTP_GET) {
    handleCaptivePortalRedirect();
    return;
  }

  sendJson(404, "{\"error\":\"not found\"}");
}

void setupApiRoutes() {
  Serial.println("[HTTP] Registering routes");
  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/time", HTTP_OPTIONS, handleOptions);
  server.on("/api/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/wifi", HTTP_OPTIONS, handleOptions);
  server.on("/api/wifi/scan", HTTP_OPTIONS, handleOptions);

  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/api/time", HTTP_GET, handleGetStatus);
  server.on("/api/time", HTTP_POST, handlePostTime);
  server.on("/api/settings", HTTP_POST, handlePostSettings);
  server.on("/api/wifi", HTTP_GET, handleGetWifiStatus);
  server.on("/api/wifi", HTTP_POST, handlePostWifi);
  server.on("/api/wifi/scan", HTTP_GET, handleGetWifiScan);

  // Captive portal probe endpoints for Android/iOS/Windows.
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
  Serial.begin(115200);
  delay(120);

  Serial.println();
  Serial.println("[BOOT] Vertical Garden backend starting");

  EEPROM.begin(EEPROM_BYTES);
  loadConfigFromEeprom();

  // Keep local setup page reachable at 192.168.4.1 while station connection is running.
  startAccessPointFallback();

  setupApiRoutes();
  server.begin();
  Serial.println("HTTP server started on port 80");

  connectWifiWithCurrentConfig();
  state.lastUpdateMs = millis();
}

void loop() {
  if (dnsStarted) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
}
