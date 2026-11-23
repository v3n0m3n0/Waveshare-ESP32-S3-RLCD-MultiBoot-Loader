# Releasing Waveshare ESP32-S3 RLCD MultiBoot Loader

Every official release of **Waveshare ESP32-S3 RLCD MultiBoot Loader** adheres to the following engineering, licensing, and reproducibility standards.

## 1. Versioning Policy
- Strictly follows Semantic Versioning (`vMAJOR.MINOR.PATCH`, e.g., `v1.0.0`).
- Update `version.txt` before compiling release binaries and tagging.
- The Git tag format is `vX.Y.Z` (annotated tag).

## 2. Binary Packaging & Integrity
- Every release artifact in `release/` is built cleanly from source using PlatformIO.
- Prebuilt release packages include:
  1. `rlcd-launcher-v<ver>.bin` — Standalone application binary for the `app0` partition (`0x10000`).
  2. `waveshare-esp32s3-rlcd-multiboot-loader-full-16MB-v<ver>.bin` — Unified factory flash image starting at `0x0` (merges `bootloader.bin`, `partitions.bin`, `otadata.bin`, and application).
  3. `bootloader.bin` (`0x0`), `partitions.bin` (`0x8000`), `otadata.bin` (`0xe000`).
  4. `flash.ps1` & `flash.bat` — Standalone Windows flash helpers supporting automatic COM port detection, UTF-8 console output, and `--after watchdog-reset`.
  5. `SHA256SUMS.txt` — Cryptographic verification manifest of all binaries and scripts.
- Only the latest release binaries remain tracked in `release/` on `main`. Historical archives are hosted exclusively on the **GitHub Releases** page.

## 3. Dedicated Application Modules Policy
- This repository is dedicated **solely to the MultiBoot Loader (`app0`)**.
- Application modules (Web Radio & Markets, Game Boy Color & DMG Emulator, etc.) are developed and maintained in **their own independent GitHub repositories**.
- Release notes must never bundle guest application binaries into this launcher repository.
