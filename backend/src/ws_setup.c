// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#include "ws_setup.h"

#include "websocket.h"
#include "storage.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ws_setup";
static char json[2048];
static stored_profile_list_t list;

// Helper function to create profile JSON object
static cJSON *profile_to_json(const char *profile_id, const profile_t *profile)
{
    cJSON *json_profile = cJSON_CreateObject();
    if (!json_profile)
        return NULL;

    cJSON_AddStringToObject(json_profile, "id", profile_id);
    cJSON_AddStringToObject(json_profile, "name", profile->name);
    cJSON_AddNumberToObject(json_profile, "maxForward", profile->max_forward);
    cJSON_AddNumberToObject(json_profile, "maxBackward", profile->max_backward);
    cJSON_AddNumberToObject(json_profile, "isDragMode", profile->is_drag_mode);

    return json_profile;
}

static void get_wiring_json(char *buffer, size_t buffer_size, const wiring_t *wiring)
{
    if (wiring->is_speed_direction)
    {
        snprintf(json, sizeof(json),
                 "{\"type\":\"wiring_response\",\"mode\":\"speed_direction\",\"is_adc_throttle\":%s,\"min_threshold\":%f,\"max_threshold\":%f,\"inputs\":{\"forward\":%d,\"backward\":%d,\"throttle\":%d},\"outputs\":{\"forward_motor\":%d,\"backward_motor\":%d}}",
                 wiring->is_adc_throttle ? "true" : "false", wiring->min_threshold, wiring->max_threshold, wiring->forward_pin, wiring->backward_pin, wiring->throttle_pin, wiring->forward_motor_pin, wiring->backward_motor_pin);
    }
    else
    {
        snprintf(json, sizeof(json),
                 "{\"type\":\"wiring_response\",\"mode\":\"dual_input\",\"is_adc_throttle\":%s,\"min_threshold\":%f,\"max_threshold\":%f,\"inputs\":{\"forward\":%d,\"backward\":%d},\"outputs\":{\"forward_motor\":%d,\"backward_motor\":%d}}",
                 wiring->is_adc_throttle ? "true" : "false", wiring->min_threshold, wiring->max_threshold, wiring->forward_pin, wiring->backward_pin, wiring->forward_motor_pin, wiring->backward_motor_pin);
    }
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

void ws_handle_get_profiles(const cJSON *root, int sockfd)
{
    ESP_LOGI(TAG, "Getting profiles");

    if (get_all_profiles(&list) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get profiles");
        return;
    }

    ESP_LOGI(TAG, "Got profiles");

    cJSON *response = cJSON_CreateObject();
    cJSON *profiles = cJSON_CreateArray();
    if (!response || !profiles)
        return;

    for (int i = 0; i < list.count; i++)
    {
        cJSON *json_profiles = profile_to_json(list.items[i].id, &list.items[i].profile);
        if (json_profiles)
        {
            cJSON_AddItemToArray(profiles, json_profiles);
        }
    }
    ESP_LOGI(TAG, "JSON profiles");

    cJSON_AddStringToObject(response, "type", "profiles_data");
    cJSON_AddItemToObject(response, "profiles", profiles);

    char *json_string = cJSON_Print(response);
    if (json_string)
    {
        send_message_sockfd(json_string, sockfd);
        free(json_string);
    }

    cJSON_Delete(response);
}

void ws_handle_save_profile(const cJSON *root, int sockfd)
{
    cJSON *profile_json = cJSON_GetObjectItem(root, "profile");
    if (!profile_json)
        return;

    const char *profile_id = cJSON_GetStringValue(cJSON_GetObjectItem(profile_json, "id"));
    const char *name = cJSON_GetStringValue(cJSON_GetObjectItem(profile_json, "name"));
    float max_forward = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(profile_json, "maxForward"));
    float max_backward = (float)cJSON_GetNumberValue(cJSON_GetObjectItem(profile_json, "maxBackward"));
    bool is_drag_mode = (float)cJSON_IsTrue(cJSON_GetObjectItem(profile_json, "isDragMode"));

    if (!profile_id || !name)
        return;

    profile_t profile = {0};
    strncpy(profile.name, name, sizeof(profile.name) - 1);
    profile.max_forward = max_forward;
    profile.max_backward = max_backward;
    profile.is_drag_mode = is_drag_mode;

    bool is_new;
    esp_err_t ret = save_profile(profile_id, &profile, &is_new);

    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "profile_saved");
    cJSON_AddStringToObject(response, "profile_id", profile_id);

    if (ret == ESP_OK)
    {
        cJSON_AddStringToObject(response, "status", "success");
    }
    else
    {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Failed to save profile");
    }

    char *json_string = cJSON_Print(response);
    if (json_string)
    {
        send_message_sockfd(json_string, sockfd);
        free(json_string);
    }
    cJSON_Delete(response);
}

void ws_handle_delete_profile(const cJSON *root, int sockfd)
{
    const char *profile_id = cJSON_GetStringValue(cJSON_GetObjectItem(root, "profile_id"));
    cJSON *response = cJSON_CreateObject();

    if (!profile_id)
        return;

    if (!is_valid_profile_id(profile_id))
    {
        cJSON_AddStringToObject(response, "type", "profile_deleted");
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Cannot delete this profile");
        cJSON_AddStringToObject(response, "profile_id", profile_id);
    }
    else if (count_total_profiles() <= 1)
    {
        cJSON_AddStringToObject(response, "type", "profile_deleted");
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Cannot delete last profile");
        cJSON_AddStringToObject(response, "profile_id", profile_id);
    }
    else
    {
        esp_err_t ret = delete_profile(profile_id);
        if (ret == ESP_OK)
        {
            cJSON_AddStringToObject(response, "type", "profile_deleted");
            cJSON_AddStringToObject(response, "status", "success");
            cJSON_AddStringToObject(response, "profileId", profile_id);
        }
        else
        {
            cJSON_AddStringToObject(response, "type", "profile_deleted");
            cJSON_AddStringToObject(response, "status", "error");
            cJSON_AddStringToObject(response, "message", "Failed to delete profile");
            cJSON_AddStringToObject(response, "profileId", profile_id);
        }
    }

    char *json_string = cJSON_Print(response);
    if (json_string)
    {
        send_message_sockfd(json_string, sockfd);
        free(json_string);
    }

    cJSON_Delete(response);
}
