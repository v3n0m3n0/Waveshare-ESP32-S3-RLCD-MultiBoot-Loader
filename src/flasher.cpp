#include "flasher.h"
#include <SD_MMC.h>
#include <Preferences.h>

FirmwareFlasher Flasher;

FirmwareFlasher::FirmwareFlasher() {}

String FirmwareFlasher::get_active_app() {
    Preferences prefs;
    prefs.begin("launcher", true);
    String app = prefs.getString("active_app", "");
    prefs.end();
    return app;
}

void FirmwareFlasher::set_active_app(const String &name, size_t size) {
    Preferences prefs;
    prefs.begin("launcher", false);
    prefs.putString("active_app", name);
    if (size > 0) {
        prefs.putUInt("active_size", (uint32_t)size);
    }
    prefs.end();
}

bool FirmwareFlasher::is_already_flashed(const String &filename) {
    Preferences prefs;
    prefs.begin("launcher", true);
    String current = prefs.getString("active_app", "");
    uint32_t active_sz = prefs.getUInt("active_size", 0);
    prefs.end();

    if (current.length() == 0 || current != filename) return false;

    // Verify file size on MicroSD card matches the flashed binary
    String path = "/firmware/" + filename;
    if (!path.startsWith("/")) path = "/" + path;
    File f = SD_MMC.open(path, FILE_READ);
    if (!f) return false;
    size_t sz = f.size();
    f.close();

    return (active_sz > 0 && sz == active_sz);
}

bool FirmwareFlasher::boot_target(const String &filename) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    Serial.printf("[FLASHER] Boot target directly: %s (Running: %s, Target: %s)\n", 
                  filename.c_str(), running ? running->label : "?", target ? target->label : "?");

    if (!target) {
        Serial.println("[FLASHER] Error: Target partition (OTA_1) not found!");
        return false;
    }

    set_active_app(filename);
    esp_err_t err = esp_ota_set_boot_partition(target);
    if (err != ESP_OK) {
        Serial.printf("[FLASHER] Failed to set boot partition: 0x%x\n", err);
        return false;
    }

    delay(200);
    ESP.restart();
    return true;
}

bool FirmwareFlasher::flash_and_boot(const String &filename, ProgressCallback on_progress) {
    String path = "/firmware/" + filename;
    if (!path.startsWith("/")) path = "/" + path;

    File f = SD_MMC.open(path, FILE_READ);
    if (!f) {
        Serial.printf("[FLASHER] Cannot open file: %s\n", path.c_str());
        return false;
    }

    size_t fileSize = f.size();
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *target = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);

    Serial.printf("[FLASHER] Flashing %s (%u bytes)...\n", path.c_str(), (unsigned)fileSize);
    Serial.printf("[FLASHER] Running on: %s, Target partition: %s\n",
                  running ? running->label : "?", target ? target->label : "?");

    if (!target) {
        Serial.println("[FLASHER] Error: Target partition (OTA_1) not found!");
        f.close();
        return false;
    }

    if (fileSize > target->size) {
        Serial.printf("[FLASHER] Error: Binary too large! Size: %u, Partition: %u\n", (unsigned)fileSize, (unsigned)target->size);
        f.close();
        return false;
    }

    esp_ota_handle_t ota_handle = 0;
    esp_err_t err = esp_ota_begin(target, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        Serial.printf("[FLASHER] esp_ota_begin failed: 0x%x\n", err);
        f.close();
        return false;
    }

    uint8_t buffer[4096];
    size_t written = 0;
    int last_pct = -1;

    while (f.available()) {
        size_t to_read = f.read(buffer, sizeof(buffer));
        if (to_read == 0) break;

        err = esp_ota_write(ota_handle, buffer, to_read);
        if (err != ESP_OK) {
            Serial.printf("[FLASHER] esp_ota_write failed: 0x%x\n", err);
            esp_ota_abort(ota_handle);
            f.close();
            return false;
        }

        written += to_read;
        int pct = (int)((written * 100ULL) / fileSize);
        if (pct != last_pct) {
            last_pct = pct;
            if (on_progress) {
                on_progress(written, fileSize, pct);
            }
        }
    }
    f.close();

    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        Serial.printf("[FLASHER] esp_ota_end failed: 0x%x\n", err);
        return false;
    }

    err = esp_ota_set_boot_partition(target);
    if (err != ESP_OK) {
        Serial.printf("[FLASHER] esp_ota_set_boot_partition failed: 0x%x\n", err);
        return false;
    }

    set_active_app(filename, fileSize);
    Serial.printf("[FLASHER] Successfully flashed %s! Rebooting into target firmware...\n", filename.c_str());
    delay(400);
    ESP.restart();
    return true;
}

void FirmwareFlasher::reboot_to_launcher() {
    const esp_partition_t *launcher = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (launcher) {
        esp_ota_set_boot_partition(launcher);
        delay(200);
        ESP.restart();
    }
}
