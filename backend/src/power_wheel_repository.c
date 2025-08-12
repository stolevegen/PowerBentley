// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#include "power_wheel_repository.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <string.h>

#include "storage.h"
#include "ws_power_wheel.h"
#include "ws_setup.h"
#include "constants.h"
#include "power_wheel_controller.h"
#include "ws_settings.h"

static const char *TAG = "power_wheel_repository";

static const char *PROFILE_LIST_KEY = "profile_list";
static const char *PROFILE_PREFIX = "profile_";
static const char *CURRENT_PROFILE_KEY = "current_profile";
static const char *DEFAULT_PROFILE_ID = "default";
static const char *WIRING_KEY = "wiring";
static const char *SETUP_MODE_KEY = "setup_mode";

// Variables in memory

static float current_speed = 0.0f;

static bool emergency_stop = false;
static bool setup_mode_enabled = false;

stored_profile_t current_profile;

wiring_t current_wiring;

// Broadcasting
static QueueHandle_t broadcast_queue = NULL;
static TaskHandle_t broadcast_task_handle = NULL;
static float previous_speed_broadcasted = -1.0f;

typedef enum
{
    BROADCAST_SPEED,
    BROADCAST_ALL
} broadcast_message_t;

// Structure to store list of profile IDs
typedef struct
{
    char ids[MAX_PROFILES][MAX_PROFILE_ID_LENGTH];
    int count;
} profile_list_t;

static void broadcast_task(void *pvParameter)
{
    broadcast_message_t msg;

    while (true)
    {
        if (xQueueReceive(broadcast_queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch (msg)
            {
            case BROADCAST_SPEED:
                if (!emergency_stop && current_speed != previous_speed_broadcasted)
                {
                    broadcast_current_speed(current_speed);
                    previous_speed_broadcasted = current_speed;
                }
                break;

            case BROADCAST_ALL:
                broadcast_all_values(current_speed, current_profile.id, emergency_stop);
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static void enqueue_broadcast(broadcast_message_t msg)
{
    if (broadcast_queue == NULL)
    {
        ESP_LOGE("PowerWheelRepo", "Broadcast queue not initialized");
        return;
    }

    if (msg == BROADCAST_SPEED)
    {
        // Only queue speed broadcast if there's no pending BROADCAST_ALL
        broadcast_message_t existing_msg;
        if (xQueuePeek(broadcast_queue, &existing_msg, 0) == pdTRUE)
        {
            // If BROADCAST_ALL is already queued, don't downgrade it to BROADCAST_SPEED
            if (existing_msg == BROADCAST_ALL)
            {
                return; // Keep the BROADCAST_ALL message
            }
        }
    }

    // Overwrite the queue with the new message
    xQueueOverwrite(broadcast_queue, &msg);
}

// Helper function to get profile key
static void get_profile_key(const char *profile_id, char *key_buffer, size_t buffer_size)
{
    snprintf(key_buffer, buffer_size, "%s%s", PROFILE_PREFIX, profile_id);
}

// Load profile list from storage
static esp_err_t load_profile_list(profile_list_t *profile_list)
{
    size_t required_size = sizeof(profile_list_t);
    esp_err_t ret = read_blob(PROFILE_LIST_KEY, profile_list, &required_size);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Profile list is empty");
        // Initialize empty list
        memset(profile_list, 0, sizeof(profile_list_t));
        profile_list->count = 0;
    }

    return ret;
}

// Save profile list to storage
static esp_err_t save_profile_list(const profile_list_t *profile_list)
{
    return write_blob(PROFILE_LIST_KEY, profile_list, sizeof(profile_list_t));
}

// Add profile ID to the list
static esp_err_t add_profile_to_list(const char *profile_id)
{
    profile_list_t profile_list;
    load_profile_list(&profile_list);

    // Check if already exists
    for (int i = 0; i < profile_list.count; i++)
    {
        if (strcmp(profile_list.ids[i], profile_id) == 0)
        {
            return ESP_OK; // Already exists
        }
    }

    // Check if we have space
    if (profile_list.count >= MAX_PROFILES)
    {
        ESP_LOGE(TAG, "Maximum number of profiles reached");
        return ESP_ERR_NO_MEM;
    }

    // Add new profile
    strncpy(profile_list.ids[profile_list.count], profile_id, MAX_PROFILE_ID_LENGTH - 1);
    profile_list.ids[profile_list.count][MAX_PROFILE_ID_LENGTH - 1] = '\0';
    profile_list.count++;

    return save_profile_list(&profile_list);
}

// Remove profile ID from the list
static esp_err_t remove_profile_from_list(const char *profile_id)
{
    profile_list_t profile_list;
    load_profile_list(&profile_list);

    // Find and remove the profile
    for (int i = 0; i < profile_list.count; i++)
    {
        if (strcmp(profile_list.ids[i], profile_id) == 0)
        {
            // Shift remaining elements
            for (int j = i; j < profile_list.count - 1; j++)
            {
                strcpy(profile_list.ids[j], profile_list.ids[j + 1]);
            }
            profile_list.count--;
            return save_profile_list(&profile_list);
        }
    }

    return ESP_ERR_NOT_FOUND;
}

// Check if profile exists
static bool profile_exists(const char *profile_id)
{
    char key_buffer[128];
    get_profile_key(profile_id, key_buffer, sizeof(key_buffer));

    profile_t profile;
    size_t required_size = sizeof(profile_t);

    return (read_blob(key_buffer, &profile, &required_size) == ESP_OK);
}

// Initialize default profile if it doesn't exist
static void ensure_default_profile()
{
    if (count_total_profiles() > 0)
    {
        return; // Default profile already exists
    }

    char key_buffer[128];
    get_profile_key(DEFAULT_PROFILE_ID, key_buffer, sizeof(key_buffer));

    profile_t default_profile;
    strcpy(default_profile.name, "Default");
    default_profile.max_forward = 60.0f;
    default_profile.max_backward = 25.0f;
    default_profile.is_drag_mode = false;

    esp_err_t ret = write_blob(key_buffer, &default_profile, sizeof(profile_t));
    if (ret == ESP_OK)
    {
        add_profile_to_list(DEFAULT_PROFILE_ID);
        ESP_LOGI(TAG, "Created default profile");
        set_current_profile(DEFAULT_PROFILE_ID);
        ESP_LOGI(TAG, "Set default profile as current");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to create default profile");
    }
}

// Load a specific profile by ID
static esp_err_t load_profile(const char *profile_id, profile_t *profile)
{
    if (!profile_id || !profile)
    {
        return ESP_ERR_INVALID_ARG;
    }

    char key_buffer[128];
    get_profile_key(profile_id, key_buffer, sizeof(key_buffer));

    size_t required_size = sizeof(profile_t);
    return read_blob(key_buffer, profile, &required_size);
}

// Get the first profile
static esp_err_t get_first_profile(profile_t *profile)
{
    if (!profile)
    {
        return ESP_ERR_INVALID_ARG;
    }

    profile_list_t profile_list;
    load_profile_list(&profile_list);
    if (profile_list.count == 0)
    {
        ensure_default_profile();
    }

    return load_profile(profile_list.ids[0], profile);
}

static esp_err_t load_current_profile(stored_profile_t *profile)
{
    if (!profile)
        return ESP_ERR_INVALID_ARG;

    char profile_id[MAX_PROFILE_ID_LENGTH] = {0};
    size_t required_size = sizeof(profile_id);
    esp_err_t ret = read_blob(CURRENT_PROFILE_KEY, profile_id, &required_size);

    if (ret != ESP_OK || !is_valid_profile_id(profile_id))
    {
        // Fallback to default
        ESP_LOGW(TAG, "Falling back to default profile");
        strncpy(profile->id, DEFAULT_PROFILE_ID, MAX_PROFILE_ID_LENGTH - 1);
        profile->id[MAX_PROFILE_ID_LENGTH - 1] = '\0';
        return get_first_profile(&profile->profile);
    }

    strncpy(profile->id, profile_id, MAX_PROFILE_ID_LENGTH - 1);
    profile->id[MAX_PROFILE_ID_LENGTH - 1] = '\0';
    return load_profile(profile_id, &profile->profile);
}

// Initialize default wiring if it doesn't exist
static void ensure_default_wiring()
{
    wiring_t tmp;
    size_t required_size = sizeof(profile_t);
    if (read_blob(WIRING_KEY, &tmp, &required_size) == ESP_OK)
    {
        // Default wiring already exists
        return;
    }
    wiring_t default_wiring = {
        .is_speed_direction = false,
        .is_adc_throttle = false,
        .min_threshold = 1.0,
        .max_threshold = 2.6,
        .forward_pin = 25,
        .backward_pin = 26,
        .throttle_pin = -1,
        .forward_motor_pin = 18,
        .backward_motor_pin = 19,
    };

    if (write_blob(WIRING_KEY, &default_wiring, sizeof(wiring_t)) == ESP_OK)
    {
        ESP_LOGI(TAG, "Created default wiring");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to create default wiring");
    }
}

static esp_err_t load_wiring(wiring_t *wiring)
{
    if (!wiring)
    {
        ESP_LOGE(TAG, "Invalid wiring pointer");
        return ESP_ERR_INVALID_ARG;
    }

    // Read wiring from storage
    size_t required_size = sizeof(wiring_t);
    esp_err_t ret = read_blob(WIRING_KEY, wiring, &required_size);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to read wiring profile");
        return ret;
    }

    return ESP_OK;
}

// Public

void power_wheel_repository_init(void)
{
    read_bool(SETUP_MODE_KEY, &setup_mode_enabled, true);

    ensure_default_wiring();
    load_wiring(&current_wiring);

    ensure_default_profile();
    // Retrieve values from storage
    load_current_profile(&current_profile);

    // Create broadcast queue (size 1 to automatically overwrite old messages)
    broadcast_queue = xQueueCreate(1, sizeof(broadcast_message_t));
    xTaskCreate(&broadcast_task, "broadcast_task", 4096, NULL, 3, &broadcast_task_handle); // Lower priority than drive
}

esp_err_t set_setup_mode(bool enabled)
{
    setup_mode_enabled = enabled;

    esp_err_t ret = write_bool(SETUP_MODE_KEY, setup_mode_enabled);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to update setup mode: %s", esp_err_to_name(ret));
        return ret;
    }

    broadcast_get_settings();

    return ESP_OK;
}

bool get_setup_mode(void)
{
    return setup_mode_enabled;
}

// Wiring management
esp_err_t set_wiring(const wiring_t *wiring)
{
    if (!wiring)
    {
        ESP_LOGE(TAG, "Invalid wiring profile");
        return ESP_ERR_INVALID_ARG;
    }

    // Save wiring to storage
    esp_err_t ret = write_blob(WIRING_KEY, wiring, sizeof(wiring_t));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to save wiring profile");
        return ret;
    }

    // Update current wiring configuration
    current_wiring.is_speed_direction = wiring->is_speed_direction;
    current_wiring.is_adc_throttle = wiring->is_adc_throttle;
    current_wiring.min_threshold = wiring->min_threshold;
    current_wiring.max_threshold = wiring->max_threshold;
    // Update current wiring pins
    current_wiring.forward_pin = wiring->forward_pin;
    current_wiring.backward_pin = wiring->backward_pin;
    current_wiring.throttle_pin = wiring->throttle_pin;
    current_wiring.forward_motor_pin = wiring->forward_motor_pin;
    current_wiring.backward_motor_pin = wiring->backward_motor_pin;

    power_wheel_controller_reconfigure_wiring();

    // Broadcast the new wiring configuration
    broadcast_wiring_response(wiring);

    return ESP_OK;
}

void get_wiring(wiring_t **wiring)
{
    *wiring = &current_wiring; // Return the pointer to the wiring structure
}

// Speed management
void set_current_speed(float speed)
{
    current_speed = speed;

    enqueue_broadcast(BROADCAST_SPEED);
}

float get_current_speed(void)
{
    return current_speed;
}

// Emergency stop management
void set_emergency_stop(bool enabled)
{
    emergency_stop = enabled;

    enqueue_broadcast(BROADCAST_ALL);
}

bool get_emergency_stop(void)
{
    return emergency_stop;
}

// Check if a profile ID is valid (exists in storage)
bool is_valid_profile_id(const char *profile_id)
{
    if (!profile_id || strlen(profile_id) == 0 || strlen(profile_id) >= MAX_PROFILE_ID_LENGTH)
    {
        return false;
    }

    return profile_exists(profile_id);
}

// Count total profiles
int count_total_profiles()
{
    profile_list_t profile_list;
    load_profile_list(&profile_list);
    return profile_list.count;
}

esp_err_t set_current_profile(const char *profile_id)
{
    if (!profile_exists(profile_id))
    {
        ESP_LOGE(TAG, "Invalid profile ID: %s", profile_id);
        return ESP_ERR_INVALID_ARG;
    }

    if (write_blob(CURRENT_PROFILE_KEY, profile_id, strlen(profile_id) + 1) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set current profile: %s", profile_id);
        return ESP_ERR_INVALID_STATE;
    }

    if (load_current_profile(&current_profile) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to load profile: %s", profile_id);
        return ESP_ERR_INVALID_STATE;
    }

    enqueue_broadcast(BROADCAST_ALL);
    return ESP_OK;
}

void get_current_profile(stored_profile_t **profile)
{
    *profile = &current_profile;
}

esp_err_t get_all_profiles(stored_profile_list_t *list)
{
    if (!list)
        return ESP_ERR_INVALID_ARG;

    profile_list_t profile_ids;
    load_profile_list(&profile_ids);

    list->count = 0;

    for (int i = 0; i < profile_ids.count; i++)
    {
        if (list->count >= MAX_PROFILES)
            break;

        profile_t profile;
        if (load_profile(profile_ids.ids[i], &profile) == ESP_OK)
        {
            strncpy(list->items[list->count].id, profile_ids.ids[i], MAX_PROFILE_ID_LENGTH - 1);
            list->items[list->count].id[MAX_PROFILE_ID_LENGTH - 1] = '\0';
            list->items[list->count].profile = profile;
            list->count++;
        }
    }

    return ESP_OK;
}

esp_err_t save_profile(const char *profile_id, const profile_t *profile, bool *is_new)
{
    if (!profile_id || !profile)
        return ESP_ERR_INVALID_ARG;

    char key_buffer[128];
    get_profile_key(profile_id, key_buffer, sizeof(key_buffer));

    bool exists = profile_exists(profile_id);
    esp_err_t ret = write_blob(key_buffer, profile, sizeof(profile_t));

    if (ret == ESP_OK && !exists)
    {
        ret = add_profile_to_list(profile_id);
    }

    if (is_new)
        *is_new = !exists;

    if (ret != ESP_OK)
        return ret;

    // Update current cached profile if this is the one being set
    if (strcmp(profile_id, current_profile.id) == 0)
    {
        load_profile(profile_id, &current_profile.profile);
    }

    // Broadcast new state
    static stored_profile_list_t list;
    get_all_profiles(&list);
    broadcast_profiles_response(&list);

    return ret;
}

esp_err_t delete_profile(const char *profile_id)
{
    if (!profile_id)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(profile_id, current_profile.id) == 0)
    {
        return ESP_ERR_NOT_ALLOWED; // Cannot delete current profile
    }

    char key_buffer[128];
    get_profile_key(profile_id, key_buffer, sizeof(key_buffer));

    esp_err_t ret = delete_blob(key_buffer);
    if (ret != ESP_OK)
        return ret;

    remove_profile_from_list(profile_id);
    if (ret != ESP_OK)
        return ret;

    // Broadcast new state
    static stored_profile_list_t list;
    get_all_profiles(&list);
    broadcast_profiles_response(&list);

    return ret;
}