#pragma once
#include <Arduino.h>
#include <functional>
#include <esp_partition.h>
#include <esp_ota_ops.h>

typedef std::function<void(size_t written, size_t total, int percent)> ProgressCallback;

class FirmwareFlasher {
public:
    FirmwareFlasher();

    String get_active_app();
    void set_active_app(const String &name, size_t size = 0);

    bool is_already_flashed(const String &filename);
    bool boot_target(const String &filename);
    bool flash_and_boot(const String &filename, ProgressCallback on_progress = nullptr);

    static void reboot_to_launcher();
};

extern FirmwareFlasher Flasher;
