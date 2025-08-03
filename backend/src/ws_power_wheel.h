// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#pragma once

#include "cJSON.h"
#include <stdbool.h>

void send_all_values(int sockfd, float current_speed, char *current_config_id, bool emergency_stop);
void broadcast_all_values(float current_speed, char *current_config_id, bool emergency_stop);
void broadcast_current_speed(float current_speed);

void ws_handle_read_all(const cJSON *root, int sockfd);
void ws_handle_read_throttle(const cJSON *root, int sockfd);
void ws_handle_emergency_stop(const cJSON *root, int sockfd);
