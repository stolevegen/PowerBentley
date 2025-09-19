#include "mqtt.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>

static const char *TAG = "mqtt";
static esp_mqtt_client_handle_t s_client = NULL;

/* For now: hardcode your broker; later load from NVS or web UI */
#define MQTT_URI "mqtt://192.168.40.15:1883"
// Example with auth / TLS later:
// .broker.address.uri = "mqtts://broker.example.com:8883"
// .credentials.username = "...", .credentials.authentication.password = "..."

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        // Example: subscribe if you need inbound commands
        // esp_mqtt_client_subscribe(s_client, "powerjeep/cmd/#", 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "Published msg_id=%d", event->msg_id);
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
    if (s_client) {
        ESP_LOGD(TAG, "mqtt_start: already running");
        return;
    }
    const esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_URI,
        // .session.keepalive = 60,
        // .network.disable_auto_reconnect = false, // default: auto-reconnect enabled
    };
    s_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
    ESP_LOGI(TAG, "MQTT client started → %s", MQTT_URI);
}

void mqtt_stop(void)
{
    if (!s_client) return;
    esp_mqtt_client_stop(s_client);
    esp_mqtt_client_destroy(s_client);
    s_client = NULL;
    ESP_LOGI(TAG, "MQTT client stopped");
}

bool mqtt_is_running(void)
{
    return s_client != NULL;
}

int mqtt_publish_str(const char *topic, const char *payload, int qos, bool retain)
{
    if (!s_client) return -1;
    return esp_mqtt_client_publish(s_client, topic, payload, 0, qos, retain ? 1 : 0);
}

int mqtt_publish_f(const char *topic, float value, int qos, bool retain)
{
    if (!s_client) return -1;
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "%.2f", value);
    return esp_mqtt_client_publish(s_client, topic, buf, n, qos, retain ? 1 : 0);
}
