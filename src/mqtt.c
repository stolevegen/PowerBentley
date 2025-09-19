#include "mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include <stdio.h>

#include "storage.h"
#include "websocket.h"   // to broadcast mqtt_status to the UI

static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t s_client = NULL;
static mqtt_config_t s_cfg;

/* NVS keys */
#define KEY_MQTT_URI   "mqtt_uri"
#define KEY_MQTT_USER  "mqtt_user"
#define KEY_MQTT_PASS  "mqtt_pass"
#define KEY_MQTT_BASE  "mqtt_base"

/* Defaults */
static void defaults(mqtt_config_t *c) {
  memset(c, 0, sizeof(*c));
  snprintf(c->uri, sizeof(c->uri), "mqtt://192.168.1.10:1883");
  snprintf(c->base_topic, sizeof(c->base_topic), "powerjeep");
  // username/password empty by default
}

void mqtt_load_config_from_nvs(mqtt_config_t *out) {
  mqtt_config_t tmp; defaults(&tmp);
  readString(KEY_MQTT_URI,  tmp.uri,  sizeof(tmp.uri),  tmp.uri);
  readString(KEY_MQTT_USER, tmp.username, sizeof(tmp.username), "");
  readString(KEY_MQTT_PASS, tmp.password, sizeof(tmp.password), "");
  readString(KEY_MQTT_BASE, tmp.base_topic, sizeof(tmp.base_topic), tmp.base_topic);
  if (out) *out = tmp;
  s_cfg = tmp;
}

void mqtt_save_config_to_nvs(const mqtt_config_t *cfg) {
  mqtt_config_t c = *cfg;
  writeString(KEY_MQTT_URI,  c.uri);
  writeString(KEY_MQTT_USER, c.username);
  writeString(KEY_MQTT_PASS, c.password);
  writeString(KEY_MQTT_BASE, c.base_topic);
  s_cfg = c;
}

void mqtt_get_config(mqtt_config_t *out) {
  if (out) *out = s_cfg;
}

static void broadcast_status(bool connected) {
  char *msg;
  asprintf(&msg, "{\"type\":\"mqtt_status\",\"connected\":%s,\"uri\":\"%s\",\"base\":\"%s\"}",
           connected ? "true" : "false", s_cfg.uri, s_cfg.base_topic);
  broadcast_message(msg);
  free(msg);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT connected");
      broadcast_status(true);
      // Example subscribe (optional):
      // esp_mqtt_client_subscribe(s_client, "powerjeep/cmd/#", 0);
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGW(TAG, "MQTT disconnected");
      broadcast_status(false);
      break;
    case MQTT_EVENT_ERROR:
      ESP_LOGE(TAG, "MQTT error");
      break;
    default:
      break;
  }
}

void mqtt_start(void)
{
  if (s_client) return;

  if (s_cfg.uri[0] == '\0') {
    ESP_LOGW(TAG, "No MQTT URI set, not starting.");
    broadcast_status(false);
    return;
  }

  esp_mqtt_client_config_t cfg = {
    .broker.address.uri = s_cfg.uri,
    // Username/password if provided
    .credentials.username = (s_cfg.username[0] ? s_cfg.username : NULL),
    .credentials.authentication.password = (s_cfg.password[0] ? s_cfg.password : NULL),
    // For TLS (mqtts://) you’d add certs here
  };

  s_client = esp_mqtt_client_init(&cfg);
  esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  esp_mqtt_client_start(s_client);
  ESP_LOGI(TAG, "MQTT starting → %s (base: %s)", s_cfg.uri, s_cfg.base_topic);
}

void mqtt_stop(void)
{
  if (!s_client) return;
  esp_mqtt_client_stop(s_client);
  esp_mqtt_client_destroy(s_client);
  s_client = NULL;
  broadcast_status(false);
  ESP_LOGI(TAG, "MQTT stopped");
}

void mqtt_apply_config_and_restart(void)
{
  bool was_running = (s_client != NULL);
  if (was_running) mqtt_stop();
  mqtt_start();
}

bool mqtt_is_running(void)
{
  return s_client != NULL;
}

static char tmp_topic[160];

static const char* full_topic(const char *suffix) {
  if (s_cfg.base_topic[0]) {
    snprintf(tmp_topic, sizeof(tmp_topic), "%s/%s", s_cfg.base_topic, suffix);
    return tmp_topic;
  }
  return suffix;
}

int mqtt_publish_str(const char *topic, const char *payload, int qos, bool retain)
{
  if (!s_client) return -1;
  return esp_mqtt_client_publish(s_client, full_topic(topic), payload, 0, qos, retain ? 1 : 0);
}

int mqtt_publish_f(const char *topic, float value, int qos, bool retain)
{
  if (!s_client) return -1;
  char buf[32];
  int n = snprintf(buf, sizeof(buf), "%.2f", value);
  return esp_mqtt_client_publish(s_client, full_topic(topic), buf, n, qos, retain ? 1 : 0);
}
