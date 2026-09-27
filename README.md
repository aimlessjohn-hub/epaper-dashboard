# ePaper Dashboard (ESP32-S3, Waveshare 1.54")

Deep-Sleep-Dashboard fuer eInk 1.54" 200x200 (GDEY0154D67 + ESP32-S3):

- **Wetter** (Open-Meteo, ohne API-Key) + Zimmer-/Aussentemperatur (SHTC3)
- **Homelab-Status** via status_endpoint (PVE/NVMe/GPU-Temps, Waschmaschine, Verbrauch)
- **Deep Sleep** mit RTC-Timer-Wake (rtc_gpio), EXT1-Button-Wake mit Debounce
- **LSB-first Font-Fix** (font5x7 Bit0=oben, ROT-Mathe bewiesen via rot_sim.py)

## Build
PlatformIO (`platformio.ini`), `cp src/secrets.h.example src/secrets.h` und WiFi eintragen.
