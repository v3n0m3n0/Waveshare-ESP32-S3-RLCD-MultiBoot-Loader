#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <FS.h>
#include "sd_manager.h"

class LauncherWebServer {
public:
    LauncherWebServer();
    bool begin();
    void handle();

    String get_ip() const;
    String get_ssid() const { return active_ssid_; }
    bool is_connected() const { return wifi_connected_; }
    bool consume_menu_dirty();

private:
    void setup_routes();
    void handle_file_upload(bool is_rom);

    WebServer *server_ = nullptr;
    bool wifi_connected_ = false;
    String ip_str_ = "192.168.4.1";
    String active_ssid_ = "";
    volatile bool menu_dirty_ = false;

    // Upload state
    File upload_file_;
    String upload_tmp_;
    String upload_dst_;
    String upload_clean_name_;
    bool upload_ok_ = false;
    bool upload_is_rom_ = false;

    // Deferred actions (allow HTTP response TCP flush before flash/reboot)
    bool pending_reboot_ = false;
    uint32_t pending_action_at_ = 0;
    String pending_boot_file_;
};

extern LauncherWebServer LauncherWeb;
