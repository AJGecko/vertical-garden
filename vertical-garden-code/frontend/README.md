# Vertical Garden Frontend

This folder contains the browser-based control panel for the Vertical Garden controller.
It is a static frontend that runs in the browser and communicates directly with the ESP8266 backend on the local network.

## Overview

The UI lets you:

- connect to the controller by IP address
- check controller status
- send time settings to the controller
- switch between automatic and manual time mode
- change the timezone
- use a custom port
- enable live polling
- switch between German and English
- toggle light and dark theme
- save your preferences in the browser

## Files

- `index.html` - page structure
- `styles.css` - layout, colors, and responsive styling
- `app.js` - UI logic, controller communication, persistence, and time conversion

## How it works

The frontend uses plain browser APIs:

- `fetch()` for HTTP requests
- `localStorage` for saving user settings
- `Intl.DateTimeFormat` for timezone handling
- `setInterval()` for live updates and polling

The controller URL is built from the entered IP address and port, for example:

```text
http://192.168.178.45:80
```

## API endpoints used by the frontend

### GET

The frontend tries these status endpoints in order:

- `GET /api/time`
- `GET /api/status`
- `GET /status`

### POST

The frontend sends data to:

- `POST /api/settings`
- `POST /api/time`

## API request and response formats

The frontend accepts a small set of response shapes because the backend can return slightly different field names for the same information.

### 1) `GET /api/time`

Purpose: read the current controller time and timezone.

Request body: none.

Example response:

```json
{
  "timezone": "Europe/Berlin",
  "localTime": "2026-04-09T14:35:12"
}
```

Other accepted response field names:

- `tz` instead of `timezone`
- `localDateTime`, `time`, `datetime`, or `isoTime` instead of `localTime`

Example alternative response:

```json
{
  "tz": "Europe/Berlin",
  "datetime": "2026-04-09T14:35:12Z"
}
```

### 2) `GET /api/status`

Purpose: read controller status data.

Request body: none.

Expected response: same format as `GET /api/time`.

Example response:

```json
{
  "timezone": "Europe/Berlin",
  "localTime": "2026-04-09T14:35:12"
}
```

### 3) `GET /status`

Purpose: fallback status endpoint if the API routes above are not available.

Request body: none.

Expected response: same format as `GET /api/time`.

Example response:

```json
{
  "timezone": "Europe/Berlin",
  "localTime": "2026-04-09T14:35:12"
}
```

### 4) `POST /api/settings`

Purpose: update controller settings.

Request body:

```json
{
  "timezone": "Europe/Berlin",
  "liveRateEnabled": true,
  "liveIntervalMs": 500
}
```

Field details:

- `timezone`: IANA timezone string such as `Europe/Berlin`
- `liveRateEnabled`: boolean flag for custom live polling rate
- `liveIntervalMs`: polling interval in milliseconds

Expected response:

- Any successful HTTP 2xx response is accepted by the frontend.
- The body can be plain text or JSON.

Example successful response:

```text
OK
```

Example JSON response:

```json
{
  "ok": true,
  "message": "Settings updated"
}
```

### 5) `POST /api/time`

Purpose: set the controller time.

Request body in automatic mode:

```json
{
  "timezone": "Europe/Berlin",
  "localTime": "2026-04-09T14:35:12",
  "mode": "now"
}
```

Request body in manual mode:

```json
{
  "timezone": "Europe/Berlin",
  "localTime": "2026-04-09T08:00:00",
  "mode": "manual"
}
```

Field details:

- `timezone`: IANA timezone string
- `localTime`: ISO-like local datetime string in `YYYY-MM-DDTHH:mm:ss` format
- `mode`: either `now` or `manual`

Expected response:

- Any successful HTTP 2xx response is accepted by the frontend.
- The body can be plain text or JSON.

Example successful response:

```text
Time updated
```

Example JSON response:

```json
{
  "ok": true,
  "message": "Time updated"
}
```

## Example requests

### Status check

```bash
curl http://192.168.178.45/api/time
```

### Update settings

```bash
curl -X POST http://192.168.178.45/api/settings \
  -H "Content-Type: application/json" \
  -d '{"timezone":"Europe/Berlin","liveRateEnabled":true,"liveIntervalMs":500}'
```

### Set time automatically

```bash
curl -X POST http://192.168.178.45/api/time \
  -H "Content-Type: application/json" \
  -d '{"timezone":"Europe/Berlin","mode":"now","localTime":"2026-04-09T14:35:12"}'
```

### Set time manually

```bash
curl -X POST http://192.168.178.45/api/time \
  -H "Content-Type: application/json" \
  -d '{"timezone":"Europe/Berlin","mode":"manual","localTime":"2026-04-09T08:00:00"}'
```

## Browser storage

The frontend stores these values locally in the browser:

- controller IP
- timezone
- time mode
- manual time
- theme
- language
- port selection
- live mode
- live rate setting
- live interval

## Time handling

The frontend supports two time modes:

- automatic: uses the current browser time
- manual: lets the user enter a specific local datetime

When manual time is used, the selected timezone is applied before the value is sent to the controller.

## Running locally

You can open the frontend in a browser or serve it with a simple local HTTP server.

Example with Python:

```bash
python3 -m http.server
```

Then open the frontend folder in the browser through the local server.

## Notes

- The controller must be reachable on the local network.
- The browser may block requests when the frontend is served over HTTPS and the controller is only available over HTTP.
- Live mode repeatedly polls the controller for updated status data.

## Backend compatibility

This frontend is designed to work with the simple ESP8266 backend in the project.
The backend is expected to expose the status, settings, and time endpoints listed above.
