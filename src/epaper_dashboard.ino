/**
 * epaper_dashboard.ino - Phase 1 MVP
 * Waveshare ESP32-S3-ePaper-1.54 (V2)
 *
 * Zyklus: Wake -> GPIO17-HIGH -> Display-Init -> WiFi -> :8123 fetch ->
 * SHTC3/BAT lesen -> E-Ink render -> Full-Refresh -> Deep Sleep (RTC/Buttons)
 *
 * Lektionen eingebaut:
 * - GPIO17 (BAT_Control) HIGH + RTC-Hold vor Deep Sleep (Rezensions-Falle,
 *   Waveshare hat rtc_gpio-Init im Beispiel auskommentiert)
 * - qio_opi PSRAM (8MB), setCpuFrequencyMhz TABU
 * - Partial-Refresh nur bei Aenderung (E-Ink-Ghosting vermeiden)
 * - Flashen NUR auf User-Kommando (harte RLCD-Regel)
 */
#include <Arduino.h>
#include "esp_sleep.h"
#include "config.h"
#include "net.h"
#include "ui.h"
#include "display.h"
#include "power.h"
#include "sensors.h"

#include "display/epaper_driver_bsp.h"
epaper_driver_display *driver = NULL;

static DashboardData data;
static uint8_t batt_pct = 0;
static float shtc_t = 0, shtc_h = 0;
static bool have_net_data = false;
static char last_seen[20] = "-";

static void epd_init_and_show(void) {
    pwr_epd_on(true);
    delay(10);
    custom_lcd_spi_t cfg = {};
    cfg.cs = EPD_CS_PIN;
    cfg.dc = EPD_DC_PIN;
    cfg.rst = EPD_RST_PIN;
    cfg.busy = EPD_BUSY_PIN;
    cfg.mosi = EPD_MOSI_PIN;
    cfg.scl = EPD_SCK_PIN;
    cfg.spi_host = EPD_SPI_NUM;
    cfg.buffer_len = 5000;
    driver = new epaper_driver_display(EPD_WIDTH, EPD_HEIGHT, cfg);
    driver->EPD_Init();
    driver->EPD_Clear();
    driver->EPD_DisplayPartBaseImage();
    driver->EPD_Init_Partial();   // Partial-Refresh aktivieren
}

static void epd_push_frame(void) {
    // UI hat Pixel in den FB geschrieben -> jetzt raus schieben
    driver->EPD_DisplayPart();
}

static void go_to_sleep(void) {
    net_wifi_off();
    // EPD-Versorgung aus (Bild bleibt!); Audio-PA aus (idle-Strom);
    // GPIO17-Hold passiert INNERHALB von pwr_deep_sleep_now().
    pwr_epd_on(false);
    pwr_audio_on(false);
    delay(20);
    Serial.printf("[sleep] cause=%d -> deep sleep %d min\r\n", (int)esp_sleep_get_wakeup_cause(), SLEEP_INTERVAL_MIN);
    Serial.flush();
    pwr_deep_sleep_now();
}

void setup() {
    Serial.begin(115200);
    delay(50);
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    Serial.printf("\n[boot] epaper_dashboard v0.1.1 (wake_cause=%d)\r\n", (int)esp_sleep_get_wakeup_cause());

    pwr_gpio_init();
    pwr_vbat_hold_dis();      // Hold vom letzten Sleep loesen (falls RTC-Domain)
    // GPIO17 HIGH sofort setzen (Regel: BAT_Control aktiv im Wachbetrieb)
    pinMode(VBAT_PWR_PIN, OUTPUT);
    digitalWrite(VBAT_PWR_PIN, HIGH);

    // agy-Review-Fix 4a: USB-Upload-Fenster beim Kaltstart (Power-On/USB-Reset).
    // Nach Deep-Sleep-Wakeup NICHT warten (Akku!), nur beim echten Power-On.
    if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
        Serial.println("[boot] Kaltstart - 3s Upload-Fenster");
        delay(3000);
    }

    epd_init_and_show();      // erst Display (schnelles Feedback am Geraet)

    bool wifi_ok = net_wifi_connect();
    Serial.printf("[net] wifi_ok=%d\r\n", wifi_ok);
    if (wifi_ok) {
        have_net_data = net_fetch_status(data);
        Serial.printf("[net] fetch_ok=%d (generated=%s)\r\n", have_net_data, data.generated);
        net_wifi_off();
    }
    if (have_net_data) strlcpy(last_seen, data.generated, sizeof(last_seen));

    sensors_init();
    sensors_read_shtc3(&shtc_t, &shtc_h);
    batt_pct = pwr_battery_percent();

#if ORIENTATION_TEST
    // Orientierungs-Testbild: 4 Punktmuster an den User-Ecken + großes
    // F mittig. Ein Foto liefert Montage-Richtung + Mirror-Beweis.
    {
        disp_clear(COL_WHITE);
        // Ecke 1 (user 5,5): 1 Punkt;  Ecke 2 (user 194,5): 2 Punkte;
        // Ecke 3 (user 5,194): 3 Punkte; Ecke 4 (user 194,194): 4 Punkte
        for (int i = 0; i < 1; i++) for (int j = 0; j < 1; j++) disp_draw_pixel(5 + i, 5 + j, COL_BLACK);
        for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) disp_draw_pixel(190 + i, 5 + j, COL_BLACK);
        for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) disp_draw_pixel(5 + i, 190 + j, COL_BLACK);
        for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) disp_draw_pixel(190 + i, 190 + j, COL_BLACK);
        // Grosses F (scale 6: 36x42 px) mittig
        disp_draw_text(70, 76, "F", COL_BLACK, 6);
        disp_draw_text(20, 30, "TOP", COL_BLACK, 2);
        disp_draw_text(20, 170, "BOTTOM", COL_BLACK, 1);
        epd_push_frame();
        Serial.println("[orient] Testbild gezeichnet");
        delay(60000);   // 60 s sichtbar halten, dann schlafen
        go_to_sleep();
    }
#endif

    if (have_net_data) {
        ui_render(data, batt_pct, wifi_ok, shtc_t, shtc_h);
    } else {
        ui_render_offline(batt_pct, last_seen);
    }
    epd_push_frame();

    // agy-Review-Fix 4b: Warten, bis BOOT/PWR-Buttons HIGH (Tasten gelöst) -
    // ohne EXT1-Wake ist das kein Sofort-Wake-Risiko mehr, aber ein
    // versehentlich gedrückter Button beim Ablegen soll nicht stören.
    const uint32_t btn_wait_t0 = millis();
    while ((digitalRead(BOOT_BTN_PIN) == LOW || digitalRead(PWR_BTN_PIN) == LOW)
           && millis() - btn_wait_t0 < 2000) {
        delay(10);
    }

    go_to_sleep();
}

void loop() {
    // Deep Sleep erreicht normalerweise nie hier; Fallback-Sleep nach 60 s
    delay(60000);
    go_to_sleep();
}