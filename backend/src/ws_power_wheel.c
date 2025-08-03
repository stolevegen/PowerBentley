// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#include "ws_power_wheel.h"
#include "websocket.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"

#include "power_wheel_repository.h"
#include "adc_utils.h"

#define TAG "ws_power_wheel"

static char json[2176];

static void get_all_values(
    char *buffer,
    size_t buffer_size,
    float current_speed,
    char *current_profile_id,
    bool emergency_stop)
{
    snprintf(buffer, buffer_size,
             "{\"type\":\"read_all_response\",\"current_speed\":%f,\"current_profile_id\":\"%s\",\"emergency_stop\":%s}",
             current_speed, current_profile_id, emergency_stop ? "true" : "false");
}

void send_all_values(int sockfd, float current_speed, char *current_profile_id, bool emergency_stop)
{
    get_all_values(json, sizeof(json), current_speed, current_profile_id, emergency_stop);
    ESP_LOGI(TAG, "Send %s", json);
    send_message_sockfd(json, sockfd);
}

void broadcast_all_values(float current_speed, char *current_profile_id, bool emergency_stop)
{
    get_all_values(json, sizeof(json), current_speed, current_profile_id, emergency_stop);
    ESP_LOGI(TAG, "Broadcast %s", json);
    broadcast_message(json);
}

void broadcast_current_speed(float current_speed)
{
    snprintf(json, sizeof(json), "{\"type\":\"read_speed_response\",\"current_speed\":%f}", current_speed);
    ESP_LOGI(TAG, "Broadcast %s", json);
    broadcast_message(json);
}

void ws_handle_read_all(const cJSON *root, int sockfd)
{
    stored_profile_t *profile;
    get_current_profile(&profile);
    send_all_values(sockfd,
                    get_current_speed(),
                    profile->id,
                    get_emergency_stop());
}

void ws_handle_read_throttle(const cJSON *root, int sockfd)
{
    wiring_t *wiring;
    get_wiring(&wiring);

    if (!wiring)
    {
        ESP_LOGE(TAG, "Failed to get wiring configuration for throttle");
        return;
    }

    uint8_t gpio = wiring->is_adc_throttle ? wiring->forward_pin : wiring->throttle_pin;

    float current_throttle;
    if (wiring->is_adc_throttle)
    {
        current_throttle = get_adc_value(gpio) / 1000.f;
    }
    else
    {
        current_throttle = !gpio_get_level(gpio) ? 1.0f : 0.0f;
    }

    snprintf(json, sizeof(json), "{\"type\":\"read_throttle_response\",\"current_throttle\":%f}", current_throttle);
    ESP_LOGI(TAG, "Send %s", json);
    send_message_sockfd(json, sockfd);
}

void ws_handle_emergency_stop(const cJSON *root, int sockfd)
{
    const cJSON *is_enabled = cJSON_GetObjectItem(root, "is_enabled");
    if (!cJSON_IsBool(is_enabled))
        return;
    set_emergency_stop(cJSON_IsTrue(is_enabled));
}
