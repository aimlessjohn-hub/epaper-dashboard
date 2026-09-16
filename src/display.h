#pragma once
#include <stdint.h>

void disp_draw_pixel(uint16_t ux, uint16_t uy, uint8_t color);   // 0=schwarz 1=weiss
void disp_clear(uint8_t color);
void disp_draw_text(uint16_t ux, uint16_t uy, const char *s, uint8_t color, uint8_t scale = 1);