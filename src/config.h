/**
 * epaper_dashboard - config.h
 * Waveshare ESP32-S3-ePaper-1.54 (V2, ESP32-S3-PICO-1-N8R8)
 *
 * Pin-Map verifiziert gegen Waveshare user_config.h (02_Example) + Schematic
 * + Boot-Log der Factory-FW (16.9.2026).
 */
#pragma once

// ---------- Display (EPD-SPI) ----------
#define EPD_WIDTH      200
#define EPD_HEIGHT     200
#define EPD_CS_PIN     GPIO_NUM_11
#define EPD_SCK_PIN    GPIO_NUM_12
#define EPD_MOSI_PIN   GPIO_NUM_13
#define EPD_DC_PIN     GPIO_NUM_10
#define EPD_RST_PIN    GPIO_NUM_9
#define EPD_BUSY_PIN   GPIO_NUM_8
#define EPD_PWR_PIN    GPIO_NUM_6     // EPD-Versorgung (board_power_bsp)
#define EPD_SPI_NUM    SPI2_HOST

// ---------- Orientation (User-Wunsch: landscape, USB-Port seitlich) ----------
// 0 = Portrait (natuerlich), 1 = 90° CW, 2 = 180°, 3 = 90° CCW
#define DISPLAY_ROTATION 1

// ---------- Power-Regeln ----------
// GPIO17 = BAT_Control: MUSS HIGH vor Deep-Sleep-Entry, sonst wacht das
// Board im Akkubetrieb NIE wieder auf (Amazon-Rezension, HW-verifiziert).
#define VBAT_PWR_PIN   GPIO_NUM_17
#define BAT_ADC_PIN    GPIO_NUM_4     // Spannungsteiler /2 (200K/200K)
#define BAT_V_FULL     4.20f          // LiPo 1S
#define BAT_V_EMPTY    3.30f

// ---------- Audio (Phase 2, vorbereitet) ----------
#define AUDIO_PWR_PIN  GPIO_NUM_42    // PA_EN (NS4150B)
#define PA_CTRL_PIN    GPIO_NUM_46
// I2S: MCLK=14 SCLK(BCLK)=15 LRCK(WS)=38 ASDOUT(DIN,Mic)=16 DSDIN(DOUT)=45
#define I2S_MCLK_PIN   GPIO_NUM_14
#define I2S_BCLK_PIN   GPIO_NUM_15
#define I2S_WS_PIN     GPIO_NUM_38
#define I2S_DIN_PIN    GPIO_NUM_16
#define I2S_DOUT_PIN   GPIO_NUM_45

// ---------- I2C (SHTC3 + PCF85063) ----------
#define I2C_SDA_PIN    GPIO_NUM_47
#define I2C_SCL_PIN    GPIO_NUM_48
#define SHTC3_ADDR     0x70
#define PCF85063_ADDR  0x51

// ---------- Buttons ----------
#define BOOT_BTN_PIN   GPIO_NUM_0     // auch EXT1-Wakeup
#define PWR_BTN_PIN    GPIO_NUM_18

// ---------- WLAN / Endpunkt (echte Werte NIE committen!) ----------
#include "secrets.h"                  // WIFI_SSID, WIFI_PASS (SD/git-ignored)

#define STATUS_URL    "http://192.168.178.83:8123/status?compact=1"
#define PIPER_URL     "http://192.168.178.56:3000/"  // Phase 2 (Thorsten TTS)

// ---------- Timing ----------
#define WIFI_TIMEOUT_MS   15000
#define HTTP_TIMEOUT_MS   10000
#define SLEEP_INTERVAL_MIN  10    // Deep-Sleep-Zyklus (Phase 3: adaptiv)