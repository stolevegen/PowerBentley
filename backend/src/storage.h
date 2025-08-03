// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#pragma once

#include "esp_err.h"
#include <stdbool.h>

void setup_storage(void);

esp_err_t read_bool(const char *key, bool *value, bool default_value);
esp_err_t write_bool(const char *key, bool value);

esp_err_t read_int(const char *key, int *value, int default_value);
esp_err_t write_int(const char *key, int value);

esp_err_t read_float(const char *key, float *value, float default_value);
esp_err_t write_float(const char *key, float value);

esp_err_t read_blob(const char *key, void *out_value, size_t *required_size);
esp_err_t write_blob(const char *key, const void *value, size_t required_size);
esp_err_t delete_blob(const char *key);