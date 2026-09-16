/**
 * ui.cpp - Dashboard-Layout 200x200 (User-Koordinaten, Landscape via Rotation).
 *
 * v2 "Slides"-Design: WENIGE, GROSSE Werte (1,54" = 184 DPI, 5x7-Font
 * scale1 = ~1mm hoch = unlesbar aus >50cm). 4 Quadranten, je 100x100:
 *   Q1 Waschmaschine (Status scale 2, Wert scale 2)
 *   Q2 Wetter (Temp scale 4 = ~29px)
 *   Q3 PVE (CPU/RAM scale 2)
 *   Q4 Zimmer (Temp scale 3)
 * E-Paper 1-Bit: Schwarz/Weiss, keine Graustoefen.
 */
#include <Arduino.h>
#include <stdio.h>
#include "ui.h"
#include "display.h"
#include "net.h"
#include "power.h"

#define COL_BLACK 0
#define COL_WHITE 1

static void hline(uint16_t x, uint16_t y, uint16_t w, uint8_t color) {
    for (uint16_t i = 0; i < w; i++) disp_draw_pixel(x + i, y, color);
}

static void vline(uint16_t x, uint16_t y, uint16_t h, uint8_t color) {
    for (uint16_t i = 0; i < h; i++) disp_draw_pixel(x, y + i, color);
}

static void draw_battery_icon(uint16_t x, uint16_t y, uint8_t pct) {
    // 18x9 Icon: Rahmen + Fuellstand
    for (int i = 0; i < 16; i++) { disp_draw_pixel(x + i, y, COL_BLACK); disp_draw_pixel(x + i, y + 8, COL_BLACK); }
    for (int j = 0; j < 9; j++) {
        disp_draw_pixel(x, y + j, COL_BLACK);
        disp_draw_pixel(x + 15, y + j, COL_BLACK);
        disp_draw_pixel(x + 16, y + 3 + (j / 3), COL_BLACK); // Pol
    }
    uint8_t fill = (uint16_t)14 * 1 * pct / 100; // bis 14px Innenbreite
    for (uint8_t i = 0; i < fill && i < 14; i++)
        for (uint8_t j = 2; j < 7; j++)
            disp_draw_pixel(x + 1 + i, y + j, COL_BLACK);
}

// WMO-Code -> kurzer deutscher Text (E-Paper-Font ist 5x7 ASCII)
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

    // ---------- Header (0..13) ----------
    char l1[24];
    snprintf(l1, sizeof(l1), "%s", d.generated);
    disp_draw_text(2, 1, l1, COL_BLACK, 1);
    draw_battery_icon(162, 1, batt_pct);
    if (!wifi_ok) disp_draw_text(184, 1, "x", COL_BLACK, 1);
    hline(0, 12, 200, COL_BLACK);

    // ---------- Q1 Waschmaschine (14..104) ----------
    disp_draw_text(6, 17, "WASCH", COL_BLACK, 1);
    if (d.wasser_on && d.wasser_w > 2.0f) {
        char big[16];
        snprintf(big, sizeof(big), "%.0f", (double)d.wasser_w);
        disp_draw_text(6, 30, big, COL_BLACK, 2);       // ~14px hoch
        disp_draw_text(6 + 6 * 2 * strlen(big) + 4, 40, "W", COL_BLACK, 1);
        disp_draw_text(6, 88, "LAEUFT", COL_BLACK, 1);
    } else if (d.wasser_on) {
        disp_draw_text(6, 40, "FERTIG", COL_BLACK, 2);
    } else {
        disp_draw_text(6, 40, "AUS", COL_BLACK, 2);
    }
    vline(99, 14, 92, COL_BLACK);

    // ---------- Q2 Wetter (14..104, x>100) ----------
    char wbig[16];
    snprintf(wbig, sizeof(wbig), "%.0f", (double)d.temp_c);
    disp_draw_text(108, 26, wbig, COL_BLACK, 4);        // ~28px hoch
    disp_draw_text(108 + 6 * 4 * strlen(wbig) + 4, 40, "C", COL_BLACK, 2);
    char wline[32];
    snprintf(wline, sizeof(wline), "%s %d/%d", wmo_desc(d.wetter_code),
             (int)(d.temp_max + 0.5f), (int)(d.temp_min + 0.5f));
    disp_draw_text(108, 60, wline, COL_BLACK, 1);       // scale 1 (Meta)
    char rain[24];
    snprintf(rain, sizeof(rain), "Regen %d%%", d.rain_pct);
    disp_draw_text(108, 92, rain, COL_BLACK, 1);
    vline(0, 0, 0, COL_BLACK); // (kein extra Rand)
    hline(0, 106, 200, COL_BLACK);

    // ---------- Q3 PVE (108..152) ----------
    char hl[40];
    snprintf(hl, sizeof(hl), "PVE %d VM  CPU %d%%", d.vms_running, (int)(d.pve_cpu_pct + 0.5f));
    disp_draw_text(6, 112, hl, COL_BLACK, 1);
    char ram[40];
    snprintf(ram, sizeof(ram), "RAM %.0f/%.0f GB", (double)d.pve_ram_gb, (double)d.pve_ram_total);
    disp_draw_text(6, 124, ram, COL_BLACK, 2);          // ~14px
    vline(99, 108, 92, COL_BLACK);

    // ---------- Q4 Zimmer (108..152, x>100) ----------
    char zbig[16];
    snprintf(zbig, sizeof(zbig), "%.1f", (double)shtc_t);
    disp_draw_text(108, 116, zbig, COL_BLACK, 3);       // ~21px
    disp_draw_text(108 + 6 * 3 * strlen(zbig) + 4, 124, "C", COL_BLACK, 1);
    char zh[24];
    snprintf(zh, sizeof(zh), "%d%%", (int)(shtc_h + 0.5f));
    disp_draw_text(150, 132, zh, COL_BLACK, 1);
}

void ui_render_offline(uint8_t batt_pct, const char *last_seen) {
    disp_clear(COL_WHITE);
    disp_draw_text(6, 20, "OFFLINE", COL_BLACK, 3);
    char t[64];
    snprintf(t, sizeof(t), "Letzte Daten: %s", last_seen);
    disp_draw_text(6, 60, t, COL_BLACK, 1);
}