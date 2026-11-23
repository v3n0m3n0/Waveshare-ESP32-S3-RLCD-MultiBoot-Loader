#pragma once
#include <Arduino.h>
#include <vector>
#include <SD_MMC.h>
#include "board_pins.h"

struct FirmwareItem {
    String filename;      // e.g. "rlcd-webradio.bin"
    String title;         // e.g. "Web Radio & Homelab"
    String subtitle;      // e.g. "Radio Streams, SHTC3, Audio"
    String module_type;   // "webradio", "gameboy", "btc-miner", "calendar-clock", "custom"
    size_t size_bytes;
    bool is_installed;    // currently in flash
};

struct RomInfo {
    String filename;
    String internal_title;
    uint8_t mbc_type;
    uint8_t cgb_flag;
    bool supported;
    String reason;
    size_t size_bytes;
};

class SDManager {
public:
    SDManager();
    bool begin();
    bool is_mounted() const { return mounted_; }

    const std::vector<FirmwareItem>& scan_firmwares(const String &active_app = "");
    const std::vector<RomInfo>& scan_roms();
    const std::vector<FirmwareItem>& get_firmwares() const { return firmwares_; }
    const std::vector<RomInfo>& get_roms() const { return roms_; }

    static String sanitize_filename(const String &raw, const char *required_ext = nullptr);
    static String get_module_type(const String &filename);
    static void save_firmware_order(const std::vector<String> &order);
    static RomInfo validate_rom_header(const String &filename, size_t size_bytes, const uint8_t *hdr, size_t hdr_len);

    uint64_t get_total_bytes() const;
    uint64_t get_used_bytes() const;

private:
    bool mounted_ = false;
    std::vector<FirmwareItem> firmwares_;
    std::vector<RomInfo> roms_;
};

extern SDManager SDCard;
