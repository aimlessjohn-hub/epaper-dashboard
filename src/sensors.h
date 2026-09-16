#pragma once
#include <stdint.h>

void sensors_init(void);
void sensors_read_shtc3(float *t_c, float *rh_pct);