#pragma once
#include <Arduino.h>

/**
 * Waveshare / Luckfox ESP32-S3 4.2-inch RLCD MultiBoot Launcher Pinout
 */

// Display: 4.2" Reflective LCD (ST7305 SPI Controller)
#define PIN_RLCD_SCK       11  // SPI Clock
#define PIN_RLCD_MOSI      12  // SPI MOSI / Data
#define PIN_RLCD_DC         5  // Data / Command select
#define PIN_RLCD_CS        40  // Chip Select (active LOW)
#define PIN_RLCD_RST       41  // Hardware Reset (active LOW)

#define RLCD_WIDTH        400  // Display Width in Pixels
#define RLCD_HEIGHT       300  // Display Height in Pixels

// MicroSD Card Interface (1-bit SDMMC Mode)
#define PIN_SD_CLK         38  // SDMMC Clock
#define PIN_SD_CMD         21  // SDMMC Command / Response
#define PIN_SD_D0          39  // SDMMC Data 0

// Audio Subsystem & Power Amp
#define PIN_PA_ENABLE      46  // Class-D Amplifier Shutdown (Active HIGH, keep LOW in launcher to save power)

// Buttons & Battery ADC
#define PIN_BTN_BOOT        0  // BOOT button: Select / Flash (Active LOW)
#define PIN_BTN_KEY        18  // KEY button: Next item (Active LOW)
#define PIN_BATTERY_ADC     4  // ADC1_CH3

// Wi-Fi Configuration
#define DEFAULT_STA_SSID   "YOUR_WIFI_SSID"
#define DEFAULT_STA_PASS   "YOUR_WIFI_PASSWORD"
#define LAUNCHER_AP_SSID   "RLCD-Launcher"
#define LAUNCHER_AP_PASS   "launcher123"
