# TNB Meter Tracker (ESP32)

Firmware for ESP32 that counts optical meter pulses, computes energy usage, and uploads readings to a web endpoint.

## What This Firmware Does

- Reads pulse events from an LDR light sensor module digital output (D0) on GPIO4.
- Debounces pulses in an interrupt service routine.
- Every interval (`UPLOAD_INTERVAL`):
  - Converts pulse count to kWh.
  - Estimates average power (W) over the interval.
  - Sends JSON data to a configured web endpoint.
- Syncs time from NTP (Malaysia UTC+8) and timestamps each reading.
- Buffers readings in NVS when offline and retries upload when connection recovers.
- Supports optional OTA firmware updates over local Wi-Fi.
- Uses watchdog protection to reduce lockup risk.

## Hardware

- ESP32 Dev Module
- Photo-resistor LDR light sensor module (LM393 type with D0/A0)
- Wiring:
  - Sensor D0 -> GPIO4
  - Sensor VCC -> ESP32 3V3
  - Sensor GND -> ESP32 GND

### Sensor Placement and Tuning

- Point the LDR directly at the meter pulse LED.
- Shield the sensor from room light (black tape or short heat-shrink tunnel helps a lot).
- Power module from 3.3V to keep ESP32 GPIO safe.
- Turn module trimpot until module D0 indicator toggles only when the meter LED blinks.
- If counts look inverted, switch interrupt edge in code from `FALLING` to `RISING`.

### If Pulse Count Is Too High (x5/x10)

- Symptom usually means comparator chatter around threshold, not real meter pulses.
- Increase lockout in `platformio.ini` build flags:
  - `-D PULSE_LOCKOUT_MS=120` (default)
  - Try `150` or `200` if still overcounting.
- Keep `DEBOUNCE_MS` in code for short bounce; lockout handles repeated false triggers.
- Calibrate on real meter LED pulses; phone/video blinking can create non-real transitions.

## Data Payload

The firmware sends JSON like:

```json
{
  "timestamp": "13-04-2026 20:45:10",
  "pulse_count": 12,
  "kwh": 0.012,
  "watts": 1440.0,
  "source": "live",
  "token": "LONG_RANDOM_SECRET"
}
```

`source` can be:
- `live` for current interval data
- `nvs_recovery` for buffered data flushed after reconnect

### Request/Response Contract

- Request method: `POST`
- Request headers: `Content-Type: application/json`, `X-Device-Token: <token>`
- Request body: JSON with `timestamp`, `pulse_count`, `kwh`, `watts`, `source`, `token`
- Redirect behavior: for Google Apps Script `30x` responses, firmware follows `Location` manually with a clean `GET`.
- Success condition in firmware: HTTP `2xx` and response body exactly `ok`
- Any other response body (for example `unauthorized`) is treated as failed upload and data remains buffered for retry

## Configuration

### 1) Local secrets file (recommended)

Edit `secrets.local.ini` (already git-ignored):

```ini
[secrets]
wifi_ssid = YOUR_WIFI_NAME
wifi_pass = YOUR_WIFI_PASSWORD
script_url = https://script.google.com/macros/s/your-id/exec
device_token = LONG_RANDOM_SECRET
ota_host = tnb-meter-tracker
ota_password = OPTIONAL_OTA_PASSWORD
ota_ip = tnb-meter-tracker.local
```

Share only `secrets.local.ini.example` in repositories.

### 2) PlatformIO settings

`platformio.ini` injects these values into build flags for firmware compilation.

### 3) OTA update setup (optional)

- OTA requires one initial USB flash first.
- After first boot with OTA-enabled firmware, check serial log for:
  - `[OTA] Ready. Hostname: ...`
  - Wi-Fi IP (you can use IP or `hostname.local` for `ota_ip`)
- Ensure PC and ESP32 are on the same LAN/subnet.

## Build and Upload

From project folder:

```powershell
C:\Users\Personal\.platformio\penv\Scripts\platformio.exe run -e esp32dev
C:\Users\Personal\.platformio\penv\Scripts\platformio.exe run -t upload -e esp32dev
```

OTA upload environment:

```powershell
C:\Users\Personal\.platformio\penv\Scripts\platformio.exe run -e esp32ota
C:\Users\Personal\.platformio\penv\Scripts\platformio.exe run -t upload -e esp32ota
```

Recommended first-time flow:

1. USB flash once using `esp32dev` environment (firmware includes OTA support).
2. Set `ota_ip` in `secrets.local.ini` to the board IP shown in serial logs.
3. Run OTA upload using `-e esp32ota` over Wi-Fi.

Optional serial monitor:

```powershell
C:\Users\Personal\.platformio\penv\Scripts\platformio.exe device monitor -b 115200
```

## Security Notes

- Keep `script_url` on HTTPS only. Firmware rejects non-HTTPS URLs.
- Firmware sends `X-Device-Token` header on every upload; backend should reject requests with wrong token.
- Firmware treats upload as success only when HTTP is 2xx and response body is `ok`.
- Do not commit `secrets.local.ini`.
- Secrets are still present in flashed firmware image; this setup protects source control, not physical firmware extraction.
- Avoid exposing ESP32 directly to the internet (no router port forwarding).

## Server Requirements (Apps Script)

Your endpoint should follow these rules:

- Accept HTTP POST with JSON body.
- Parse and validate these fields: `timestamp`, `pulse_count`, `kwh`, `watts`, `source`, `token`.
- Verify `token` matches a value stored in Apps Script Script Properties.
- On success, return response body exactly `ok`.
- On auth fail or validation fail, return a non-`ok` body (for example `unauthorized` or `bad_request`).
- Finish request handling quickly (target under 5 seconds) to avoid firmware timeout.

Reference Apps Script pattern:

```javascript
const TOKEN_PROPERTY_KEYS = ["DEVICE_TOKEN", "device_token"];

function doPost(e) {
  let body = {};
  try {
    body = JSON.parse((e && e.postData && e.postData.contents) ? e.postData.contents : "{}");
  } catch (err) {
    return ContentService.createTextOutput("bad_request").setMimeType(ContentService.MimeType.TEXT);
  }

  const scriptProperties = PropertiesService.getScriptProperties();
  const expectedToken = TOKEN_PROPERTY_KEYS
    .map(key => String(scriptProperties.getProperty(key) || "").trim())
    .find(Boolean);

  if (!body.timestamp || body.pulse_count === undefined || body.kwh === undefined || body.watts === undefined || !body.source) {
    return ContentService.createTextOutput("bad_request").setMimeType(ContentService.MimeType.TEXT);
  }

  if (!expectedToken) {
    return ContentService.createTextOutput("bad_request").setMimeType(ContentService.MimeType.TEXT);
  }

  if (body.token !== expectedToken) {
    return ContentService.createTextOutput("unauthorized").setMimeType(ContentService.MimeType.TEXT);
  }

  // existing sheet write here
  return ContentService.createTextOutput("ok").setMimeType(ContentService.MimeType.TEXT);
}
```

Set the Apps Script property under `Project Settings -> Script properties` using key `DEVICE_TOKEN` (preferred). `device_token` is also accepted for compatibility.

Note: Firmware also sends token in `X-Device-Token` header. Body-based validation keeps this compatible with Google Apps Script web apps.

## Runtime Behavior

- Upload interval: 30 seconds
- Debounce: 50 ms
- Pulses-per-kWh: 1000 (adjust if your meter differs)
- Offline buffer capacity: up to 2880 entries (~24 hours at 30-second intervals)

## Tuning

Change these in `src/main.cpp`:

- `IMP_PER_KWH`
- `UPLOAD_INTERVAL`
- `DEBOUNCE_MS`
- `MAX_NVS_ENTRIES`
- `SENSOR_PIN`

## Troubleshooting

- If upload fails, confirm USB driver and COM port selection in PlatformIO.
- If no data reaches server, verify:
  - Wi-Fi credentials
  - HTTPS endpoint URL
  - Serial logs for `[HTTP]` and `[CONFIG]` messages
