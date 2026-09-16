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
#include "config.h"
#include "net.h"
#include "ui.h"
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
    Serial.printf("[sleep] deep sleep, wake in %d min\r\n", SLEEP_INTERVAL_MIN);
    Serial.flush();
    pwr_deep_sleep_now();
}

void setup() {
    Serial.begin(115200);
    delay(50);
    Serial.println("\n[boot] epaper_dashboard v0.1");

    pwr_gpio_init();
    pwr_vbat_hold_dis();      // Hold vom letzten Sleep loesen (falls RTC-Domain)
    // GPIO17 HIGH sofort setzen (Regel: BAT_Control aktiv im Wachbetrieb)
    pinMode(VBAT_PWR_PIN, OUTPUT);
    digitalWrite(VBAT_PWR_PIN, HIGH);

    epd_init_and_show();      // erst Display (schnelles Feedback am Geraet)

    bool wifi_ok = net_wifi_connect();
    if (wifi_ok) {
        have_net_data = net_fetch_status(data);
        net_wifi_off();
    }
    if (have_net_data) strlcpy(last_seen, data.generated, sizeof(last_seen));

    sensors_init();
    sensors_read_shtc3(&shtc_t, &shtc_h);
    batt_pct = pwr_battery_percent();

    if (have_net_data) {
        ui_render(data, batt_pct, wifi_ok, shtc_t, shtc_h);
    } else {
        ui_render_offline(batt_pct, last_seen);
    }
    epd_push_frame();

    go_to_sleep();
}

void loop() {
    // Deep Sleep erreicht normalerweise nie hier; Fallback-Sleep nach 60 s
    delay(60000);
    go_to_sleep();
}