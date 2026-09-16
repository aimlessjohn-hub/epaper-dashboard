/**
 * net.cpp - WiFi + HTTP-Client fuer :8123/status?compact=1
 */
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "net.h"
#include "config.h"

bool net_wifi_connect(void) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(true);              // modem sleep waehrend Wachphase ok
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > WIFI_TIMEOUT_MS) return false;
        delay(200);
    }
    return true;
}

void net_wifi_off(void) {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
}

static bool http_get_json(const char *url, JsonDocument &doc) {
    HTTPClient http;
    http.setConnectTimeout(HTTP_TIMEOUT_MS);
    http.setTimeout(HTTP_TIMEOUT_MS);
    if (!http.begin(url)) return false;
    int code = http.GET();
    if (code != 200) { http.end(); return false; }
    // Antwort kann bis ~4 KB sein (compact) -> PSRAM-Stream in String
    String body = http.getString();
    http.end();
    DeserializationError err = deserializeJson(doc, body);
    return err == DeserializationError::Ok;
}

bool net_fetch_status(DashboardData &out) {
    JsonDocument doc;
    if (!http_get_json(STATUS_URL, doc)) return false;

    // Feldnamen exakt nach status_endpoint.py compact=1 (16.9. live verifiziert):
    // waschmaschine{on,w}, pve_host{cpu_pct,ram_gb,ram_total_gb,...},
    // wetter{temp_c,code,wind_kmh,t_max,t_min,rain_pct}, homelab_dose{temp_c,lab_w}
    strlcpy(out.generated, doc["generated"] | "?", sizeof(out.generated));

    JsonObject wm = doc["waschmaschine"];
    out.wasser_on = wm["on"] | false;
    out.wasser_w  = wm["w"] | 0.0f;
    snprintf(out.wasser_state, sizeof(out.wasser_state), "%s",
             out.wasser_on ? (out.wasser_w > 2.0f ? "laeuft" : "fertig/idle") : "aus");

    JsonObject pve = doc["pve_host"];
    out.pve_cpu_pct   = pve["cpu_pct"] | 0.0f;
    out.pve_ram_gb    = pve["ram_gb"] | 0.0f;
    out.pve_ram_total = pve["ram_total_gb"] | 0.0f;
    out.pve_cpu_temp  = pve["temp_k10temp_c"] | 0.0f;

    out.vms_running = doc["vms_running"] | 0;
    out.vms_total   = doc["vms_total"]   | 0;

    JsonObject wetter = doc["wetter"];
    out.temp_c   = wetter["temp_c"] | 0.0f;
    out.temp_max = wetter["t_max"]  | 0.0f;
    out.temp_min = wetter["t_min"]  | 0.0f;
    out.rain_pct = wetter["rain_pct"] | 0;
    out.wetter_code = wetter["code"] | -1;
    out.wind_kmh  = wetter["wind_kmh"] | 0.0f;

    JsonObject hd = doc["homelab_dose"];
    out.dect_temp_c = hd["temp_c"] | 0.0f;
    out.dect_lab_w  = hd["lab_w"] | 0.0f;

    strlcpy(out.last_backup, doc["letzter_backup_lauf"] | "?", sizeof(out.last_backup));
    return true;
}