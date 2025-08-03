// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#define MAX_PROFILE_ID_LENGTH 32
#define MAX_PROFILES 5

// Profile structure stored
typedef struct
{
    char name[32];
    float max_forward;
    float max_backward;
    bool is_drag_mode;
} profile_t;

typedef struct
{
    char id[MAX_PROFILE_ID_LENGTH];
    profile_t profile;
} stored_profile_t;

typedef struct
{
    stored_profile_t items[MAX_PROFILE_ID_LENGTH];
    int count;
} stored_profile_list_t;

typedef struct
{
    bool is_speed_direction;
    int forward_pin;
    int backward_pin;
    int throttle_pin; // Only used if is_speed_direction is true
    int forward_motor_pin;
    int backward_motor_pin;
    bool is_adc_throttle; // Whether the throttle is an ADC input
    float min_threshold;  // Min voltage threshold for ADC, below is 0%
    float max_threshold;  // Max voltage threshold for ADC, above is 100%
} wiring_t;

// Initialize the repository
void power_wheel_repository_init(void);

// Wiring management
esp_err_t set_setup_mode(bool enabled);
bool get_setup_mode(void);

esp_err_t set_wiring(const wiring_t *wiring);
void get_wiring(wiring_t **wiring);

// Speed management
void set_current_speed(float speed);
float get_current_speed(void);

// Emergency stop management
void set_emergency_stop(bool enabled);
bool get_emergency_stop(void);

// Profile management
bool is_valid_profile_id(const char *profile_id);
int count_total_profiles();
esp_err_t set_current_profile(const char *profile_id);
void get_current_profile(stored_profile_t **profile);
esp_err_t get_all_profiles(stored_profile_list_t *list);
esp_err_t save_profile(const char *profile_id, const profile_t *profile, bool *is_new);
esp_err_t delete_profile(const char *profile_id);
