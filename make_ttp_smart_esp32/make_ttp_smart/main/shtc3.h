#pragma once

#include "esp_err.h"

typedef struct {
    float temperature;
    float humidity;
    esp_err_t error;
} shtc3_t;

esp_err_t shtc3_init(void);
shtc3_t shtc3_static_read(void);
void shtc3_task_f(void *arg);