// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#include "ws_setup.h"

#include "websocket.h"
#include "storage.h"
#include "esp_log.h"
#include <string.h>
#include "utils.h"

static const char *TAG = "ws_setup";
static char json[2048];
static stored_profile_list_t list;

static void get_wiring_json(char *buffer, size_t buffer_size, const wiring_t *wiring)
{
    if (!buffer || !wiring || buffer_size < 64)
    {
        ESP_LOGW(TAG, "Invalid arguments for get_wiring_json");
        return;
    }

    if (wiring->is_speed_direction)
    {
        snprintf(buffer, buffer_size,
                 "{\"type\":\"wiring_response\",\"mode\":\"speed_direction\",\"is_adc_throttle\":%s,\"min_threshold\":%f,\"max_threshold\":%f,\"inputs\":{\"forward\":%d,\"backward\":%d,\"throttle\":%d},\"outputs\":{\"forward_motor\":%d,\"backward_motor\":%d}}",
                 wiring->is_adc_throttle ? "true" : "false", wiring->min_threshold, wiring->max_threshold, wiring->forward_pin, wiring->backward_pin, wiring->throttle_pin, wiring->forward_motor_pin, wiring->backward_motor_pin);
    }
    else
    {
        snprintf(buffer, buffer_size,
                 "{\"type\":\"wiring_response\",\"mode\":\"dual_input\",\"is_adc_throttle\":%s,\"min_threshold\":%f,\"max_threshold\":%f,\"inputs\":{\"forward\":%d,\"backward\":%d},\"outputs\":{\"forward_motor\":%d,\"backward_motor\":%d}}",
                 wiring->is_adc_throttle ? "true" : "false", wiring->min_threshold, wiring->max_threshold, wiring->forward_pin, wiring->backward_pin, wiring->forward_motor_pin, wiring->backward_motor_pin);
    }
}

static bool get_profile_json(char *buffer, size_t buffer_size, const char *id, const profile_t *profile)
{
    if (!buffer || !profile || buffer_size < 64)
    {
        ESP_LOGW(TAG, "Invalid arguments for get_profile_json");
        return false;
    }

    // Escape string fields that might contain special characters
    char escaped_name[64];
    if (!json_escape_string(profile->name, escaped_name, sizeof(escaped_name)))
    {
        ESP_LOGW(TAG, "Failed to escape profile name");
        return false;
    }

    int written = snprintf(buffer, buffer_size,
                           "{\"id\":\"%s\",\"name\":%s,\"maxForward\":%f,\"maxBackward\":%f,\"isDragMode\":%s}",
                           id, escaped_name, profile->max_forward, profile->max_backward, profile->is_drag_mode ? "true" : "false");

    if (written < 0 || written >= buffer_size)
    {
        ESP_LOGW(TAG, "Profile JSON truncated or failed");
        return false;
    }

    return true;
}

static bool get_profiles_json(char *buffer, size_t buffer_size, const stored_profile_list_t *profiles)
{
    if (!buffer || buffer_size < 64)
        return false;

    size_t pos = 0;

    // Start JSON object and array
    if (!safe_append(buffer, buffer_size, &pos, "{\"type\":\"profiles_data\",\"profiles\":["))
    {
        return false;
    }

    for (int i = 0; i < profiles->count; i++)
    {
        char profile_json[512]; // Adjust based on profile size
        if (!get_profile_json(profile_json, sizeof(profile_json),
                              profiles->items[i].id, &profiles->items[i].profile))
        {
            ESP_LOGW(TAG, "Failed to serialize profile %d", i);
            continue;
        }

        // Add comma separator for subsequent items
        if (i > 0)
        {
            if (!safe_append(buffer, buffer_size, &pos, ","))
            {
                return false;
            }
        }

        if (!safe_append(buffer, buffer_size, &pos, profile_json))
        {
            return false;
        }
    }

    // Close array and object
    if (!safe_append(buffer, buffer_size, &pos, "]}"))
    {
        return false;
    }

    return true;
}

void ws_handle_setup_mode(const cJSON *root, int sockfd)
{
    const cJSON *is_enabled = cJSON_GetObjectItem(root, "is_enabled");
    if (!cJSON_IsBool(is_enabled))
        return;
    set_setup_mode(cJSON_IsTrue(is_enabled));
}

void send_wiring_response(int sockfd, const wiring_t *wiring)
{
    get_wiring_json(json, sizeof(json), wiring);
    ESP_LOGI(TAG, "Send wiring response: %s", json);
    send_message_sockfd(json, sockfd);
}

void broadcast_wiring_response(const wiring_t *wiring)
{
    get_wiring_json(json, sizeof(json), wiring);
    ESP_LOGI(TAG, "Broadcast wiring: %s", json);
    broadcast_message(json);
}

void ws_handle_get_wiring(const cJSON *root, int sockfd)
{
    wiring_t *wiring;
    get_wiring(&wiring);

    send_wiring_response(sockfd, wiring);
}

void ws_handle_set_wiring(const cJSON *root, int sockfd)
{
    const cJSON *mode_node = cJSON_GetObjectItem(root, "mode");
    const cJSON *adc_throttle_node = cJSON_GetObjectItem(root, "is_adc_throttle");
    const cJSON *min_threshold_node = cJSON_GetObjectItem(root, "min_threshold");
    const cJSON *max_threshold_node = cJSON_GetObjectItem(root, "max_threshold");
    const cJSON *inputs_node = cJSON_GetObjectItem(root, "inputs");
    const cJSON *outputs_node = cJSON_GetObjectItem(root, "outputs");
    if (!cJSON_IsString(mode_node) ||
        !cJSON_IsBool(adc_throttle_node) ||
        !cJSON_IsObject(inputs_node) ||
        !cJSON_IsObject(outputs_node) ||
        !cJSON_IsNumber(min_threshold_node) ||
        !cJSON_IsNumber(max_threshold_node))
    {
        ESP_LOGE(TAG, "Invalid json");
        return;
    }
    const bool is_speed_direction = strcmp(mode_node->valuestring, "speed_direction") == 0;

    const cJSON *inputs_forward_node = cJSON_GetObjectItem(inputs_node, "forward");
    const cJSON *inputs_backward_node = cJSON_GetObjectItem(inputs_node, "backward");
    const cJSON *inputs_throttle_node = cJSON_GetObjectItem(inputs_node, "throttle");
    if (!cJSON_IsNumber(inputs_forward_node) || !cJSON_IsNumber(inputs_backward_node) || (is_speed_direction && !cJSON_IsNumber(inputs_throttle_node)))
    {
        ESP_LOGE(TAG, "Invalid inputs json");
        return;
    }

    const cJSON *outputs_forward_motor_node = cJSON_GetObjectItem(outputs_node, "forward_motor");
    const cJSON *outputs_backward_motor_node = cJSON_GetObjectItem(outputs_node, "backward_motor");
    if (!cJSON_IsNumber(outputs_forward_motor_node) || !cJSON_IsNumber(outputs_backward_motor_node))
    {
        ESP_LOGE(TAG, "Invalid outputs json");
        return;
    }

    wiring_t wiring;
    wiring.is_speed_direction = is_speed_direction;
    wiring.is_adc_throttle = cJSON_IsTrue(adc_throttle_node);
    wiring.min_threshold = min_threshold_node->valuedouble;
    wiring.max_threshold = max_threshold_node->valuedouble;
    wiring.forward_pin = inputs_forward_node->valueint;
    wiring.backward_pin = inputs_backward_node->valueint;
    wiring.throttle_pin = is_speed_direction ? inputs_throttle_node->valueint : -1;
    wiring.forward_motor_pin = outputs_forward_motor_node->valueint;
    wiring.backward_motor_pin = outputs_backward_motor_node->valueint;
    if (is_speed_direction)
    {
        ESP_LOGI(TAG, "Setting wiring: Speed/Direction mode with forward pin %d, backward pin %d, throttle pin %d", wiring.forward_pin, wiring.backward_pin, wiring.throttle_pin);
    }
    else
    {
        ESP_LOGI(TAG, "Setting wiring: Direction mode with forward pin %d, backward pin %d", wiring.forward_pin, wiring.backward_pin);
    }
    esp_err_t ret = set_wiring(&wiring);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set wiring: %s", esp_err_to_name(ret));
        snprintf(json, sizeof(json),
                 "{\"type\":\"set_wiring_response\",\"status\":\"error\",\"message\":\"Failed to set wiring: %s\"}",
                 esp_err_to_name(ret));
        send_message_sockfd(json, sockfd);
    }

    // set_wiring will broadcast the new wiring configuration
}

void ws_handle_set_current_profile(const cJSON *root, int sockfd)
{
    const cJSON *profile_id_node = cJSON_GetObjectItem(root, "profile_id");
    if (!cJSON_IsString(profile_id_node))
    {
        ESP_LOGE(TAG, "Invalid or missing profile_id");
        return;
    }
    esp_err_t ret = set_current_profile(profile_id_node->valuestring);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set current profile: %s", esp_err_to_name(ret));
        snprintf(json, sizeof(json),
                 "{\"type\":\"set_current_profile_response\",\"status\":\"error\",\"message\":\"Failed to set current profile: %s\"}",
                 esp_err_to_name(ret));
        send_message_sockfd(json, sockfd);
        return;
    }

    // Setting profile broadcast the new values
}

void send_profiles_response(int sockfd, const stored_profile_list_t *profiles)
{
    if (get_profiles_json(json, sizeof(json), profiles))
    {
        ESP_LOGI(TAG, "Send profiles response: %s", json);
        send_message_sockfd(json, sockfd);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to construct profiles JSON");
    }
}

void broadcast_profiles_response(const stored_profile_list_t *profiles)
{
    if (get_profiles_json(json, sizeof(json), profiles))
    {
        ESP_LOGI(TAG, "Broadcast profiles: %s", json);
        broadcast_message(json);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to construct profiles JSON");
    }
}

void ws_handle_get_profiles(const cJSON *root, int sockfd)
{
    ESP_LOGI(TAG, "Getting profiles");

    if (get_all_profiles(&list) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get profiles");
        return;
    }

    send_profiles_response(sockfd, &list);
}

void ws_handle_save_profile(const cJSON *root, int sockfd)
{
    cJSON *profile_json = cJSON_GetObjectItem(root, "profile");
    if (!profile_json)
    {
        ESP_LOGE(TAG, "Invalid json");
        return;
    }

    const cJSON *id_node = cJSON_GetObjectItem(profile_json, "id");
    const cJSON *name_node = cJSON_GetObjectItem(profile_json, "name");
    const cJSON *max_forward_node = cJSON_GetObjectItem(profile_json, "maxForward");
    const cJSON *max_backward_node = cJSON_GetObjectItem(profile_json, "maxBackward");
    const cJSON *is_drag_mode_node = cJSON_GetObjectItem(profile_json, "isDragMode");

    if (!cJSON_IsString(id_node) ||
        !cJSON_IsString(name_node) ||
        !cJSON_IsNumber(max_forward_node) ||
        !cJSON_IsNumber(max_backward_node) ||
        !cJSON_IsBool(is_drag_mode_node))
    {
        ESP_LOGE(TAG, "Invalid json");
        return;
    }

    profile_t profile = {0};
    strncpy(profile.name, name_node->valuestring, sizeof(profile.name) - 1);
    profile.max_forward = max_forward_node->valuedouble;
    profile.max_backward = max_backward_node->valuedouble;
    profile.is_drag_mode = cJSON_IsTrue(is_drag_mode_node);

    bool is_new;
    esp_err_t ret = save_profile(id_node->valuestring, &profile, &is_new);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save profile: %s", esp_err_to_name(ret));
        snprintf(json, sizeof(json),
                 "{\"type\":\"error\",\"message\":\"Failed to save profile: %s\"}",
                 esp_err_to_name(ret));
        send_message_sockfd(json, sockfd);
        return;
    }

    // save_profile will broadcast the new profile configuration
}

void ws_handle_delete_profile(const cJSON *root, int sockfd)
{
    const char *profile_id = cJSON_GetStringValue(cJSON_GetObjectItem(root, "profile_id"));

    if (!profile_id)
    {
        ESP_LOGE(TAG, "Invalid json");
        return;
    }

    esp_err_t ret = delete_profile(profile_id);
    if (ret == ESP_ERR_NOT_ALLOWED)
    {
        ESP_LOGE(TAG, "Cannot delete the current profile");
        snprintf(json, sizeof(json),
                 "{\"type\":\"error\",\"message\":\"Cannot delete the current profile. Please select another profile first.\"}");
        send_message_sockfd(json, sockfd);
        return;
    }

    // delete_profile will broadcast the new profile configuration
}
