# Release Notes — Waveshare ESP32-S3 RLCD MultiBoot Loader

---

## Latest Release: v1.0.0 (`2026-11-21`)

**Initial public release of the Waveshare ESP32-S3 RLCD MultiBoot Loader** — powering the **Waveshare / Luckfox ESP32-S3 4.2″ Reflective LCD (RLCD)** with an autonomous Dual-OTA firmware manager, high-speed SDMMC MicroSD loading, browser-based Web Device Hub with drag-and-drop firmware reordering, interruptible Auto-Boot countdown, unified high-contrast graphical splash screen, and physical hardware reset protection.

For full release details and binary packages, see [docs/releases/v1.0.0.md](docs/releases/v1.0.0.md) and [release/README.md](release/README.md).

### Summary of v1.0.0 Features
- **Dual-OTA Architecture (`app0` Launcher / `app1` Guest Modules):** Keeps the MultiBoot Loader permanently intact in `app0` (`3.5 MB`), while flashing selected guest firmwares into `app1` (`3.5 MB`) on demand. Includes a boot-pointer contract ensuring any reboot, power-cycle, or crash safely returns to the MultiBoot Loader.
- **Negative Double-Outlined Battery Gauge:** Precision battery monitoring via ADC1_CH3 with smoothed filtering, rendering a high-contrast negative battery indicator (`voltage + % + double-bezel icon`) on the inverted top header bar.
- **Persistent Firmware Reordering via WebUI:** Web Manager interface with up/down arrows (`▲`/`▼`) submitting to `POST /api/firmware/order`, permanently persisting the custom firmware order in NVS (`"launcher"` namespace).
- **Configurable Auto-Boot Countdown:** Configurable splash screen countdown that automatically launches the designated default application unless interrupted by pressing `[KEY]`, `[BOOT]`, or sending a serial command.
- **Unified Technical Splash Screen & Progress Bar:** High-contrast graphic splash screen featuring an ESP32-S3 silicon package emblem, board specs, and animated progress bar used across cold boot and SD-to-Flash flashing operations.
- **High-Speed SDMMC 1-Bit Driver:** Native 40 MHz SDMMC peripheral support scanning `/firmware/*.bin` files and `/roms/` in milliseconds with automatic metadata and sizing detection.
- **Responsive Web Device Hub:** Embedded web portal on port 80 with firmware flashing, binary uploading, SD card file deletion, and Wi-Fi credential management.
- **Physical Button & USB-CDC Control:** Responsive on-device navigation with `[KEY]` (next item) and `[BOOT]` (launch/flash), alongside a complete USB Serial CLI suite (`STATUS`, `BOOT <name>`, `SDLIST`, `BATTERY`, `LAUNCHER`).
