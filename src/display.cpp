#include "display.h"
#include <SPI.h>

#define SPI_CLK 24000000
#define ST7305_TILE_WIDTH 38
#define ST7305_TILE_HEIGHT 50
#define ST7305_BUFFER_ROW_BYTES (ST7305_TILE_WIDTH * 8)

static u8x8_display_info_t st7305_display_info = {
    0, 1, 0, 0, 20, 50, 0, 0, SPI_CLK, 0, 4, 0, 0,
    ST7305_TILE_WIDTH, ST7305_TILE_HEIGHT,
    0, 0, 300, 400
};

static uint8_t u8x8_byte_custom(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    return 1;
}

static uint8_t u8x8_d_st7305_custom(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch (msg) {
        case U8X8_MSG_DISPLAY_SETUP_MEMORY:
            u8x8_d_helper_display_setup_memory(u8x8, &st7305_display_info);
            break;
        case U8X8_MSG_DISPLAY_INIT:
            break;
        case U8X8_MSG_DISPLAY_SET_POWER_SAVE:
            break;
        case U8X8_MSG_DISPLAY_DRAW_TILE: {
            u8x8_tile_t *tile = ((u8x8_tile_t *)arg_ptr);
            uint8_t cnt = tile->cnt;
            uint8_t y_pos = tile->y_pos;
            uint8_t x_pos = tile->x_pos;

            int first_col = x_pos * 8;
            int last_col = (x_pos + cnt) * 8 - 1;
            if (last_col >= 300) last_col = 299;

            int addr_start = 0x12 + first_col / 12;
            int addr_end = 0x12 + last_col / 12;
            int send_start = (addr_start - 0x12) * 3;
            int send_cnt = (addr_end - addr_start + 1) * 3;

            int addr_first_col = (addr_start - 0x12) * 12;
            int addr_last_col = (addr_end - 0x12) * 12 + 11;
            if (addr_last_col >= 300) addr_last_col = 299;

            uint8_t *row_base = tile->tile_ptr - ((uint16_t)x_pos * 8U);

            uint8_t col_bounds[] = {(uint8_t)(0x3C - addr_end), (uint8_t)(0x3C - addr_start)};
            digitalWrite(PIN_RLCD_DC, LOW);
            digitalWrite(PIN_RLCD_CS, LOW);
            SPI.transfer(0x2A);
            digitalWrite(PIN_RLCD_DC, HIGH);
            SPI.transferBytes(col_bounds, nullptr, sizeof(col_bounds));
            digitalWrite(PIN_RLCD_CS, HIGH);

            uint8_t row_bounds[] = {(uint8_t)(y_pos * 4), (uint8_t)(y_pos * 4 + 3)};
            digitalWrite(PIN_RLCD_DC, LOW);
            digitalWrite(PIN_RLCD_CS, LOW);
            SPI.transfer(0x2B);
            digitalWrite(PIN_RLCD_DC, HIGH);
            SPI.transferBytes(row_bounds, nullptr, sizeof(row_bounds));
            digitalWrite(PIN_RLCD_CS, HIGH);

            static const uint8_t st_lut[4][4] = {
                {0x00, 0x80, 0x40, 0xC0},
                {0x00, 0x20, 0x10, 0x30},
                {0x00, 0x08, 0x04, 0x0C},
                {0x00, 0x02, 0x01, 0x03},
            };

            uint8_t all_rows[300] = {0};
            for (int sr = 0; sr < 4; sr++) {
                int shift = sr * 2;
                int base_off = sr * send_cnt;
                int idx = base_off + (addr_first_col >> 2) - send_start;

                for (int col = addr_first_col; col <= addr_last_col; col += 4, idx++) {
                    all_rows[idx] = st_lut[0][(row_base[col] >> shift) & 3]
                                  | st_lut[1][(row_base[col + 1] >> shift) & 3]
                                  | st_lut[2][(row_base[col + 2] >> shift) & 3]
                                  | st_lut[3][(row_base[col + 3] >> shift) & 3];
                }
            }

            digitalWrite(PIN_RLCD_DC, LOW);
            digitalWrite(PIN_RLCD_CS, LOW);
            SPI.transfer(0x2C);
            digitalWrite(PIN_RLCD_DC, HIGH);
            SPI.transferBytes(all_rows, nullptr, (size_t)send_cnt * 4U);
            digitalWrite(PIN_RLCD_CS, HIGH);
            break;
        }
        default:
            return 0;
    }
    return 1;
}

LauncherDisplay Display;

LauncherDisplay::LauncherDisplay() {}

LauncherDisplay::~LauncherDisplay() {
    if (buffer_) free(buffer_);
}

void LauncherDisplay::send_command(uint8_t cmd) {
    digitalWrite(PIN_RLCD_DC, LOW);
    digitalWrite(PIN_RLCD_CS, LOW);
    SPI.transfer(cmd);
    digitalWrite(PIN_RLCD_CS, HIGH);
}

void LauncherDisplay::send_data(const uint8_t *data, size_t len) {
    if (!data || len == 0) return;
    digitalWrite(PIN_RLCD_DC, HIGH);
    digitalWrite(PIN_RLCD_CS, LOW);
    SPI.transferBytes((uint8_t *)data, nullptr, len);
    digitalWrite(PIN_RLCD_CS, HIGH);
}

void LauncherDisplay::init_hardware() {
    digitalWrite(PIN_RLCD_RST, HIGH);
    delay(50);
    digitalWrite(PIN_RLCD_RST, LOW);
    delay(20);
    digitalWrite(PIN_RLCD_RST, HIGH);
    delay(50);

    const uint8_t d6[] = {0x17, 0x02};
    const uint8_t d1[] = {0x01};
    const uint8_t c0[] = {0x11, 0x04};
    const uint8_t c1[] = {0x69, 0x69, 0x69, 0x69};
    const uint8_t c2[] = {0x19, 0x19, 0x19, 0x19};
    const uint8_t c4[] = {0x4B, 0x4B, 0x4B, 0x4B};
    const uint8_t d8[] = {0x80, 0xE9};
    const uint8_t b2[] = {0x02};
    const uint8_t b3[] = {0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
    const uint8_t b4[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
    const uint8_t g_timing[] = {0x32, 0x03, 0x1F};
    const uint8_t b7[] = {0x13};
    const uint8_t b0[] = {0x64};
    const uint8_t c9[] = {0x00};
    const uint8_t m36[] = {0x48};
    const uint8_t m3a[] = {0x11};
    const uint8_t b9[] = {0x20};
    const uint8_t b8[] = {0x29};
    const uint8_t win_a[] = {0x12, 0x2A};
    const uint8_t win_b[] = {0x00, 0xC7};
    const uint8_t m35[] = {0x00};
    const uint8_t d0[] = {0xFF};

    auto cmd_data = [this](uint8_t cmd, const uint8_t *d, size_t len) {
        send_command(cmd);
        if (len > 0) send_data(d, len);
    };

    cmd_data(0xD6, d6, sizeof(d6));
    cmd_data(0xD1, d1, sizeof(d1));
    cmd_data(0xC0, c0, sizeof(c0));
    cmd_data(0xC1, c1, sizeof(c1));
    cmd_data(0xC2, c2, sizeof(c2));
    cmd_data(0xC4, c4, sizeof(c4));
    cmd_data(0xC5, c2, sizeof(c2));
    cmd_data(0xD8, d8, sizeof(d8));
    cmd_data(0xB2, b2, sizeof(b2));
    cmd_data(0xB3, b3, sizeof(b3));
    cmd_data(0xB4, b4, sizeof(b4));
    cmd_data(0x62, g_timing, sizeof(g_timing));
    cmd_data(0xB7, b7, sizeof(b7));
    cmd_data(0xB0, b0, sizeof(b0));
    send_command(0x11);
    delay(120);
    cmd_data(0xC9, c9, sizeof(c9));
    cmd_data(0x36, m36, sizeof(m36));
    cmd_data(0x3A, m3a, sizeof(m3a));
    cmd_data(0xB9, b9, sizeof(b9));
    cmd_data(0xB8, b8, sizeof(b8));
    send_command(0x20); // 0x20 = INVOFF (Normal Display: 0=White background, 1=Black text/pixels)
    cmd_data(0x2A, win_a, sizeof(win_a));
    cmd_data(0x2B, win_b, sizeof(win_b));
    cmd_data(0x35, m35, sizeof(m35));
    cmd_data(0xD0, d0, sizeof(d0));
    send_command(0x38);
    send_command(0x29);
}

bool LauncherDisplay::begin() {
    pinMode(PIN_RLCD_DC, OUTPUT);
    pinMode(PIN_RLCD_CS, OUTPUT);
    pinMode(PIN_RLCD_RST, OUTPUT);
    digitalWrite(PIN_RLCD_CS, HIGH);
    digitalWrite(PIN_RLCD_DC, HIGH);
    digitalWrite(PIN_RLCD_RST, HIGH);

    SPI.begin(PIN_RLCD_SCK, -1, PIN_RLCD_MOSI, -1);
    SPI.beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE0));

    init_hardware();

    size_t buf_sz = ST7305_BUFFER_ROW_BYTES * ST7305_TILE_HEIGHT;
    buffer_ = (uint8_t *)heap_caps_malloc(buf_sz, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!buffer_) buffer_ = (uint8_t *)malloc(buf_sz);
    memset(buffer_, 0, buf_sz);

    u8x8_Setup(u8g2_GetU8x8(&u8g2_obj_), u8x8_d_st7305_custom, u8x8_dummy_cb, u8x8_byte_custom, u8x8_dummy_cb);
    u8g2_SetupBuffer(&u8g2_obj_, buffer_, ST7305_TILE_HEIGHT, u8g2_ll_hvline_vertical_top_lsb, U8G2_R1);
    u8g2_InitDisplay(&u8g2_obj_);
    u8g2_SetPowerSave(&u8g2_obj_, 0);

    clear();
    flush();
    return true;
}

void LauncherDisplay::clear() {
    u8g2_ClearBuffer(&u8g2_obj_);
}

void LauncherDisplay::flush() {
    u8g2_SendBuffer(&u8g2_obj_);
}

void LauncherDisplay::draw_pixel(int x, int y, bool black) {
    if (x < 0 || x >= RLCD_WIDTH || y < 0 || y >= RLCD_HEIGHT) return;
    u8g2_SetDrawColor(&u8g2_obj_, black ? 1 : 0);
    u8g2_DrawPixel(&u8g2_obj_, x, y);
}

void LauncherDisplay::draw_string(int x, int y, const char *str, bool black, const uint8_t *font) {
    if (!str) return;
    u8g2_SetFont(&u8g2_obj_, font ? font : u8g2_font_helvB10_tr);
    u8g2_SetDrawColor(&u8g2_obj_, black ? 1 : 0);
    u8g2_DrawStr(&u8g2_obj_, x, y, str);
}

void LauncherDisplay::draw_rect(int x, int y, int w, int h, bool black, bool fill) {
    u8g2_SetDrawColor(&u8g2_obj_, black ? 1 : 0);
    if (fill) {
        u8g2_DrawBox(&u8g2_obj_, x, y, w, h);
    } else {
        u8g2_DrawFrame(&u8g2_obj_, x, y, w, h);
    }
}

void LauncherDisplay::draw_line(int x0, int y0, int x1, int y1, bool black) {
    u8g2_SetDrawColor(&u8g2_obj_, black ? 1 : 0);
    u8g2_DrawLine(&u8g2_obj_, x0, y0, x1, y1);
}

void LauncherDisplay::draw_progress_bar(int x, int y, int w, int h, int percent, const char *label) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;

    if (label) {
        draw_string(x, y - 6, label, true, u8g2_font_helvB10_tr);
    }

    // Outer frame
    draw_rect(x, y, w, h, true, false);

    // Inner progress fill
    int inner_w = (w - 4) * percent / 100;
    if (inner_w > 0) {
        draw_rect(x + 2, y + 2, inner_w, h - 4, true, true);
    }

    char pct_str[16];
    snprintf(pct_str, sizeof(pct_str), "%d%%", percent);
    draw_string(x + w + 8, y + h - 2, pct_str, true, u8g2_font_helvR10_tr);
}

void LauncherDisplay::draw_battery(int x, int y, int percent) {
    int pct = constrain(percent, 0, 100);
    // White outer frame + terminal tip on black top bar
    draw_rect(x, y, 20, 11, false, false);
    draw_rect(x + 20, y + 3, 2, 5, false, true);
    // 1px black inner separation border so 100% fill remains clearly outlined
    draw_rect(x + 1, y + 1, 18, 9, true, true);
    int fill_w = (16 * pct) / 100;
    if (fill_w > 0) {
        draw_rect(x + 2, y + 2, fill_w, 7, false, true);
    }
}
