#include "sd_manager.h"
#include <Preferences.h>
#include <algorithm>
#include <esp_heap_caps.h>

SDManager SDCard;

static bool is_supported_dmg_mbc(uint8_t mbc) {
    switch (mbc) {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x05: case 0x06:
        case 0x08: case 0x09:
        case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
            return true;
        default:
            return false;
    }
}

SDManager::SDManager() {}

String SDManager::sanitize_filename(const String &raw, const char *required_ext) {
    String s = raw;
    int slash = s.lastIndexOf('/');
    if (slash >= 0) s = s.substring(slash + 1);
    int bslash = s.lastIndexOf('\\');
    if (bslash >= 0) s = s.substring(bslash + 1);

    String out;
    out.reserve(s.length() + 4);
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') {
            out += c;
        } else if (c == ' ') {
            out += '_';
        }
    }
    while (out.startsWith(".")) out.remove(0, 1);
    if (out.isEmpty()) return "";

    if (required_ext && required_ext[0] != '\0') {
        String lower = out;
        lower.toLowerCase();
        String req = required_ext;
        req.toLowerCase();
        if (!lower.endsWith(req)) {
            if (req == ".gb" && lower.endsWith(".gbc")) {
                // allow .gbc extension as well
            } else {
                out += required_ext;
            }
        }
    }
    return out;
}

String SDManager::get_module_type(const String &filename) {
    String lower = filename;
    lower.toLowerCase();
    if (lower.indexOf("radio") >= 0) return "webradio";
    if (lower.indexOf("gameboy") >= 0 || lower.indexOf("gb") >= 0) return "gameboy";
    if (lower.indexOf("miner") >= 0 || lower.indexOf("btc") >= 0) return "btc-miner";
    if (lower.indexOf("clock") >= 0 || lower.indexOf("calendar") >= 0) return "calendar-clock";
    if (lower.indexOf("pc-monitor") >= 0 || lower.indexOf("pcmon") >= 0) return "pc-monitor";
    if (lower.indexOf("trmnl") >= 0 || lower.indexOf("frame") >= 0) return "trmnl-frame";
    if (lower.indexOf("flight") >= 0 || lower.indexOf("radar") >= 0 || lower.indexOf("air") >= 0) return "flight-air";
    if (lower.indexOf("transit") >= 0 || lower.indexOf("bus") >= 0) return "transit-board";
    if (lower.indexOf("crypto") >= 0 || lower.indexOf("stocks") >= 0 || lower.indexOf("market") >= 0) return "crypto-stocks";
    return "custom";
}

RomInfo SDManager::validate_rom_header(const String &filename, size_t size_bytes, const uint8_t *hdr, size_t hdr_len) {
    RomInfo info;
    info.filename = filename;
    info.size_bytes = size_bytes;
    info.mbc_type = 0;
    info.cgb_flag = 0;
    info.supported = false;

    if (size_bytes < 0x150 || !hdr || hdr_len < 0x150) {
        info.reason = "Invalid header (< 336 B)";
        return info;
    }

    char title_buf[17];
    int tlen = 0;
    for (int i = 0x134; i <= 0x142; ++i) {
        uint8_t ch = hdr[i];
        if (ch == 0) break;
        title_buf[tlen++] = (ch >= 0x20 && ch <= 0x7E) ? (char)ch : '?';
    }
    title_buf[tlen] = '\0';
    info.internal_title = String(title_buf);
    info.internal_title.trim();

    info.cgb_flag = hdr[0x143];
    info.mbc_type = hdr[0x147];

    if (!is_supported_dmg_mbc(info.mbc_type)) {
        char mbuf[32];
        snprintf(mbuf, sizeof(mbuf), "Unsupported MBC 0x%02X", info.mbc_type);
        info.reason = mbuf;
        return info;
    }
    info.supported = true;
    if (info.cgb_flag == 0xC0) {
        info.reason = "GBC Supported (gnuboy)";
    } else if (info.cgb_flag == 0x80) {
        info.reason = "GBC+DMG Hybrid";
    } else {
        info.reason = "DMG Compatible";
    }
    return info;
}

bool SDManager::begin() {
    SD_MMC.setPins(PIN_SD_CLK, PIN_SD_CMD, PIN_SD_D0);
    if (!SD_MMC.begin("/sdcard", true, false, 20000)) { // 1-bit mode, 20 MHz
        Serial.println("[SD] Failed to mount SD Card via 1-bit SDMMC!");
        mounted_ = false;
        return false;
    }
    mounted_ = true;
    Serial.printf("[SD] SDMMC Card mounted successfully! Card Size: %llu MB\n", SD_MMC.cardSize() / (1024 * 1024));

    if (!SD_MMC.exists("/firmware")) {
        SD_MMC.mkdir("/firmware");
        Serial.println("[SD] Created /firmware directory.");
    }
    if (!SD_MMC.exists("/roms")) {
        SD_MMC.mkdir("/roms");
        Serial.println("[SD] Created /roms directory.");
    }
    if (!SD_MMC.exists("/images")) {
        SD_MMC.mkdir("/images");
        Serial.println("[SD] Created /images directory.");
    }

    return true;
}

const std::vector<FirmwareItem>& SDManager::scan_firmwares(const String &active_app) {
    firmwares_.clear();
    if (!mounted_) return firmwares_;

    File dir = SD_MMC.open("/firmware");
    if (!dir || !dir.isDirectory()) {
        Serial.println("[SD] /firmware is not a valid directory.");
        return firmwares_;
    }

    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String fname = file.name();
            int lastSlash = fname.lastIndexOf('/');
            if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);
            int lastBackslash = fname.lastIndexOf('\\');
            if (lastBackslash >= 0) fname = fname.substring(lastBackslash + 1);

            if (fname.equalsIgnoreCase("launcher.bin") || fname.equalsIgnoreCase("rlcd-launcher.bin")) {
                file = dir.openNextFile();
                continue;
            }

            String lower = fname;
            lower.toLowerCase();
            if (lower.endsWith(".bin") && !fname.startsWith(".")) {
                FirmwareItem item;
                item.filename = fname;
                item.size_bytes = file.size();
                item.is_installed = (active_app.length() > 0 && active_app == fname);
                item.module_type = get_module_type(fname);

                if (item.module_type == "webradio") {
                    item.title = "Web Radio & Markets";
                    item.subtitle = "Radio, SD Music, 5D Weather, Markets, RSS";
                } else if (item.module_type == "gameboy") {
                    item.title = "Game Boy Color & DMG";
                    item.subtitle = "gnuboy GBC, WebPad, BLE, ES8311";
                } else if (item.module_type == "btc-miner") {
                    item.title = "Bitcoin Solo Miner";
                    item.subtitle = "SHA-256D, Stratum, BTC Ticker";
                } else if (item.module_type == "calendar-clock") {
                    item.title = "Calendar & Desk Clock";
                    item.subtitle = "PCF85063, SHTC3, ECO Mode";
                } else if (item.module_type == "pc-monitor") {
                    item.title = "PC Monitor & Cyber-Pet";
                    item.subtitle = "USB/HTTP Telemetry, CPU/RAM/Agent";
                } else if (item.module_type == "trmnl-frame") {
                    item.title = "TRMNL Frame & Noticeboard";
                    item.subtitle = "BYOS Client, 1-Bit BMP Gallery";
                } else if (item.module_type == "flight-air") {
                    item.title = "Flight Radar & Air Quality";
                    item.subtitle = "ATC Radar Sweep, Open-Meteo AQI";
                } else if (item.module_type == "transit-board") {
                    item.title = "myBus R&G Transit Board";
                    item.subtitle = "Starachowice 27-200 Live GPS";
                } else if (item.module_type == "crypto-stocks") {
                    item.title = "NBP & Crypto Market Desk";
                    item.subtitle = "NBP Table A + Crypto/Gold 30D";
                } else {
                    item.title = fname.substring(0, fname.length() - 4);
                    item.subtitle = "Custom Firmware";
                }

                firmwares_.push_back(item);
                Serial.printf("[SD] Found firmware: %s (%s, %u bytes)\n", fname.c_str(), item.title.c_str(), (unsigned)item.size_bytes);
            }
        }
        file = dir.openNextFile();
    }
    dir.close();

    // Sort by custom saved order in NVS ("launcher" / "fw_order")
    Preferences op;
    op.begin("launcher", true);
    String ordCsv = op.getString("fw_order", "");
    op.end();
    if (!ordCsv.isEmpty() && firmwares_.size() > 1) {
        std::vector<String> orderList;
        int start = 0;
        while (start < (int)ordCsv.length()) {
            int comma = ordCsv.indexOf(',', start);
            String tok = (comma >= 0) ? ordCsv.substring(start, comma) : ordCsv.substring(start);
            tok.trim();
            if (!tok.isEmpty()) orderList.push_back(tok);
            if (comma < 0) break;
            start = comma + 1;
        }
        auto get_rank = [&](const String &fn) -> int {
            for (size_t i = 0; i < orderList.size(); ++i) {
                if (orderList[i] == fn) return (int)i;
            }
            return 10000;
        };
        std::stable_sort(firmwares_.begin(), firmwares_.end(), [&](const FirmwareItem &a, const FirmwareItem &b) {
            int ra = get_rank(a.filename);
            int rb = get_rank(b.filename);
            if (ra != rb) return ra < rb;
            return a.filename.compareTo(b.filename) < 0;
        });
    }

    return firmwares_;
}

void SDManager::save_firmware_order(const std::vector<String> &order) {
    String csv;
    for (size_t i = 0; i < order.size(); ++i) {
        if (!order[i].isEmpty()) {
            if (!csv.isEmpty()) csv += ',';
            csv += order[i];
        }
    }
    Preferences op;
    op.begin("launcher", false);
    op.putString("fw_order", csv);
    op.end();
}

const std::vector<RomInfo>& SDManager::scan_roms() {
    roms_.clear();
    if (!mounted_) return roms_;

    File dir = SD_MMC.open("/roms");
    if (!dir || !dir.isDirectory()) {
        Serial.println("[SD] /roms directory not found.");
        return roms_;
    }

    // Read headers into internal DMA-safe SRAM (avoid PSRAM SDMMC DMA issue)
    uint8_t *hdr = (uint8_t*)heap_caps_malloc(0x150, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!hdr) {
        dir.close();
        return roms_;
    }

    File file = dir.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            String fname = file.name();
            int lastSlash = fname.lastIndexOf('/');
            if (lastSlash >= 0) fname = fname.substring(lastSlash + 1);
            int lastBackslash = fname.lastIndexOf('\\');
            if (lastBackslash >= 0) fname = fname.substring(lastBackslash + 1);

            String lower = fname;
            lower.toLowerCase();
            if (!fname.startsWith(".") && (lower.endsWith(".gb") || lower.endsWith(".gbc"))) {
                size_t sz = file.size();
                size_t got = (sz >= 0x150) ? file.read(hdr, 0x150) : 0;
                RomInfo info = validate_rom_header(fname, sz, hdr, got);
                roms_.push_back(info);
                Serial.printf("[SD] ROM: %s (%s, %u B, %s)\n",
                              fname.c_str(), info.internal_title.c_str(),
                              (unsigned)sz, info.supported ? "OK" : info.reason.c_str());
            }
        }
        file = dir.openNextFile();
    }
    heap_caps_free(hdr);
    dir.close();
    return roms_;
}

uint64_t SDManager::get_total_bytes() const {
    return mounted_ ? SD_MMC.totalBytes() : 0;
}

uint64_t SDManager::get_used_bytes() const {
    return mounted_ ? SD_MMC.usedBytes() : 0;
}
