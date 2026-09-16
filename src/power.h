#pragma once
#include <stdint.h>

void pwr_gpio_init(void);
void pwr_epd_on(bool on);
void pwr_audio_on(bool on);
void pwr_vbat_hold_high_for_sleep(void);   // GPIO17-Regel: RTC-HOLD HIGH
void pwr_vbat_hold_dis(void);
void pwr_enable_wakeup_sources(void);
void pwr_deep_sleep_now(void);             // Reihenfolge fix: hold -> wake -> sleep
float pwr_battery_voltage(void);
uint8_t pwr_battery_percent(void);