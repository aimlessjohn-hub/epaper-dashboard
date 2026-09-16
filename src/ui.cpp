/**
 * ui.cpp - Dashboard-Layout 200x200 (User-Koordinaten, Landscape via Rotation).
 *
 * v3 "Slides"-Design: WENIGE, GROSSE Werte (1,54" = 184 DPI, 5x7-Font
 * scale1 = ~1mm hoch = unlesbar, User-Feedback 16.9.: "nicht lesbar").
 * 4 Quadranten, je 100x100, NUR scale>=2 Zeilen + kompakte scale-1-Meta:
 *   Q1 Waschmaschine  (Status scale 2, Wert scale 3)
 *   Q2 Wetter         (Temp scale 4, Min/Max scale 1 untereinander)
 *   Q3 PVE            (RAM scale 2, CPU-Zeile scale 1)
 *   Q4 Zimmer         (Temp scale 3)
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
    uint8_t fill = (uint16_t)14 * pct / 100;
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

    // ---------- Header (0..13): Zeit + Akku, scale 1 (rein informativ) ----------
    disp_draw_text(2, 1, d.generated, COL_BLACK, 1);
    draw_battery_icon(162, 1, batt_pct);
    if (!wifi_ok) disp_draw_text(184, 1, "x", COL_BLACK, 1);
    hline(0, 12, 200, COL_BLACK);

    // ---------- Q1 Waschmaschine (14..104, links) ----------
    if (d.wasser_on && d.wasser_w > 2.0f) {
        char big[16];
        snprintf(big, sizeof(big), "%.0fW", (double)d.wasser_w);
        disp_draw_text(8, 34, big, COL_BLACK, 2);       // ~14px
        disp_draw_text(8, 74, "LAEUFT", COL_BLACK, 2);
    } else if (d.wasser_on) {
        disp_draw_text(8, 44, "FERTIG", COL_BLACK, 2);
    } else {
        disp_draw_text(8, 44, "AUS", COL_BLACK, 2);
    }
    vline(99, 14, 92, COL_BLACK);

    // ---------- Q2 Wetter (14..104, rechts) ----------
    char wbig[16];
    snprintf(wbig, sizeof(wbig), "%.0f", (double)d.temp_c);
    disp_draw_text(106, 30, wbig, COL_BLACK, 4);        // ~28px, dominanter Wert
    disp_draw_text(106 + 6 * 4 * strlen(wbig) + 4, 44, "C", COL_BLACK, 2);
    char wmin[24];
    snprintf(wmin, sizeof(wmin), "H %d T %d", (int)(d.temp_max + 0.5f), (int)(d.temp_min + 0.5f));
    disp_draw_text(106, 68, wmin, COL_BLACK, 1);
    char rain[24];
    snprintf(rain, sizeof(rain), "Regen %d%%", d.rain_pct);
    disp_draw_text(106, 92, rain, COL_BLACK, 1);
    hline(0, 106, 200, COL_BLACK);

    // ---------- Q3 PVE (108..152, links) ----------
    char ram[32];
    snprintf(ram, sizeof(ram), "RAM %.0f/%.0f", (double)d.pve_ram_gb, (double)d.pve_ram_total);
    disp_draw_text(8, 118, ram, COL_BLACK, 2);          // ~14px
    char hl[40];
    snprintf(hl, sizeof(hl), "%d VMs  CPU %d%%", d.vms_running, (int)(d.pve_cpu_pct + 0.5f));
    disp_draw_text(8, 140, hl, COL_BLACK, 1);
    vline(99, 108, 92, COL_BLACK);

    // ---------- Q4 Zimmer (108..152, rechts) ----------
    char zbig[16];
    snprintf(zbig, sizeof(zbig), "%.1f", (double)shtc_t);
    disp_draw_text(106, 116, zbig, COL_BLACK, 3);       // ~21px
    disp_draw_text(106 + 6 * 3 * strlen(zbig) + 4, 126, "C", COL_BLACK, 2);
    char zh[24];
    snprintf(zh, sizeof(zh), "%d%% rF", (int)(shtc_h + 0.5f));
    disp_draw_text(106, 148, zh, COL_BLACK, 1);
}

void ui_render_offline(uint8_t batt_pct, const char *last_seen) {
    disp_clear(COL_WHITE);
    disp_draw_text(8, 40, "OFFLINE", COL_BLACK, 3);
    char t[64];
    snprintf(t, sizeof(t), "Letzte Daten: %s", last_seen);
    disp_draw_text(8, 90, t, COL_BLACK, 1);
}