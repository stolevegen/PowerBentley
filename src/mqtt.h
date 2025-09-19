#pragma once
#include <stdbool.h>

/* Start/stop controlled by STA link events (wifi.c) */
void mqtt_start(void);
void mqtt_stop(void);
bool mqtt_is_running(void);

/* Configuration (NVS-backed) */
typedef struct {
  char uri[128];      // e.g. "mqtt://192.168.1.10:1883" or "mqtts://host:8883"
  char username[64];
  char password[64];
  char base_topic[64]; // e.g. "powerbentley"
} mqtt_config_t;

void mqtt_load_config_from_nvs(mqtt_config_t *out);             // fills defaults if unset
void mqtt_save_config_to_nvs(const mqtt_config_t *cfg);         // persists
void mqtt_apply_config_and_restart(void);                       // stop → reinit → start
void mqtt_get_config(mqtt_config_t *out);                       // current in-RAM copy

/* Publish helpers */
int mqtt_publish_str(const char *topic, const char *payload, int qos, bool retain);
int mqtt_publish_f(const char *topic, float value, int qos, bool retain);
