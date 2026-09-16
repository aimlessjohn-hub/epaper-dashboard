/**
 * power.cpp - Board-Power-BSP (korrigiert) fuer Waveshare ESP32-S3-ePaper-1.54
 *
 * Kritischer Fix gegenueber Waveshare-Beispiel:
 * GPIO17 (BAT_Control) muss im Deep Sleep HIGH bleiben (Rezension "Reiner":
 * "Pin 17 muß auf HIGH liegen, sonst wacht der Baustein im Batteriebetrieb
 * nicht aus dem Deep-Sleep auf"). Waveshare hat die rtc_gpio_*-Zeilen im
 * Beispiel AUSKOMMENTIERT (board_power_bsp.cpp, Konstruktor). Wir machen
 * sie explizit: RTC-GPIO-Init -> Output-HIGH -> hold im Sleep.
 */
#include <Arduino.h>
#include <Wire.h>
#include "power.h"
#include "config.h"

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"

void pwr_gpio_init(void) {
    // EPD-Power (GPIO6, active-low enable), Audio-PA (GPIO42), BAT_Control (GPIO17)
    for (uint8_t pin : {EPD_PWR_PIN, AUDIO_PWR_PIN, VBAT_PWR_PIN}) {
        gpio_reset_pin((gpio_num_t)pin);
    }
    gpio_config_t io = {};
    io.intr_type = GPIO_INTR_DISABLE;
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = (1ULL << EPD_PWR_PIN) | (1ULL << AUDIO_PWR_PIN) | (1ULL << VBAT_PWR_PIN);
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io));

    // PCF85063-INT-Flags loeschen (Demo-FW hinterliess evtl. aktiven
    // RTC-Alarm -> INT(GPIO5) haengt LOW). Control_2 (0x01) auf 0x00.
    // I2C hier manuell, da Wire noch nicht gebunden ist:
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.beginTransmission(PCF85063_ADDR);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.endTransmission();
}

void pwr_epd_on(bool on)  { gpio_set_level((gpio_num_t)EPD_PWR_PIN, on ? 0 : 1); }
void pwr_audio_on(bool on) { gpio_set_level((gpio_num_t)AUDIO_PWR_PIN, on ? 0 : 1); }

void pwr_vbat_hold_high_for_sleep(void) {
    // GPIO17 in den RTC-Domain bringen, HIGH treiben und im Sleep halten.
    // Waveshare-Referenz-Stil: rtc_gpio_hold_en reicht (sleep-direction
    // raus - im Verdacht, den RTC-Timer-Wake zu blockieren, 16.9.).
    const gpio_num_t pin = (gpio_num_t)VBAT_PWR_PIN;
    rtc_gpio_init(pin);
    rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(pin, 1);                                      // HIGH = BAT_Control aktiv
    rtc_gpio_pullup_en(pin);
    rtc_gpio_pulldown_dis(pin);
    rtc_gpio_hold_en(pin);                                           // Latch bis Aufwachen
}

void pwr_vbat_hold_dis(void) {
    rtc_gpio_hold_dis((gpio_num_t)VBAT_PWR_PIN);
    gpio_hold_dis((gpio_num_t)VBAT_PWR_PIN);
}

// Wake-Quellen: NUR Timer (Phase 1). EXT1 (BOOT/PWR/RTC-INT) deaktiviert:
// GPIO5 = PCF85063-INT ist Open-Drain OHNE Sicherstellung des Pullups im
// Sleep - haengt er LOW (alter RTC-Alarm der Demo-FW), wake'd der Chip
// sofort wieder (Boot-Loop, 19.9.-Diagnose). Buttons mit sauberem
// rtc_gpio-Pull-Setup kommen in Phase 3 zurueck.
void pwr_enable_wakeup_sources(void) {
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_MIN * 60ULL * 1000000ULL));
}

void pwr_deep_sleep_now(void) {
    // Reihenfolge ist ABSICHT (GPIO17-Regel zuerst verankern):
    // 1. BAT_Control HIGH + RTC-Hold  2. Wake-Quellen  3. Schlaf
    pwr_vbat_hold_high_for_sleep();
    pwr_enable_wakeup_sources();
    Serial.println("[sleep] entering deep sleep NOW");
    Serial.flush();
    delay(50);
    esp_deep_sleep_start();
}

// Battery: ADC am /2-Teiler (200K/200K), GPIO4 = ADC1_CH3
float pwr_battery_voltage(void) {
    analogReadResolution(12);
    analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);   // bis ~3.3 V am Pin
    uint32_t mv = 0;
    for (int i = 0; i < 8; i++) { mv += analogReadMilliVolts(BAT_ADC_PIN); delay(2); }
    mv /= 8;
    return (mv / 1000.0f) * 2.0f;                     // Teiler /2 rueckrechnen
}

uint8_t pwr_battery_percent(void) {
    float v = pwr_battery_voltage();
    float pct = (v - BAT_V_EMPTY) / (BAT_V_FULL - BAT_V_EMPTY) * 100.0f;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}