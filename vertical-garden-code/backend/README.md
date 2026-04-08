# Wemos D1 mini Backend (ESP8266) - Simple

Dieses Backend ist absichtlich minimal und passt zu deinem Frontend in Vercel.

## API (kompatibel zum Frontend)

- `GET /api/status`
- `GET /api/time` (liefert auch Status)
- `GET /status`
- `POST /api/time`
- `POST /api/settings`
- `GET /api/wifi`
- `POST /api/wifi` (SSID/Passwort setzen)
- `OPTIONS` fuer CORS-Preflight auf den API-Routen

WLAN-Zugangsdaten werden im EEPROM gespeichert und nach Neustart wieder geladen.

## 1) Arduino IDE vorbereiten

- Board installieren: **ESP8266 by ESP8266 Community**
- Board waehlen: **LOLIN(WEMOS) D1 R2 & mini**

## 2) Sketch anpassen

Datei: `wemos_d1_mini_backend_simple.ino`

Setze oben deine WLAN-Daten:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

## 3) Flashen

- Wemos D1 mini per USB verbinden
- COM/tty Port auswaehlen
- Upload starten

Im Serial Monitor (115200) siehst du danach die IP, z. B. `192.168.178.45`.

## 4) Frontend verbinden

- Im Vercel-Frontend diese IP als Controller-IP eintragen
- Port auf `80` lassen

## Hinweis zu Vercel (wichtig)

Ein HTTPS-Frontend (Vercel) zu einem lokalen HTTP-Geraet kann durch Browser-Regeln blockiert werden (Mixed Content / Private Network Access).

Wenn das passiert, teste alternativ:

- Frontend lokal per HTTP starten (gleicher LAN-Kontext)
- oder ESP per HTTPS/Proxy/Tunnel erreichbar machen

## Beispiel-Requests

```bash
curl http://192.168.178.45/api/status

curl -X POST http://192.168.178.45/api/settings \
  -H "Content-Type: application/json" \
  -d '{"timezone":"Europe/Berlin","liveRateEnabled":true,"liveIntervalMs":1000}'

curl -X POST http://192.168.178.45/api/time \
  -H "Content-Type: application/json" \
  -d '{"timezone":"Europe/Berlin","mode":"now","localTime":"2026-04-08T12:00:00"}'
```
