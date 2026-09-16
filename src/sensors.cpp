/**
 * sensors.cpp - SHTC3 (Temp/Feuchte) via I2C (GPIO47/48, Addr 0x70)
 * Wake: 0x3517, Measure normal T first: 0x7866, Read 6 B.
 * CRC: polynomial 0x31, init 0xFF (wie Datasheet).
 */
#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "sensors.h"

static const uint8_t SHTC3_WAKE[2]   = {0x35, 0x17};
static const uint8_t SHTC3_MEAS_T[2] = {0x78, 0x66};  // normal, T first
static const uint8_t SHTC3_SLEEP[2]  = {0xB0, 0x98};

static bool crc8_ok(const uint8_t *d2) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < 2; i++) {
        crc ^= d2[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc == d2[2];
}

void sensors_init(void) {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);
    Wire.beginTransmission(SHTC3_ADDR);
    Wire.write(SHTC3_WAKE, 2);
    Wire.endTransmission();
    delayMicroseconds(300);          // wakeup time per datasheet
}

void sensors_read_shtc3(float *t_c, float *rh_pct) {
    Wire.beginTransmission(SHTC3_ADDR);
    Wire.write(SHTC3_MEAS_T, 2);
    if (Wire.endTransmission() != 0) { *t_c = 0; *rh_pct = 0; return; }
    delay(13);                        // typ. measurement time
    if (Wire.requestFrom((int)SHTC3_ADDR, 6) != 6) { *t_c = 0; *rh_pct = 0; return; }
    uint8_t b[6];
    for (int i = 0; i < 6; i++) b[i] = Wire.read();
    if (!crc8_ok(b) || !crc8_ok(b + 3)) { *t_c = 0; *rh_pct = 0; return; }
    uint16_t raw_t = (b[0] << 8) | b[1];
    uint16_t raw_h = (b[3] << 8) | b[4];
    *t_c = -45.0f + 175.0f * raw_t / 65535.0f;
    *rh_pct = 100.0f * raw_h / 65535.0f;
    Wire.beginTransmission(SHTC3_ADDR);
    Wire.write(SHTC3_SLEEP, 2);
    Wire.endTransmission();
}