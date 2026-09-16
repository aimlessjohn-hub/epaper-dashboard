/**
 * display.cpp - E-Ink-Rendering mit Orientation-Transform (200x200).
 *
 * Basis: Waveshare epaper_driver_bsp (SensorLib-Beispiele, 11_RTC_Sleep_Test).
 * Der Waveshare-Treiber zeichnet in einen RAM-Framebuffer (1 Bit/Pixel,
 * y*25 + x>>3). Keine Rotation im Treiber -> wir transformieren User- zu
 * Panel-Koordinaten in draw_px() bevor wir in den FB schreiben.
 * Layout-Zeichnung arbeitet NUR in User-Koordinaten (UX = landscape-Breite).
 */
#include <Arduino.h>
#include "config.h"
#include "display.h"

// Waveshare-Treiber (aus _ref kopiert, Header-Pfad wie im Beispiel)
#include "display/epaper_driver_bsp.h"
#include "font5x7.h"

extern epaper_driver_display *driver;

// Panel-Koordinaten aus User-Koordinaten je nach DISPLAY_ROTATION
static inline void map_xy(uint16_t ux, uint16_t uy, uint16_t &px, uint16_t &py) {
#if DISPLAY_ROTATION == 0          // Portrait
    px = ux; py = uy;
#elif DISPLAY_ROTATION == 1        // 90° CW (USB-Port links)
    px = uy; py = (EPD_HEIGHT - 1) - ux;
#elif DISPLAY_ROTATION == 2        // 180°
    px = (EPD_WIDTH - 1) - ux; py = (EPD_HEIGHT - 1) - uy;
#else                              // 90° CCW (USB-Port rechts)
    px = (EPD_WIDTH - 1) - uy; py = ux;
#endif
}

void disp_draw_pixel(uint16_t ux, uint16_t uy, uint8_t color) {
    uint16_t px, py;
    map_xy(ux, uy, px, py);
    driver->EPD_DrawColorPixel(px, py, color);
}

void disp_clear(uint8_t color) {
    driver->EPD_Clear();
}

// 5x7 Bitmap-Font (ASCII 32..126), Tabelle in font5x7.h (PROGMEM).
void disp_draw_char(uint16_t ux, uint16_t uy, char c, uint8_t color) {
    if (c < 32 || c > 126) c = '?';
    uint8_t idx = c - 32;
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = pgm_read_byte(&font5x7[idx][col]);
        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (0x80 >> row))
                disp_draw_pixel(ux + col, uy + row, color);
        }
    }
}

void disp_draw_text(uint16_t ux, uint16_t uy, const char *s, uint8_t color, uint8_t scale) {
    uint16_t x = ux;
    while (*s) {
        if (*s == '\n') { uy += 8 * scale; x = ux; s++; continue; }
        for (uint8_t sc = 0; sc < scale; sc++)
            disp_draw_char(x, uy + sc, *s, color);
        // scale in X: Spalten wiederholen
        x += 6 * scale;
        s++;
    }
}