#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include "board_pins.h"

class LauncherDisplay {
public:
    LauncherDisplay();
    ~LauncherDisplay();

    bool begin();
    void clear();
    void flush();
    void dump_framebuffer();

    void draw_pixel(int x, int y, bool black);
    void draw_string(int x, int y, const char *str, bool black = true, const uint8_t *font = nullptr);
    void draw_rect(int x, int y, int w, int h, bool black = true, bool fill = false);
    void draw_line(int x0, int y0, int x1, int y1, bool black = true);
    void draw_progress_bar(int x, int y, int w, int h, int percent, const char *label = nullptr);
    void draw_battery(int x, int y, int percent);

    u8g2_t* get_u8g2() { return &u8g2_obj_; }

private:
    void init_hardware();
    void send_command(uint8_t cmd);
    void send_data(const uint8_t *data, size_t len);

    u8g2_t u8g2_obj_;
    uint8_t *buffer_ = nullptr;
};

extern LauncherDisplay Display;
