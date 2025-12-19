# ESP32 MagicBand+ BLE Broadcaster

A small ESP32-S3 project that broadcasts MagicBand+ BLE manufacturer packets and exposes a web interface (captive-portal style) to trigger light/vibration presets from a browser or scripts.

**Quick summary:** boot the device, connect to the access point (`MagicBand-Controller` by default), open the web UI, and send preset or custom commands to broadcast MagicBand+ packets over BLE.

**Features**
- **BLE broadcaster:** Sends MagicBand+ style manufacturer data to nearby devices.
- **Wi‑Fi captive portal:** The ESP runs an AP and DNS server so phones open the web UI automatically.
- **Web UI + API:** A small frontend is served from LittleFS; POST to `/command` to trigger effects.

**Files of interest**
- `src/main.cpp`: main firmware — BLE initialization, WiFi AP, captive DNS, web server routes and packet broadcast logic.
- `frontend/` (frontend): static web files that are compiled and uploaded to LittleFS and served by the ESP (`index.html`, `index.js`, `index.css`).
- `platformio.ini`: PlatformIO project configuration (build/flash settings).
- `partition_map.csv`: Describes how to divide up flash to support littlefs

Hardware
- Any ESP32 board that supports BLE and LittleFS (ESP32-S3 is used in development). Make sure you have the appropriate board selected in `platformio.ini`.

Configuration
- AP SSID / password: change the constants at the top of `src/main.cpp`:
  - `ap_ssid` — default `MagicBand-Controller`
  - `ap_password` — default `magicband123`

How it works
- The web server serves files from LittleFS. The UI sends POST requests to `/command` with parameters to build manufacturer data packets.
- The firmware creates BLE advertisement manufacturer data and broadcasts for ~2 seconds.

Web API
- Endpoint: `POST /command`
- Parameters (form-data / x-www-form-urlencoded):
  - `action` (required): one of `ping`, `preset`, `rainbow`, `dual`, `circle`, `crossfade`
  - `color`, `c1`..`c5`: numeric color codes (or named values used by the UI) for presets
  - `vib`: vibration intensity (0–15)

Example curl commands
- Ping:
  `curl -X POST http://192.168.4.1/command -F 'action=ping'`
- Send a red preset with vibration level 3:
  `curl -X POST http://192.168.4.1/command -F 'action=preset' -F 'color=red' -F 'vib=3'`
- Send a 5-color rainbow (use numeric color codes for `c1`..`c5`):
  `curl -X POST http://192.168.4.1/command -F 'action=rainbow' -F 'c1=15' -F 'c2=21' -F 'vib=2'`

Building & flashing
- Open the project in VS Code with the PlatformIO extension.
- To build and upload firmware: use the PlatformIO task `Upload` (or run `PlatformIO: Upload`).
- To upload the LittleFS filesystem (the `data/` folder containing the web UI): use PlatformIO's `Build Filesystem Image` and `Upload Filesystem image` task. 

Notes & safety
- The firmware broadcasts manufacturer data as advertisements; no BLE connections are required.

Credits & references
- Based on information from: https://emcot.world/Disney_MagicBand%2B_Bluetooth_Codes (see code comments in `src/main.cpp`).