// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#pragma once

#include <stdint.h>
#include <stdbool.h>

bool adc_calibration_init(void);

void adc_init_single_pin(uint8_t pin);
void adc_init_two_pins(uint8_t forward_pin, uint8_t backward_pin);

uint32_t get_adc_value(uint8_t gpio);
