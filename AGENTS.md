# AGENTS.md — Waveshare ESP32-S3 RLCD MultiBoot Loader

This guide provides architectural context and operational rules for AI assistants, agents, and contributors modifying this codebase.

---

## 1. Project Purpose & Ecosystem Context

The **Waveshare ESP32-S3 RLCD MultiBoot Loader** is a dedicated, autonomous firmware loader and web manager engineered for the **Waveshare / Luckfox ESP32-S3 4.2″ Reflective LCD (RLCD) Development Board** (`ESP32-S3-WROOM-1-N16R8`, ST7305 controller, 1-bit monochrome `400×300`, SDMMC MicroSD, Octal PSRAM).

### Multi-Repository Architecture
- **This Repository:** Owns exclusively the bootloader/manager application residing in the `app0` partition.
- **Application Modules:** Every functional guest firmware (such as Web Radio & Markets, Game Boy Color & DMG, PC Monitor, TRMNL Frame) is maintained in **its own dedicated GitHub repository**.
- Modules compile to standalone binaries targeted for `app1` (`0x390000`).
- The MultiBoot Loader scans `/firmware/*.bin` on the MicroSD card, flashes the chosen module into `app1`, and branches execution to it.

---

## 2. Flash Partition Layout (`partitions.csv`)

The board uses a 16 MB QIO SPI Flash partitioned as:
```csv
# ESP32-S3 16MB Partition Table with Dual OTA and Storage
# Name,   Type, SubType, Offset,   Size,     Flags
nvs,      data, nvs,     0x9000,   0x5000,
otadata,  data, ota,     0xe000,   0x2000,
app0,     app,  ota_0,   0x10000,  0x380000,
app1,     app,  ota_1,   0x390000, 0x380000,
storage,  data, spiffs,  0x710000, 0x8E0000,
coredump, data, coredump,0xFF0000, 0x10000,
```

### Safety Guarantee: Always Return to Launcher
1. `app0` is permanently locked to MultiBoot Loader.
2. In `flasher.cpp`, when a module is launched, the OTA boot pointer is set to `app1` (`esp_ota_set_boot_partition(app1)`).
3. Every compliant module, in its initial `setup()`, executes:
   ```cpp
   const esp_partition_t *launcher = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
   if (launcher) esp_ota_set_boot_partition(launcher);
   ```
4. Therefore, any physical hardware reset (`RST` button), power cycle, crash, or `LAUNCHER` command immediately boots right back into `app0` (MultiBoot Loader).

---

## 3. Hardware Interconnects (`board_pins.h`)

| Subsystem | GPIO | Protocol / Function | Runtime Notes |
|---|---|---|---|
| **ST7305 RLCD (400×300)** | `11` (SCK), `12` (MOSI), `5` (DC), `40` (CS), `41` (RST) | SPI (U8g2 `U8G2_R1`) | Landscape orientation, 1-bit B/W reflective panel |
| **MicroSD Card** | `38` (CLK), `21` (CMD), `39` (D0) | SDMMC 1-Bit (40 MHz) | Direct SDMMC peripheral; requires clean unmounts |
| **Physical Buttons** | `0` (BOOT), `18` (KEY) | Active LOW | Debounced in software; `[KEY]` selects, `[BOOT]` launches |
| **Battery ADC** | `4` (ADC1_CH3) | Resistor divider | Sampled via 12-bit ADC; discharge curve mapped to % |
| **Audio Amp (PA)** | `46` (PA_ENABLE) | Digital Output | Kept LOW (muted) in launcher to prevent pops |

---

## 4. Key Subsystems & Source Files

- `src/main.cpp`: Initialization, auto-boot countdown logic (cancelable via keypress), physical button state machine, loop scheduler.
- `src/display.cpp` / `display.h`: RLCD rendering pipeline with high-contrast negative battery gauge, reordered firmware list with custom icons, unified graphical splash screen with progress bar.
- `src/flasher.cpp` / `flasher.h`: Direct SDMMC-to-Flash streaming copy to `app1` with block verification and immediate boot dispatch.
- `src/sd_manager.cpp` / `sd_manager.h`: MicroSD scanner, firmware metadata extractor (`/firmware/*.bin`), ROM scanner (`/roms/`), storage capacity reporting.
- `src/web_server.cpp` / `web_server.h`: Asynchronous HTTP WebUI server (`PORT 80`) providing firmware listing, reordering (`POST /api/firmware/order`), live flashing (`POST /api/flash`), binary file upload, and Wi-Fi credential management.
- `src/web_ui_assets.h`: Gzipped, modern single-page WebUI HTML/CSS/JS assets.

---

## 5. Development & Testing Instructions

```powershell
# Build using PlatformIO
pio run -e esp32-s3-rlcd-launcher

# Upload directly to ESP32-S3 over USB-CDC
pio run -e esp32-s3-rlcd-launcher -t upload --upload-port COM11
```
Use `--after watchdog-reset` when invoking `esptool.py` to leave ROM download mode cleanly without power-cycling the board.
