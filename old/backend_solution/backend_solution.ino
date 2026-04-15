#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

const char* ssid = "Vertical_Garden_AP";
const char* password = "password";

ESP8266WebServer server(80);

// Pin configurations (matching D1 mini typical usages)
const int PUMP_PIN = 5;  // D1
const int MOISTURE_PIN = A0;
const int RED_PIN = 12;  // D6
const int GREEN_PIN = 13; // D7
const int BLUE_PIN = 14;  // D5

// State
bool pumpOn = false;
unsigned long pumpDurationMs = 5000;
unsigned long pumpStartTime = 0;
bool autoPumpEnabled = false;
bool manualPumpControlEnabled = true;
int moistureThresholdPercent = 30;
unsigned long autoPumpDurationMs = 5000;
unsigned long autoPumpCooldownMs = 60000;

bool ledStripOn = false;
int ledStripR = 0;
int ledStripG = 0;
int ledStripB = 0;
String ledEffect = "solid";
unsigned long ledEffectSpeedMs = 1000;

bool lightScheduleEnabled = false;
int lightOnMinute = 480; // 8:00
int lightOffMinute = 1200; // 20:00

// Time mock
unsigned long controllerEpochMs = 1680000000000ULL;
String controllerTimeZone = "Europe/Berlin";

void addCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleOptions() {
    addCorsHeaders();
    server.send(204);
}

void sendJson(const JsonDocument& doc) {
    String output;
    serializeJson(doc, output);
    addCorsHeaders();
    server.send(200, "application/json", output);
}

void handleGetStatus() {
    StaticJsonDocument<512> doc;
    
    // Pump
    JsonObject pump = doc.createNestedObject("pump");
    pump["on"] = pumpOn;
    pump["durationMs"] = pumpDurationMs;
    
    unsigned long remaining = 0;
    if (pumpOn && pumpDurationMs > 0) {
        unsigned long elapsed = millis() - pumpStartTime;
        if (elapsed < pumpDurationMs) {
            remaining = pumpDurationMs - elapsed;
        }
    }
    pump["remainingMs"] = remaining;
    pump["autoEnabled"] = autoPumpEnabled;
    pump["manualEnabled"] = manualPumpControlEnabled;
    pump["thresholdPercent"] = moistureThresholdPercent;
    pump["autoDurationMs"] = autoPumpDurationMs;
    pump["autoCooldownMs"] = autoPumpCooldownMs;

    // Moisture
    int raw = analogRead(MOISTURE_PIN);
    int percent = map(raw, 1024, 0, 0, 100);
    percent = constrain(percent, 0, 100);
    
    JsonObject moisture = doc.createNestedObject("moisture");
    moisture["raw"] = raw;
    moisture["percent"] = percent;

    // LED Strip
    JsonObject ledStrip = doc.createNestedObject("ledStrip");
    ledStrip["on"] = ledStripOn;
    ledStrip["r"] = ledStripR;
    ledStrip["g"] = ledStripG;
    ledStrip["b"] = ledStripB;
    
    // Additional
    doc["ledEffect"] = ledEffect;
    doc["ledEffectSpeedMs"] = ledEffectSpeedMs;
    doc["lightScheduleEnabled"] = lightScheduleEnabled;
    doc["lightOnMinute"] = lightOnMinute;
    doc["lightOffMinute"] = lightOffMinute;

    sendJson(doc);
}

void handleGetSensors() {
    handleGetStatus(); // Same structure conceptually works for sensors here
}

void handleGetTime() {
    StaticJsonDocument<256> doc;
    doc["controllerEpochMs"] = controllerEpochMs + millis();
    doc["timezone"] = controllerTimeZone;
    sendJson(doc);
}

void handlePostTime() {
    if (server.hasArg("plain") == false) {
        addCorsHeaders();
        server.send(400, "application/json", "{\"error\":\"Body required\"}");
        return;
    }
    StaticJsonDocument<256> doc;
    deserializeJson(doc, server.arg("plain"));
    if (doc.containsKey("epochMs")) {
        controllerEpochMs = doc["epochMs"].as<unsigned long>() - millis();
    }
    addCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handlePostSettings() {
    if (server.hasArg("plain")) {
        StaticJsonDocument<256> doc;
        deserializeJson(doc, server.arg("plain"));
        if (doc.containsKey("timezone")) {
            controllerTimeZone = doc["timezone"].as<String>();
        }
    }
    addCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleGetGardenSettings() {
    StaticJsonDocument<512> doc;
    doc["manualPumpControlEnabled"] = manualPumpControlEnabled;
    doc["moistureThresholdPercent"] = moistureThresholdPercent;
    doc["autoPumpDurationMs"] = autoPumpDurationMs;
    doc["autoPumpCooldownMs"] = autoPumpCooldownMs;
    doc["lightScheduleEnabled"] = lightScheduleEnabled;
    doc["lightOnMinute"] = lightOnMinute;
    doc["lightOffMinute"] = lightOffMinute;
    doc["ledEffect"] = ledEffect;
    doc["ledEffectSpeedMs"] = ledEffectSpeedMs;
    sendJson(doc);
}

void handlePostGardenSettings() {
    if (server.hasArg("plain")) {
        StaticJsonDocument<512> doc;
        deserializeJson(doc, server.arg("plain"));
        
        if (doc.containsKey("manualPumpControlEnabled")) manualPumpControlEnabled = doc["manualPumpControlEnabled"];
        if (doc.containsKey("autoPumpEnabled")) autoPumpEnabled = doc["autoPumpEnabled"];
        if (doc.containsKey("moistureThresholdPercent")) moistureThresholdPercent = doc["moistureThresholdPercent"];
        if (doc.containsKey("autoPumpDurationMs")) autoPumpDurationMs = doc["autoPumpDurationMs"];
        if (doc.containsKey("autoPumpCooldownMs")) autoPumpCooldownMs = doc["autoPumpCooldownMs"];
        if (doc.containsKey("lightScheduleEnabled")) lightScheduleEnabled = doc["lightScheduleEnabled"];
        if (doc.containsKey("lightOnMinute")) lightOnMinute = lightOnMinute = doc["lightOnMinute"];
        if (doc.containsKey("lightOffMinute")) lightOffMinute = doc["lightOffMinute"];
    }
    addCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handlePostPump() {
    if (server.hasArg("plain")) {
        StaticJsonDocument<256> doc;
        deserializeJson(doc, server.arg("plain"));
        
        if (doc["action"] == "toggle") {
            pumpOn = !pumpOn;
            if (pumpOn) {
                if (doc.containsKey("durationMs")) {
                    pumpDurationMs = doc["durationMs"];
                }
                pumpStartTime = millis();
                digitalWrite(PUMP_PIN, HIGH);
            } else {
                digitalWrite(PUMP_PIN, LOW);
            }
        }
    }
    addCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handlePostPumpDuration() {
    if (server.hasArg("plain")) {
        StaticJsonDocument<256> doc;
        deserializeJson(doc, server.arg("plain"));
        if (doc.containsKey("pumpDurationMs")) {
            pumpDurationMs = doc["pumpDurationMs"];
        }
    }
    addCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void updateLED() {
    if (ledStripOn) {
        analogWrite(RED_PIN, map(ledStripR, 0, 255, 0, 1023));
        analogWrite(GREEN_PIN, map(ledStripG, 0, 255, 0, 1023));
        analogWrite(BLUE_PIN, map(ledStripB, 0, 255, 0, 1023));
    } else {
        analogWrite(RED_PIN, 0);
        analogWrite(GREEN_PIN, 0);
        analogWrite(BLUE_PIN, 0);
    }
}

void handlePostLed() {
    if (server.hasArg("plain")) {
        StaticJsonDocument<256> doc;
        deserializeJson(doc, server.arg("plain"));
        
        if (doc.containsKey("on")) ledStripOn = doc["on"];
        if (doc.containsKey("r")) ledStripR = doc["r"];
        if (doc.containsKey("g")) ledStripG = doc["g"];
        if (doc.containsKey("b")) ledStripB = doc["b"];
        
        updateLED();
    }
    addCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handlePostLedEffect() {
    if (server.hasArg("plain")) {
        StaticJsonDocument<256> doc;
        deserializeJson(doc, server.arg("plain"));
        
        if (doc.containsKey("effect")) ledEffect = doc["effect"].as<String>();
        if (doc.containsKey("speedMs")) ledEffectSpeedMs = doc["speedMs"];
    }
    addCorsHeaders();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void setup() {
    Serial.begin(115200);
    
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);
    
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    updateLED();

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    
    Serial.println("");
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.onNotFound(handleOptions); // Important for CORS preflight
    
    server.on("/api/status", HTTP_GET, handleGetStatus);
    server.on("/status", HTTP_GET, handleGetStatus);
    server.on("/api/sensors", HTTP_GET, handleGetSensors);
    
    server.on("/api/time", HTTP_GET, handleGetTime);
    server.on("/api/time", HTTP_POST, handlePostTime);
    
    server.on("/api/settings", HTTP_POST, handlePostSettings);
    
    server.on("/api/garden/settings", HTTP_GET, handleGetGardenSettings);
    server.on("/api/garden/settings", HTTP_POST, handlePostGardenSettings);
    
    server.on("/api/pump", HTTP_POST, handlePostPump);
    server.on("/api/pump-duration", HTTP_POST, handlePostPumpDuration);
    
    server.on("/api/led", HTTP_POST, handlePostLed);
    server.on("/api/led/effect", HTTP_POST, handlePostLedEffect);

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
    
    // Auto turn off pump if duration passed
    if (pumpOn && pumpDurationMs > 0) {
        if (millis() - pumpStartTime >= pumpDurationMs) {
            pumpOn = false;
            digitalWrite(PUMP_PIN, LOW);
        }
    }
}
