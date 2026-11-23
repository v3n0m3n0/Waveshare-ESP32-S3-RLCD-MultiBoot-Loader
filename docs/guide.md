# User & Configuration Guide — Waveshare ESP32-S3 RLCD MultiBoot Loader

---

## 1. Physical Hardware Operation

The MultiBoot Loader can be operated completely offline without a computer or Wi-Fi network using the on-board controls and screen:

```
+--------------------------------------------------------+
| [WIFI: 192.168.0.91]                  4.12V  92% [|||] | <- Inverted Header Bar
+--------------------------------------------------------+
|                                                        |
|  [x] Web Radio & Markets            (rlcd-webradio.bin)| <- Active in app1
|  [ ] Game Boy Color & DMG           (rlcd-gameboy.bin) |
|  [ ] Bitcoin Solo Miner             (rlcd-btc-miner.bin|
|  [ ] PC Monitor & Cyber-Pet         (rlcd-pc-monitor.bi|
|                                                        |
+--------------------------------------------------------+
| [KEY] Select Module               [BOOT] Launch / Flash| <- Bottom Action Bar
+--------------------------------------------------------+
```

### Button Controls
- **`[KEY]` Button (GPIO 18):**
  - **Single Press:** Moves the cursor down to select the next firmware in the list. Wraps around automatically at the end of the list.
- **`[BOOT]` Button (GPIO 0):**
  - **Single Press:** Launches the currently highlighted firmware.
    - If the firmware is already written in `app1` (`[x]`), it boots immediately (0 seconds).
    - If the firmware is not in `app1`, the launcher flashes the `.bin` from the MicroSD card to partition `app1` with a graphical progress bar, then boots it.
  - **Press during Auto-Boot Countdown:** Immediately cancels the countdown and keeps the device in the interactive MultiBoot Launcher menu.

---

## 2. Web Device Hub (`http://<device-ip>/`)

When the ESP32-S3 connects to your Wi-Fi network, it starts an embedded web management portal on port 80.

### Features
1. **Module Launcher & Status:**
   - Displays all `.bin` files detected on the MicroSD card under `/firmware/`.
   - Shows which firmware is currently active in `app1`.
   - Single-click **Launch** button to flash or start any module.
2. **Persistent Firmware Reordering:**
   - Click the **▲** (Move Up) or **▼** (Move Down) buttons next to any module to customize your preferred on-screen list order.
   - The order is instantly saved in NVS and reflects on the RLCD screen.
3. **Firmware & ROM Uploads:**
   - **Upload Firmware:** Directly upload `.bin` files into the `/firmware/` directory on the MicroSD card.
   - **Upload ROMs:** Upload `.gb` or `.gbc` ROM files into `/roms/` for the Game Boy module.
4. **File Management:**
   - Delete unused firmware or ROM files with a single click.
5. **Wi-Fi Configuration:**
   - Update Wi-Fi SSID and password directly through the web portal.

---

## 3. MicroSD Card Organization

The MultiBoot Loader requires a MicroSD card formatted as **FAT32** or **exFAT** (recommended: FAT32, allocation size 32 KB).

Recommended folder structure:
```
/
├── firmware/
│   ├── rlcd-webradio.bin        # Web Radio & Markets module
│   ├── rlcd-gameboy.bin         # Game Boy Color & DMG emulator
│   └── custom-app.bin           # Any custom user module
└── roms/
    ├── Tetris.gb                # Game Boy ROMs
    └── MarioDeluxe.gbc
```

---

## 4. USB Serial CLI Reference (`115200` baud)

Connect your ESP32-S3 to a PC via the USB port. The USB-CDC serial interface provides a full interactive command suite:

| Command | Description | Example Output |
|---|---|---|
| `STATUS` | Prints system status, active app, Wi-Fi IP, and battery voltage | `Status: LAUNCHER_READY, ActiveApp: rlcd-webradio.bin, Battery: 4.12 V (92%)` |
| `BOOT <filename>` | Flashes and boots the specified firmware from `/firmware/` | `[LAUNCHER] Flashing from SD card: rlcd-webradio.bin...` |
| `SDLIST` | Lists all detected firmware binaries and ROM files | `[SD] Found firmware: rlcd-webradio.bin (2321728 bytes)` |
| `BATTERY` / `BAT` | Reads raw and filtered battery voltage from ADC | `Battery: 4.12 V (92%)` |
| `LAUNCHER` | Safely resets boot partition pointer to `app0` | `[COMMAND] Next boot set to Launcher` |
| `WIFI <ssid> <pass>` | Updates Wi-Fi credentials in NVS and restarts device | `[WIFI] Saved credentials. Restarting...` |
