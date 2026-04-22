# CYD ESP32 Pi-hole Dashboard

A Pi-hole v6 stats dashboard for the **ESP32-2432S028R** ("Cheap Yellow Display" / CYD). Displays live blocking stats across three rotating slides, polling your Pi-hole every 30 seconds.

Built from scratch for the CYD hardware. No cloud dependency, no app — just your Pi-hole and a $15 display.

---

## Hardware

**ESP32-2432S028R** — the ubiquitous "Cheap Yellow Display":
- ESP32 dual-core, 240MHz
- 2.8" ILI9341 TFT, 320×240
- USB-C or Micro-USB (depending on revision)

Available from AliExpress, Amazon, and similar for ~$10–15.

---

## What It Shows

Three slides cycle automatically every 5 seconds:

**Slide 1 — Overview**
- Block percentage (large, color-coded green/orange)
- Total queries today
- Total blocked today
- Header turns red if Pi-hole blocking is disabled

**Slide 2 — Blocking Stats**
- Proportional bar charts for queries, blocked, cached, and forwarded traffic

**Slide 3 — Blocklist**
- Total domains in gravity blocklist
- Total clients seen
- Cached query count

A dot indicator at the bottom of every slide shows your position in the rotation.

---

## Requirements

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- Pi-hole v6 (uses the v6 session-based API — not compatible with v5)
- 2.4GHz WiFi network (the ESP32 does not support 5GHz)

---

## Installation

### 1. Clone the repo

```bash
git clone https://github.com/jasonpersinger/CYD-ESP32-PIHOLE-DASHBOARD.git
cd CYD-ESP32-PIHOLE-DASHBOARD
```

### 2. Create your secrets file

```bash
cp src/secrets.h.example src/secrets.h
```

Edit `src/secrets.h` and fill in your details:

```cpp
#define WIFI_SSID     "your_wifi_ssid"       // Must be 2.4GHz
#define WIFI_PASSWORD "your_wifi_password"
#define PIHOLE_IP     "192.168.x.x"          // Local IP of your Pi-hole
#define PIHOLE_PASS   "your_pihole_password"
```

`secrets.h` is gitignored and will never be committed.

### 3. Copy the TFT display config

The TFT_eSPI library requires a hardware config file. After PlatformIO fetches dependencies (or after any clean build), copy it into place:

```bash
cp User_Setup.h .pio/libdeps/esp32dev/TFT_eSPI/User_Setup.h
```

> **Why?** PlatformIO's `lib_deps` fetches a fresh copy of TFT_eSPI that doesn't know about the CYD's specific wiring. The included `User_Setup.h` sets the correct driver (`ILI9341_2_DRIVER`), SPI port (`USE_HSPI_PORT`), and critically `TFT_INVERSION_ON` — without that flag, colors are inverted and the background appears white.

### 4. Build and flash

Plug in the CYD via USB, then:

```bash
pio run --target upload
```

Or use the PlatformIO VS Code extension's **Upload** button.

---

## Configuration

Two timing constants at the top of `src/main.cpp`:

```cpp
const int REFRESH_MS = 30000;  // How often to poll Pi-hole (ms)
const int SLIDE_MS   = 5000;   // How long each slide is displayed (ms)
```

---

## Troubleshooting

**"NO DATA" on screen**
- Check that `PIHOLE_IP` is correct and reachable from your network
- Confirm you're using Pi-hole v6 (this firmware uses the v6 session API)
- Verify your Pi-hole password in `secrets.h`

**Colors are inverted / white background**
- The `User_Setup.h` copy step was missed. See Step 3 above.

**Won't connect to WiFi**
- The ESP32 only supports 2.4GHz. If your SSID is 5GHz-only, it will never connect.

**Header is red but Pi-hole isn't disabled**
- Not a bug — Pi-hole v6 reports the active blocking *mode* (e.g. `null`, `ip`, `nxdomain`) rather than the string `"enabled"`. The firmware already handles this: the header only turns red when the status is explicitly `"disabled"`.

---

## Project Structure

```
├── src/
│   ├── main.cpp          # All firmware logic
│   ├── secrets.h         # Your credentials (gitignored)
│   └── secrets.h.example # Template — copy to secrets.h
├── User_Setup.h          # TFT_eSPI hardware config for the CYD
└── platformio.ini        # PlatformIO build config
```

---

## Dependencies

Managed automatically by PlatformIO:

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) `^2.5.43`
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) `^7.0.0`

---

## License

MIT
