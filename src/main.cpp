#include <Arduino.h>
#include <SD_MMC.h>
#include <Preferences.h>
#include <esp_heap_caps.h>
#include "board_pins.h"
#include "display.h"
#include "sd_manager.h"
#include "flasher.h"
#include "web_server.h"

// Battery measurement helper (Waveshare ESP32-S3-RLCD-4.2 uses 3:1 ADC divider on GPIO 4)
static float read_battery_voltage() {
    uint32_t sum_mv = 0;
    for (int i = 0; i < 8; ++i) {
        sum_mv += analogReadMilliVolts(PIN_BATTERY_ADC);
        delayMicroseconds(50);
    }
    return ((float)sum_mv / 8.0f) * 3.0f / 1000.0f;
}

static int get_battery_percent(float v) {
    if (v >= 4.15f) return 100;
    if (v <= 3.30f) return 0;
    return (int)((v - 3.30f) / (4.15f - 3.30f) * 100.0f);
}

void get_battery_status(float &voltage, int &percent) {
    voltage = read_battery_voltage();
    percent = get_battery_percent(voltage);
}

static int selected_idx = 0;
static int scroll_offset = 0;
static const int VISIBLE_ITEMS = 6;
static bool menu_active = false;
static uint32_t last_ui_render = 0;
static std::vector<FirmwareItem> cached_fws;
static bool key_was_pressed = false;
static bool boot_was_pressed = false;

void draw_boot_splash(int pct, const char *custom_banner = nullptr, const char *bottom_hint = nullptr) {
    Display.clear();
    u8g2_t *u8 = Display.get_u8g2();
    u8g2_SetDrawColor(u8, 1);

    // Outer technical double-bezel with corner brackets
    u8g2_DrawFrame(u8, 4, 4, RLCD_WIDTH - 8, RLCD_HEIGHT - 8);
    u8g2_DrawFrame(u8, 7, 7, RLCD_WIDTH - 14, RLCD_HEIGHT - 14);
    u8g2_DrawBox(u8, 7, 7, 14, 4);
    u8g2_DrawBox(u8, 7, 7, 4, 14);
    u8g2_DrawBox(u8, RLCD_WIDTH - 21, 7, 14, 4);
    u8g2_DrawBox(u8, RLCD_WIDTH - 11, 7, 4, 14);
    u8g2_DrawBox(u8, 7, RLCD_HEIGHT - 11, 14, 4);
    u8g2_DrawBox(u8, 7, RLCD_HEIGHT - 21, 4, 14);
    u8g2_DrawBox(u8, RLCD_WIDTH - 21, RLCD_HEIGHT - 11, 14, 4);
    u8g2_DrawBox(u8, RLCD_WIDTH - 11, RLCD_HEIGHT - 21, 4, 14);

    // Central Graphic Emblem: ESP32-S3 Silicon Package & RLCD Display Crest (center x=200, y=88)
    const int cx = 200, cy = 88;
    // Outer display tablet silhouette
    u8g2_DrawRFrame(u8, cx - 76, cy - 52, 152, 104, 8);
    u8g2_DrawRFrame(u8, cx - 72, cy - 48, 144, 96, 6);

    // CPU IC Die at center with pins on all 4 sides
    u8g2_DrawBox(u8, cx - 28, cy - 28, 56, 56);
    u8g2_SetDrawColor(u8, 0);
    u8g2_DrawFrame(u8, cx - 23, cy - 23, 46, 46);
    u8g2_SetFont(u8, u8g2_font_helvB12_tr);
    u8g2_DrawStr(u8, cx - 15, cy - 2, "S3");
    u8g2_SetFont(u8, u8g2_font_5x8_tr);
    u8g2_DrawStr(u8, cx - 17, cy + 12, "RLCD");
    u8g2_SetDrawColor(u8, 1);

    for (int p = -18; p <= 18; p += 9) {
        u8g2_DrawBox(u8, cx + p - 2, cy - 36, 4, 7); // Top pins
        u8g2_DrawBox(u8, cx + p - 2, cy + 29, 4, 7); // Bottom pins
        u8g2_DrawBox(u8, cx - 36, cy + p - 2, 7, 4); // Left pins
        u8g2_DrawBox(u8, cx + 29, cy + p - 2, 7, 4); // Right pins
    }

    // Left & Right circuit traces inside tablet silhouette
    u8g2_DrawLine(u8, cx - 62, cy - 20, cx - 42, cy - 20);
    u8g2_DrawDisc(u8, cx - 62, cy - 20, 2, U8G2_DRAW_ALL);
    u8g2_DrawLine(u8, cx - 64, cy,      cx - 42, cy);
    u8g2_DrawDisc(u8, cx - 64, cy,      2, U8G2_DRAW_ALL);
    u8g2_DrawLine(u8, cx - 62, cy + 20, cx - 42, cy + 20);
    u8g2_DrawDisc(u8, cx - 62, cy + 20, 2, U8G2_DRAW_ALL);

    u8g2_DrawLine(u8, cx + 42, cy - 20, cx + 62, cy - 20);
    u8g2_DrawDisc(u8, cx + 62, cy - 20, 2, U8G2_DRAW_ALL);
    u8g2_DrawLine(u8, cx + 42, cy,      cx + 64, cy);
    u8g2_DrawDisc(u8, cx + 64, cy,      2, U8G2_DRAW_ALL);
    u8g2_DrawLine(u8, cx + 42, cy + 20, cx + 62, cy + 20);
    u8g2_DrawDisc(u8, cx + 62, cy + 20, 2, U8G2_DRAW_ALL);

    // Title & Dynamic Module/OS Typography
    u8g2_SetFont(u8, u8g2_font_helvB14_tr);
    const char *t1 = "WAVESHARE ESP32-S3 RLCD 4.2\"";
    u8g2_DrawStr(u8, (RLCD_WIDTH - u8g2_GetStrWidth(u8, t1)) / 2, 168, t1);

    char banner_buf[48] = "MULTIBOOT OS & APPLICATION SUITE v5.0";
    if (custom_banner && custom_banner[0] != '\0') {
        size_t i = 0;
        for (; i < sizeof(banner_buf) - 1 && custom_banner[i] != '\0'; ++i) {
            banner_buf[i] = (char)toupper((uint8_t)custom_banner[i]);
        }
        banner_buf[i] = '\0';
    }
    u8g2_SetFont(u8, u8g2_font_helvB08_tr);
    int tw = u8g2_GetStrWidth(u8, banner_buf);
    int box_w = constrain(tw + 28, 200, 368);
    int box_x = (RLCD_WIDTH - box_w) / 2;
    u8g2_DrawBox(u8, box_x, 177, box_w, 18);
    u8g2_SetDrawColor(u8, 0);
    u8g2_DrawStr(u8, (RLCD_WIDTH - tw) / 2, 190, banner_buf);
    u8g2_SetDrawColor(u8, 1);

    // Hardware Spec Badges
    u8g2_SetFont(u8, u8g2_font_6x10_tr);
    const char *specs = "[240MHz Dual-Core]  [8MB PSRAM]  [16MB Flash]  [ST7305]";
    u8g2_DrawStr(u8, (RLCD_WIDTH - u8g2_GetStrWidth(u8, specs)) / 2, 216, specs);

    // Sleek Graphical Boot Progress Bar
    int bx = 50, by = 238, bw = 300, bh = 16;
    u8g2_DrawRFrame(u8, bx, by, bw, bh, 4);
    int fillW = (constrain(pct, 0, 100) * (bw - 6)) / 100;
    if (fillW > 0) {
        u8g2_DrawBox(u8, bx + 3, by + 3, fillW, bh - 6);
    }

    if (bottom_hint && bottom_hint[0] != '\0') {
        u8g2_SetFont(u8, u8g2_font_6x10_tr);
        int hw = u8g2_GetStrWidth(u8, bottom_hint);
        u8g2_DrawStr(u8, (RLCD_WIDTH - hw) / 2, 273, bottom_hint);
    } else {
        // Segmented progress dots below bar
        for (int s = 0; s < 5; ++s) {
            int dx = 160 + s * 20;
            if (pct >= (s + 1) * 20) {
                u8g2_DrawDisc(u8, dx, 268, 3, U8G2_DRAW_ALL);
            } else {
                u8g2_DrawCircle(u8, dx, 268, 3, U8G2_DRAW_ALL);
            }
        }
    }

    Display.flush();
}

void render_menu(const std::vector<FirmwareItem> &fws, int countdown_sec = -1) {
    Display.clear();
    u8g2_t *u8 = Display.get_u8g2();

    // 1. Compact Header Bar (y = 0..23)
    Display.draw_rect(0, 0, RLCD_WIDTH, 24, true, true);
    Display.draw_string(8, 17, "RLCD MULTIBOOT LAUNCHER", false, u8g2_font_helvB10_tr);

    // Battery (Negative double-outlined icon on black bar)
    float vbat = 0.0f;
    int bat_pct = 0;
    get_battery_status(vbat, bat_pct);
    Display.draw_battery(RLCD_WIDTH - 30, 6, bat_pct);

    char bat_str[20];
    snprintf(bat_str, sizeof(bat_str), "%.2fV %d%%", vbat, bat_pct);
    u8g2_SetFont(u8, u8g2_font_helvR08_tr);
    int bat_w = u8g2_GetStrWidth(u8, bat_str);
    Display.draw_string(RLCD_WIDTH - 36 - bat_w, 16, bat_str, false, u8g2_font_helvR08_tr);

    // 2. Compact Subheader with WiFi & SD Info (y = 24..43)
    Display.draw_rect(0, 24, RLCD_WIDTH, 19, true, false);
    String ip_info = "Web Hub: http://" + LauncherWeb.get_ip() + "/";
    Display.draw_string(8, 37, ip_info.c_str(), true, u8g2_font_helvR08_tr);

    String sd_info = "SD: " + String((unsigned)(SDCard.get_used_bytes() / (1024 * 1024))) + "/"
                   + String((unsigned)(SDCard.get_total_bytes() / (1024 * 1024))) + " MB";
    u8g2_SetFont(u8, u8g2_font_helvR08_tr);
    int sd_w = u8g2_GetStrWidth(u8, sd_info.c_str());
    Display.draw_string(RLCD_WIDTH - 8 - sd_w, 37, sd_info.c_str(), true, u8g2_font_helvR08_tr);

    // 3. Firmware List (6 visible rows at once: y = 46..273, item_h = 38)
    int y_start = 46;
    int total = (int)fws.size();
    if (total == 0) {
        selected_idx = 0;
        scroll_offset = 0;
        Display.draw_rect(20, 80, RLCD_WIDTH - 40, 120, true, false);
        Display.draw_string(40, 110, "No firmware files found in /firmware/ !", true, u8g2_font_helvB12_tr);
        Display.draw_string(40, 135, "1. Upload .bin files via Web Manager:", true, u8g2_font_helvR10_tr);
        Display.draw_string(55, 155, ("http://" + LauncherWeb.get_ip() + "/").c_str(), true, u8g2_font_helvB10_tr);
        Display.draw_string(40, 180, "2. Or copy them directly to MicroSD card.", true, u8g2_font_helvR10_tr);
    } else {
        if (selected_idx < 0) selected_idx = 0;
        if (selected_idx >= total) selected_idx = total - 1;
        if (selected_idx < scroll_offset) scroll_offset = selected_idx;
        if (selected_idx >= scroll_offset + VISIBLE_ITEMS) scroll_offset = selected_idx - VISIBLE_ITEMS + 1;
        if (scroll_offset < 0) scroll_offset = 0;

        const int item_h = 38;
        const int card_w = (total > VISIBLE_ITEMS) ? (RLCD_WIDTH - 22) : (RLCD_WIDTH - 12);

        for (int slot = 0; slot < VISIBLE_ITEMS && (scroll_offset + slot) < total; ++slot) {
            int i = scroll_offset + slot;
            int y = y_start + slot * item_h;
            bool is_sel = (i == selected_idx);

            char title_line[48];
            snprintf(title_line, sizeof(title_line), "%d. %s", i + 1, fws[i].title.c_str());

            if (is_sel) {
                // High-contrast inverted card for selected item
                Display.draw_rect(6, y, card_w, item_h - 3, true, true);
                Display.draw_string(12, y + 16, ">", false, u8g2_font_helvB10_tr);
                Display.draw_string(24, y + 15, title_line, false, u8g2_font_helvB10_tr);
                Display.draw_string(24, y + 29, fws[i].subtitle.c_str(), false, u8g2_font_helvR08_tr);

                if (fws[i].is_installed) {
                    Display.draw_rect(6 + card_w - 82, y + 7, 74, 18, false, true);
                    Display.draw_string(6 + card_w - 76, y + 20, "INSTALLED", true, u8g2_font_helvB08_tr);
                } else {
                    char sz_str[24];
                    snprintf(sz_str, sizeof(sz_str), "%.1f MB", (float)fws[i].size_bytes / (1024.0f * 1024.0f));
                    Display.draw_string(6 + card_w - 52, y + 21, sz_str, false, u8g2_font_helvR08_tr);
                }
            } else {
                // Normal framed card
                Display.draw_rect(6, y, card_w, item_h - 3, true, false);
                Display.draw_string(24, y + 15, title_line, true, u8g2_font_helvB10_tr);
                Display.draw_string(24, y + 29, fws[i].subtitle.c_str(), true, u8g2_font_helvR08_tr);

                if (fws[i].is_installed) {
                    Display.draw_rect(6 + card_w - 82, y + 7, 74, 18, true, true);
                    Display.draw_string(6 + card_w - 76, y + 20, "INSTALLED", false, u8g2_font_helvB08_tr);
                } else {
                    char sz_str[24];
                    snprintf(sz_str, sizeof(sz_str), "%.1f MB", (float)fws[i].size_bytes / (1024.0f * 1024.0f));
                    Display.draw_string(6 + card_w - 52, y + 21, sz_str, true, u8g2_font_helvR08_tr);
                }
            }
        }

        // Scrollbar track on far right when total > VISIBLE_ITEMS
        if (total > VISIBLE_ITEMS) {
            int sb_x = RLCD_WIDTH - 11;
            int sb_y = y_start;
            int sb_h = VISIBLE_ITEMS * item_h - 3;
            Display.draw_rect(sb_x, sb_y, 6, sb_h, true, false);
            int thumb_h = max(18, (sb_h * VISIBLE_ITEMS) / total);
            int max_off = total - VISIBLE_ITEMS;
            int thumb_y = sb_y + ((sb_h - thumb_h) * scroll_offset) / max(1, max_off);
            Display.draw_rect(sb_x + 1, thumb_y, 4, thumb_h, true, true);
        }
    }

    // 4. Footer navigation hint + page indicator (y = 278..300)
    Display.draw_rect(0, RLCD_HEIGHT - 22, RLCD_WIDTH, 22, true, true);
    Display.draw_string(10, RLCD_HEIGHT - 7, "[KEY] Next Module   |   [BOOT] Select & Launch", false, u8g2_font_helvR08_tr);
    if (total > 0) {
        char page_buf[24];
        snprintf(page_buf, sizeof(page_buf), "APP %d/%d %s%s",
                 selected_idx + 1, total,
                 (scroll_offset > 0) ? "^" : "",
                 (scroll_offset + VISIBLE_ITEMS < total) ? "v" : "");
        u8g2_SetFont(u8, u8g2_font_helvB08_tr);
        int pw = u8g2_GetStrWidth(u8, page_buf);
        Display.draw_string(RLCD_WIDTH - 10 - pw, RLCD_HEIGHT - 7, page_buf, false, u8g2_font_helvB08_tr);
    }

    Display.flush();
}

void launch_firmware_idx(int idx) {
    if (cached_fws.empty() || idx < 0 || idx >= (int)cached_fws.size()) {
        Serial.printf("[LAUNCHER] Invalid selection index: %d\n", idx);
        return;
    }

    const auto &chosen = cached_fws[idx];
    Serial.printf("[LAUNCHER] Launching: %s (%s)\n", chosen.filename.c_str(), chosen.title.c_str());

    // If already flashed, boot instantly using uniform Splash Screen
    if (Flasher.is_already_flashed(chosen.filename)) {
        draw_boot_splash(100, chosen.title.c_str(), "STARTING MODULE...");
        Serial.println("[LAUNCHER] Already installed in Flash -> Instant Boot!");
        delay(300);
        Flasher.boot_target(chosen.filename);
    } else {
        // Flash from SD card using uniform Splash Screen with module name & progress
        draw_boot_splash(0, chosen.title.c_str(), "FLASHING TO MEMORY: 0%");
        Serial.printf("[LAUNCHER] Flashing from SD card: %s...\n", chosen.filename.c_str());

        bool ok = Flasher.flash_and_boot(chosen.filename, [&](size_t written, size_t total, int pct) {
            static int last_drawn_pct = -1;
            if (pct != last_drawn_pct && (pct % 5 == 0 || pct == 100)) {
                last_drawn_pct = pct;
                char msg[40];
                snprintf(msg, sizeof(msg), "FLASHING TO MEMORY: %d%%", pct);
                draw_boot_splash(pct, chosen.title.c_str(), msg);
                Serial.printf("[FLASH] Progress: %d%% (%u / %u bytes)\n", pct, (unsigned)written, (unsigned)total);
            }
        });
        if (!ok) {
            Display.clear();
            Display.draw_string(30, 120, "Flashing Error!", true, u8g2_font_helvB14_tr);
            Display.draw_string(30, 150, "Partition or file is invalid.", true, u8g2_font_helvR10_tr);
            Display.flush();
            Serial.println("[LAUNCHER] Flashing Failed!");
            delay(2500);
            render_menu(cached_fws);
        }
    }
}

void handle_serial_commands() {
    if (!Serial.available()) return;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) return;

    Serial.printf("[COMMAND] %s\n", line.c_str());

    if (line == "HELP") {
        Serial.println("MultiBoot Launcher Commands:");
        Serial.println("  STATUS          - Battery, Wi-Fi IP, SD Card status, Active App");
        Serial.println("  LIST            - List available firmware images on SD card");
        Serial.println("  NEXT            - Select next firmware in menu");
        Serial.println("  BOOT            - Launch currently selected firmware");
        Serial.println("  BOOT <name|idx> - Launch specific firmware by name or index");
        Serial.println("  SCAN            - Rescan SD card /firmware/ directory");
        Serial.println("  SCREENSHOT      - Dump current 400x300 RLCD framebuffer as PBM");
    } else if (line == "SCREENSHOT") {
        Display.dump_framebuffer();
    } else if (line == "STATUS") {
        float v = read_battery_voltage();
        int pct = get_battery_percent(v);
        Serial.printf("Status: LAUNCHER_READY, Battery: %.2f V (%d%%), IP: %s, ActiveApp: %s\n",
                      v, pct, LauncherWeb.get_ip().c_str(), Flasher.get_active_app().c_str());
        Serial.printf("SD Card: %llu MB used / %llu MB total\n",
                      SDCard.get_used_bytes() / (1024 * 1024),
                      SDCard.get_total_bytes() / (1024 * 1024));
        if (!cached_fws.empty() && selected_idx < (int)cached_fws.size()) {
            Serial.printf("Selection: [%d] %s (%s)\n",
                          selected_idx, cached_fws[selected_idx].filename.c_str(),
                          cached_fws[selected_idx].title.c_str());
        }
    } else if (line == "LIST") {
        Serial.printf("[FIRMWARES] Total: %u\n", (unsigned)cached_fws.size());
        for (size_t i = 0; i < cached_fws.size(); ++i) {
            Serial.printf("  [%d] %s (%s, %u bytes) %s%s\n",
                          (int)i, cached_fws[i].filename.c_str(), cached_fws[i].title.c_str(),
                          (unsigned)cached_fws[i].size_bytes,
                          cached_fws[i].is_installed ? "[INSTALLED] " : "",
                          (int)i == selected_idx ? "<-- SELECTED" : "");
        }
    } else if (line == "NEXT") {
        if (!cached_fws.empty()) {
            selected_idx = (selected_idx + 1) % cached_fws.size();
            render_menu(cached_fws);
            Serial.printf("[LAUNCHER] Selected [%d]: %s (%s)\n",
                          selected_idx, cached_fws[selected_idx].filename.c_str(),
                          cached_fws[selected_idx].title.c_str());
        }
    } else if (line == "BOOT") {
        launch_firmware_idx(selected_idx);
    } else if (line.startsWith("BOOT ")) {
        String target = line.substring(5);
        target.trim();
        int idx = -1;
        if (target.length() == 1 && isDigit(target.charAt(0))) {
            idx = target.toInt();
        } else {
            for (size_t i = 0; i < cached_fws.size(); ++i) {
                if (cached_fws[i].filename.indexOf(target) >= 0) {
                    idx = (int)i;
                    break;
                }
            }
        }
        if (idx >= 0 && idx < (int)cached_fws.size()) {
            selected_idx = idx;
            launch_firmware_idx(idx);
        } else {
            Serial.printf("[ERROR] Target firmware '%s' not found!\n", target.c_str());
        }
    } else if (line == "SCAN") {
        String active_app = Flasher.get_active_app();
        cached_fws = SDCard.scan_firmwares(active_app);
        SDCard.scan_roms();
        selected_idx = 0;
        render_menu(cached_fws);
        Serial.printf("[OK] Found %u firmware files.\n", (unsigned)cached_fws.size());
    } else if (line.startsWith("SDPUT ") || line.startsWith("SDDEL ")) {
        auto get_sd_dir = [](const String &p) -> const char * {
            if (p.indexOf("..") >= 0 || p.indexOf('\\') >= 0) return nullptr;
            const char *dir = nullptr;
            String base;
            if (p.startsWith("/firmware/"))      { dir = "/firmware"; base = p.substring(10); }
            else if (p.startsWith("/roms/"))     { dir = "/roms";     base = p.substring(6);  }
            else if (p.startsWith("/images/"))   { dir = "/images";   base = p.substring(8);  }
            else if (p.startsWith("/music/"))    { dir = "/music";    base = p.substring(7);  }
            else return nullptr;
            if (base.isEmpty() || base.startsWith(".") || base.indexOf('/') >= 0) return nullptr;
            return dir;
        };

        if (line.startsWith("SDDEL ")) {
            String p = line.substring(6);
            p.trim();
            if (!get_sd_dir(p) || !SD_MMC.exists(p) || !SD_MMC.remove(p)) {
                Serial.println("SDDEL_ERR");
            } else {
                cached_fws = SDCard.scan_firmwares(Flasher.get_active_app());
                SDCard.scan_roms();
                if (!cached_fws.empty() && selected_idx >= (int)cached_fws.size()) {
                    selected_idx = (int)cached_fws.size() - 1;
                }
                render_menu(cached_fws);
                Serial.println("SDDEL_OK");
            }
            return;
        }

        auto crc32_update = [](uint32_t crc, const uint8_t *data, size_t len) -> uint32_t {
            for (size_t i = 0; i < len; ++i) {
                crc ^= data[i];
                for (int b = 0; b < 8; ++b) {
                    crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320UL) : (crc >> 1);
                }
            }
            return crc;
        };

        String rest = line.substring(6);
        rest.trim();
        int sp = rest.lastIndexOf(' ');
        if (sp <= 0) {
            Serial.println("SDPUT_ERR bad_args");
            return;
        }
        String dst = rest.substring(0, sp);
        dst.trim();
        size_t total = (size_t)rest.substring(sp + 1).toInt();
        const char *dir = get_sd_dir(dst);
        if (!SDCard.is_mounted() || !dir || total == 0 || total > 8 * 1024 * 1024) {
            Serial.println("SDPUT_ERR invalid_path_or_size");
            return;
        }

        if (!SD_MMC.exists(dir)) SD_MMC.mkdir(dir);
        String tmp = String(dir) + "/.usb.tmp";
        if (SD_MMC.exists(tmp)) SD_MMC.remove(tmp);

        const size_t CHUNK = 4096;
        uint8_t *buf = (uint8_t *)heap_caps_malloc(CHUNK, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!buf) {
            Serial.println("SDPUT_ERR no_mem");
            return;
        }
        File f = SD_MMC.open(tmp, FILE_WRITE);
        if (!f) {
            heap_caps_free(buf);
            Serial.println("SDPUT_ERR open_fail");
            return;
        }

        Serial.println("SDPUT_READY");
        Serial.flush();

        size_t written = 0;
        uint32_t crc = 0xFFFFFFFFUL;
        bool ok = true;
        String err_reason = "";

        while (written < total) {
            size_t want = min(CHUNK, total - written);
            size_t got = 0;
            uint32_t last_rx = millis();
            while (got < want) {
                int avail = Serial.available();
                if (avail > 0) {
                    size_t n = Serial.read(buf + got, min((size_t)avail, want - got));
                    got += n;
                    last_rx = millis();
                } else if (millis() - last_rx > 5000) {
                    ok = false;
                    err_reason = "timeout";
                    break;
                } else {
                    delay(1);
                }
            }
            if (!ok) {
                err_reason += String("_") + String((unsigned)(written + got));
                break;
            }
            if (f.write(buf, got) != got) {
                ok = false;
                err_reason = "sd_write";
                break;
            }
            crc = crc32_update(crc, buf, got);
            written += got;
            Serial.printf("ACK %u\n", (unsigned)written);
            Serial.flush();
        }

        f.close();
        heap_caps_free(buf);

        if (!ok || written != total) {
            SD_MMC.remove(tmp);
            Serial.printf("SDPUT_ERR %s\n", err_reason.isEmpty() ? "incomplete" : err_reason.c_str());
        } else {
            if (SD_MMC.exists(dst)) SD_MMC.remove(dst);
            if (!SD_MMC.rename(tmp, dst)) {
                SD_MMC.remove(tmp);
                Serial.println("SDPUT_ERR rename_fail");
            } else {
                cached_fws = SDCard.scan_firmwares(Flasher.get_active_app());
                SDCard.scan_roms();
                render_menu(cached_fws);
                Serial.printf("SDPUT_OK %u %08X\n", (unsigned)written, (unsigned)(crc ^ 0xFFFFFFFFUL));
            }
        }
    } else if (line.startsWith("WIFI ")) {
        String rest = line.substring(5);
        int sep = rest.indexOf('|');
        if (sep < 0) sep = rest.indexOf('\t');
        String ssid = (sep >= 0) ? rest.substring(0, sep) : rest;
        String pass = (sep >= 0) ? rest.substring(sep + 1) : "";
        ssid.trim();
        pass.trim();
        if (ssid.isEmpty()) {
            Serial.println("WIFI_ERR empty_ssid");
        } else {
            const char *ns[] = {"wifi", "radio"};
            for (int i = 0; i < 2; i++) {
                Preferences pr;
                pr.begin(ns[i], false);
                pr.putString("ssid", ssid);
                if (!pass.isEmpty()) pr.putString("pass", pass);
                pr.end();
            }
            Serial.printf("WIFI_SAVED %s\n", ssid.c_str());
        }
    }
}

void setup() {
    Serial.setRxBufferSize(16384);
    Serial.begin(115200);
    uint32_t cdc_t0 = millis();
    while (!Serial && (millis() - cdc_t0 < 1200)) {
        delay(10);
    }

    Serial.println("\n========================================");
    Serial.println("   Waveshare ESP32-S3 RLCD MultiBoot   ");
    Serial.println("========================================");

    // Mute Class-D Audio amplifier to save battery
    pinMode(PIN_PA_ENABLE, OUTPUT);
    digitalWrite(PIN_PA_ENABLE, LOW);

    // Init navigation buttons
    pinMode(PIN_BTN_BOOT, INPUT_PULLUP);
    pinMode(PIN_BTN_KEY, INPUT_PULLUP);
    analogReadResolution(12);

    // 1. Init RLCD & Graphical Boot Splash Screen
    Display.begin();
    draw_boot_splash(25);

    // 2. Init SD Card
    SDCard.begin();
    draw_boot_splash(60);

    // 3. Init WiFi and Web Server
    LauncherWeb.begin();
    draw_boot_splash(100);
    delay(350);

    String active_app = Flasher.get_active_app();
    cached_fws = SDCard.scan_firmwares(active_app);
    SDCard.scan_roms();

    // Enforce that the MultiBoot Launcher is always the active boot partition on any future reset
    const esp_partition_t *launcher = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (launcher) {
        esp_ota_set_boot_partition(launcher);
    }

    // Optional Auto-Boot of last selected module during Splash Screen (interruptible with [KEY] or Serial)
    {
        Preferences lp;
        lp.begin("launcher", true);
        bool autoboot = lp.getBool("autoboot", false);
        int ab_sec = constrain(lp.getInt("ab_sec", 3), 2, 10);
        lp.end();

        if (autoboot && !active_app.isEmpty() && !cached_fws.empty()) {
            int target_idx = -1;
            for (size_t i = 0; i < cached_fws.size(); ++i) {
                if (cached_fws[i].filename == active_app) {
                    target_idx = (int)i;
                    break;
                }
            }
            if (target_idx >= 0) {
                selected_idx = target_idx;
                bool aborted = false;
                uint32_t ab_start = millis();
                uint32_t ab_dur = (uint32_t)ab_sec * 1000UL;
                int last_s = -1;
                while ((millis() - ab_start) < ab_dur) {
                    if (digitalRead(PIN_BTN_KEY) == LOW || Serial.available() > 0) {
                        aborted = true;
                        break;
                    }
                    uint32_t rem_ms = ab_dur - (millis() - ab_start);
                    int rem_s = (int)((rem_ms + 999UL) / 1000UL);
                    if (rem_s != last_s) {
                        last_s = rem_s;
                        int pct = 100 - (int)((rem_ms * 100UL) / ab_dur);
                        char hint[48];
                        snprintf(hint, sizeof(hint), "AUTO-BOOT IN %ds  |  [KEY] MENU", rem_s);
                        draw_boot_splash(pct, cached_fws[target_idx].title.c_str(), hint);
                    }
                    delay(25);
                }
                if (!aborted) {
                    Serial.printf("[LAUNCHER] Auto-Booting '%s'...\n", active_app.c_str());
                    launch_firmware_idx(target_idx);
                } else {
                    Serial.println("[LAUNCHER] Auto-Boot interrupted by [KEY]/Serial -> Menu");
                    draw_boot_splash(100, cached_fws[target_idx].title.c_str(), "MENU REQUESTED...");
                    while (digitalRead(PIN_BTN_KEY) == LOW) delay(20);
                }
            }
        }
    }

    menu_active = true;
    render_menu(cached_fws);
    Serial.println("[OK] MultiBoot Launcher ready!");
}

void loop() {
    LauncherWeb.handle();
    handle_serial_commands();

    if (LauncherWeb.consume_menu_dirty()) {
        cached_fws = SDCard.scan_firmwares(Flasher.get_active_app());
        SDCard.scan_roms();
        if (!cached_fws.empty() && selected_idx >= (int)cached_fws.size()) {
            selected_idx = (int)cached_fws.size() - 1;
        } else if (cached_fws.empty()) {
            selected_idx = 0;
        }
        render_menu(cached_fws);
    }

    // 1. KEY button: Immediate edge detection -> Next firmware
    bool key_now = (digitalRead(PIN_BTN_KEY) == LOW);
    if (key_now && !key_was_pressed) {
        key_was_pressed = true;
        Serial.println("[INPUT] KEY button pressed -> Next firmware");
        if (!cached_fws.empty()) {
            selected_idx = (selected_idx + 1) % cached_fws.size();
            render_menu(cached_fws);
        }
    } else if (!key_now && key_was_pressed) {
        key_was_pressed = false;
    }

    // 2. BOOT button: Immediate edge detection -> Launch / Flash
    bool boot_now = (digitalRead(PIN_BTN_BOOT) == LOW);
    if (boot_now && !boot_was_pressed) {
        boot_was_pressed = true;
        Serial.println("[INPUT] BOOT button pressed -> Select & Launch");
        launch_firmware_idx(selected_idx);
    } else if (!boot_now && boot_was_pressed) {
        boot_was_pressed = false;
    }

    // Periodic UI refresh (every 8 seconds for Battery & IP)
    if (millis() - last_ui_render > 8000) {
        last_ui_render = millis();
        render_menu(cached_fws);
    }

    delay(10);
}
