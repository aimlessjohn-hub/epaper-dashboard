/**
 * ui.cpp - Dashboard-Layout 200x200 (User-Koordinaten, Landscape via Rotation).
 *
 * Zeichnet ueber display.cpp (Transformation auf Panel-Koordinaten).
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

    // ---------- Header (Zeile 0..14) ----------
    // Links: Zeitstempel vom Endpoint, Rechts: Akku + WiFi
    char l1[24];
    snprintf(l1, sizeof(l1), "%s", d.generated);
    disp_draw_text(2, 1, l1, COL_BLACK, 1);
    draw_battery_icon(160, 1, batt_pct);
    if (!wifi_ok) disp_draw_text(182, 1, "x", COL_BLACK, 1);

    hline(0, 12, 200, COL_BLACK);

    // ---------- Waschmaschine (groß, 14..62) ----------
    disp_draw_text(2, 16, "Waschmaschine", COL_BLACK, 1);
    if (d.wasser_on && d.wasser_w > 2.0f) {
        char t[32];
        snprintf(t, sizeof(t), "%s W", d.wasser_w >= 100 ? "" : "");
        char big[32];
        snprintf(big, sizeof(big), "%.0f W", (double)d.wasser_w);
        disp_draw_text(2, 28, big, COL_BLACK, 3);   // ~18px hoch
        disp_draw_text(2, 52, "LAEUFT", COL_BLACK, 1);
    } else if (d.wasser_on) {
        disp_draw_text(2, 28, "FERTIG", COL_BLACK, 3);
    } else {
        disp_draw_text(2, 28, "AUS", COL_BLACK, 3);
    }

    hline(0, 60, 200, COL_BLACK);

    // ---------- Wetter (62..106) ----------
    char wline[48];
    snprintf(wline, sizeof(wline), "%s  %.1f/%.1fC  Regen %d%%",
             wmo_desc(d.wetter_code), (double)d.temp_max, (double)d.temp_min, d.rain_pct);
    disp_draw_text(2, 64, wline, COL_BLACK, 1);
    char wbig[24];
    snprintf(wbig, sizeof(wbig), "%.1f C", (double)d.temp_c);
    disp_draw_text(2, 72, wbig, COL_BLACK, 3);

    // ---------- Homelab (106..150) ----------
    char hl[48];
    snprintf(hl, sizeof(hl), "PVE %d/%d VMs  CPU %d%%  %.0fC",
             d.vms_running, d.vms_total, (int)(d.pve_cpu_pct + 0.5f), (double)d.pve_cpu_temp);
    disp_draw_text(2, 108, hl, COL_BLACK, 1);
    char ram[48];
    snprintf(ram, sizeof(ram), "RAM %.1f/%.1f GB", (double)d.pve_ram_gb, (double)d.pve_ram_total);
    disp_draw_text(2, 118, ram, COL_BLACK, 1);
    char bak[48];
    snprintf(bak, sizeof(bak), "Backup %s", d.last_backup);
    disp_draw_text(2, 128, bak, COL_BLACK, 1);

    hline(0, 140, 200, COL_BLACK);

    // ---------- Innen-Klima + Statuszeile (140..199) ----------
    char climate[64];
    snprintf(climate, sizeof(climate), "Zimmer %.1fC %d%%  |  DECT %.1fC",
             (double)shtc_t, (int)(shtc_h + 0.5f), (double)d.dect_temp_c);
    disp_draw_text(2, 144, climate, COL_BLACK, 1);

    // Unten: Battery-% numerisch + Update-Zeit
    char foot[48];
    snprintf(foot, sizeof(foot), "Akku %d%%  Update %s", batt_pct, d.generated);
    disp_draw_text(2, 188, foot, COL_BLACK, 1);
}

void ui_render_offline(uint8_t batt_pct, const char *last_seen) {
    disp_clear(COL_WHITE);
    disp_draw_text(2, 16, "OFFLINE", COL_BLACK, 3);
    char t[64];
    snprintf(t, sizeof(t), "Letzte Daten: %s", last_seen);
    disp_draw_text(2, 48, t, COL_BLACK, 1);
    disp_draw_text(2, 188, "Akku %-Zahl folgt", COL_BLACK, 1);
}