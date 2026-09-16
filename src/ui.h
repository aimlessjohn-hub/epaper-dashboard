#pragma once
#include "net.h"

void ui_render(const DashboardData &d, uint8_t batt_pct, bool wifi_ok,
               float shtc_t, float shtc_h);
void ui_render_offline(uint8_t batt_pct, const char *last_seen);