#pragma once
#include <stdint.h>
#include <WString.h>

struct DashboardData {
    char generated[20];
    // Waschmaschine (P110 .81)
    bool wasser_on;
    float wasser_w;
    char wasser_state[16];
    // Homelab (pve_host)
    int vms_running, vms_total;
    float pve_cpu_pct;
    float pve_ram_gb, pve_ram_total;
    float pve_cpu_temp;
    // Wetter (Open-Meteo via Endpoint)
    float temp_c, temp_max, temp_min;
    int rain_pct;
    int wetter_code;      // WMO-Code (fuer Icon-Mapping)
    float wind_kmh;
    // DECT-Homelab-Dose (Fritz)
    float dect_temp_c;
    float dect_lab_w;
    char last_backup[20];
};

bool net_wifi_connect(void);
void net_wifi_off(void);
bool net_fetch_status(DashboardData &out);