# Prebuilt Release Binaries (`v1.0.0`)

This directory contains the production release artifacts for **Waveshare ESP32-S3 RLCD MultiBoot Loader (`v1.0.0`)**, including automated Windows flashing scripts and SHA-256 integrity checksums.

---

## Package Contents

| File | Offset | Description |
|---|---|---|
| `waveshare-esp32s3-rlcd-multiboot-loader-full-16MB-v1.0.0.bin` | `0x0` | Unified factory flash image (bootloader + partitions + otadata + app0). |
| `rlcd-launcher-v1.0.0.bin` | `0x10000` | Standalone MultiBoot Loader application binary (`app0`). |
| `bootloader.bin` | `0x0` | ESP32-S3 secondary bootloader. |
| `partitions.bin` | `0x8000` | 16 MB Dual-OTA partition table binary. |
| `otadata.bin` | `0xE000` | Initial OTA data structure directing startup to `app0`. |
| `flash.ps1` / `flash.bat` | — | Automated Windows flashing script with watchdog-reset support. |
| `SHA256SUMS.txt` | — | SHA-256 cryptographic checksums for all release assets. |

---

## Flashing Instructions (Windows)

### Quick Flash (Preserves NVS Settings)
```powershell
.\flash.ps1 -Port COM11
```
Or double-click `flash.bat` and enter your COM port.

### Factory Clean Flash (Full 16 MB Image at `0x0`)
To flash a brand-new board from scratch:
```powershell
.\flash.ps1 -Port COM11 -FullImage
```

---

## Checksum Verification

Verify local files against `SHA256SUMS.txt`:
```powershell
Get-FileHash *.bin, flash.ps1, flash.bat, README.md | Format-Table Hash, Path
```
