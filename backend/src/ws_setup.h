// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#pragma once

#include "cJSON.h"
#include <stdbool.h>

#include "power_wheel_repository.h"

void ws_handle_setup_mode(const cJSON *root, int sockfd);

void send_wiring_response(int sockfd, const wiring_t *wiring);
void broadcast_wiring_response(const wiring_t *wiring);
void ws_handle_get_wiring(const cJSON *root, int sockfd);
void ws_handle_set_wiring(const cJSON *root, int sockfd);

void ws_handle_set_current_profile(const cJSON *root, int sockfd);
void ws_handle_get_profiles(const cJSON *root, int sockfd);
void ws_handle_save_profile(const cJSON *root, int sockfd);
void ws_handle_delete_profile(const cJSON *root, int sockfd);
