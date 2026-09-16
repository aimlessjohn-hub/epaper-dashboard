/**
 * ui.cpp - Dashboard-Layout 200x200 (User-Koordinaten, ROTATION 3).
 *
 * v4 "3 Riesenwerte": User-Feedback 16.9. abends - "zu viele Infos,
 * unlesbar". Radikaler Schnitt: NUR 3 Werte in Riesentyp (scale 4-6),
 * keine Meta-Zeilen. Alles andere (PVE-Detail, Regen, Backup) ist
 * bewusst DRAUSSEN - das Display ist ein Blick-Am-Tag-Geraet.
 *
 *   Zeile 1:  WASCHMASCHINE-Status (LAEUFT 8W / FERTIG / AUS)
 *   Zeile 2:  AUSSEN-Temperatur (gross, 28px) + H/T-MinMax klein
 *   Zeile 3:  ZIMMER-Temperatur (21px) + rF
 * Footer: Akku-Icon + Update-Zeit (scale 1, nur 1 Zeile)
 */
#include <Arduino.h>
#include <stdio.h>
#include "ui.h"
#include "display.h"
#include "net.h"
#include "power.h"

static void hline(uint16_t x, uint16_t y, uint16_t w, uint8_t color) {
    for (uint16_t i = 0; i < w; i++) disp_draw_pixel(x + i, y, color);
}

static void draw_battery_icon(uint16_t x, uint16_t y, uint8_t pct) {
    for (int i = 0; i < 16; i++) { disp_draw_pixel(x + i, y, COL_BLACK); disp_draw_pixel(x + i, y + 8, COL_BLACK); }
    for (int j = 0; j < 9; j++) {
        disp_draw_pixel(x, y + j, COL_BLACK);
        disp_draw_pixel(x + 15, y + j, COL_BLACK);
        disp_draw_pixel(x + 16, y + 3 + (j / 3), COL_BLACK);
    }
    uint8_t fill = (uint16_t)14 * pct / 100;
    for (uint8_t i = 0; i < fill && i < 14; i++)
        for (uint8_t j = 2; j < 7; j++)
            disp_draw_pixel(x + 1 + i, y + j, COL_BLACK);
}

static const char *wmo_desc(int code) {
    if (code < 0) return "?";
    if (code == 0) return "klar";
    if (code <= 2) return "wolkig";
    if (code == 3) return "bedeckt";
    if (code <= 48) return "nebel";
    if (code <= 57) return "Niesel";
    if (code <= 67) return "Regen";
    if (code <= 77) return "Schnee";
    if (code <= 82) return "Schauer";
    return "Gewitter";
}

void ui_render(const DashboardData &d, uint8_t batt_pct, bool wifi_ok,
               float shtc_t, float shtc_h) {
    disp_clear(COL_WHITE);

    // ---------- Zeile 1: Waschmaschine (14..74) ----------
    if (d.wasser_on && d.wasser_w > 2.0f) {
        char big[16];
        snprintf(big, sizeof(big), "%.0fW", (double)d.wasser_w);
        disp_draw_text(8, 22, big, COL_BLACK, 4);        // 28px, Riesenwert
        disp_draw_text(8, 64, "LAEUFT", COL_BLACK, 1);
    } else if (d.wasser_on) {
        disp_draw_text(8, 22, "FERTIG", COL_BLACK, 4);
    } else {
        disp_draw_text(8, 22, "AUS", COL_BLACK, 4);
    }
    hline(0, 76, 200, COL_BLACK);

    // ---------- Zeile 2: Aussen (78..138) ----------
    char wb[16];
    snprintf(wb, sizeof(wb), "%.0fC", (double)d.temp_c);
    disp_draw_text(8, 84, wb, COL_BLACK, 5);             // 35px, groesster Wert
    char wsub[40];
    snprintf(wsub, sizeof(wsub), "%s  H%d T%d  R%d%%",
             wmo_desc(d.wetter_code), (int)(d.temp_max + 0.5f),
             (int)(d.temp_min + 0.5f), d.rain_pct);
    disp_draw_text(8, 126, wsub, COL_BLACK, 1);          // 1 Meta-Zeile
    hline(0, 140, 200, COL_BLACK);

    // ---------- Zeile 3: Zimmer (140..186) ----------
    char zb[16];
    snprintf(zb, sizeof(zb), "%.1f", (double)shtc_t);
    disp_draw_text(8, 146, zb, COL_BLACK, 4);            // 28px
    disp_draw_text(8 + 6 * 4 * strlen(zb) + 4, 156, "C", COL_BLACK, 2);
    char zh[24];
    snprintf(zh, sizeof(zh), "%d%%", (int)(shtc_h + 0.5f));
    disp_draw_text(150, 170, zh, COL_BLACK, 1);

    // ---------- Footer: Akku + Zeit (190..199) ----------
    draw_battery_icon(6, 191, batt_pct);
    disp_draw_text(30, 191, d.generated, COL_BLACK, 1);
    if (!wifi_ok) disp_draw_text(190, 191, "x", COL_BLACK, 1);
}

void ui_render_offline(uint8_t batt_pct, const char *last_seen) {
    disp_clear(COL_WHITE);
    disp_draw_text(8, 40, "OFFLINE", COL_BLACK, 4);
    disp_draw_text(8, 100, last_seen, COL_BLACK, 1);
    draw_battery_icon(6, 191, batt_pct);
}