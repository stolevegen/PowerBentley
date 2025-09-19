#include "wifi.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "storage.h"   // <— NVS helpers
#include "mqtt.h"      // <— optional: start/stop on link events (ok if you add later)

/* --------- AP (captive portal) --------- */
#define AP_SSID        "PowerBentley"
#define AP_PASS        "Bentley!"
#define AP_CHANNEL     10
#define AP_MAX_CONN    4

/* NVS keys for STA creds */
#define KEY_STA_SSID   "sta_ssid"
#define KEY_STA_PASS   "sta_pass"

/* Buffers for loaded creds */
static char g_sta_ssid[33];    // 32 + NUL
static char g_sta_pass[65];    // 64 + NUL

static const char *TAG = "wifi_apsta";

/* Forward */
static void apply_sta_config_and_connect(void);

/* Wi-Fi + IP event handlers */
static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT) {
        switch (id) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "STA start → connecting…");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "STA disconnected → stopping MQTT and retrying…");
            /* Optional */
            #ifdef MQTT_H
            mqtt_stop();
            #endif
            esp_wifi_connect();
            break;
        case WIFI_EVENT_AP_START:
            ESP_LOGI(TAG, "AP started (SSID: %s, channel: %d)", AP_SSID, AP_CHANNEL);
            break;
        case WIFI_EVENT_AP_STACONNECTED: {
            wifi_event_ap_staconnected_t* ev = (wifi_event_ap_staconnected_t*)data;
            ESP_LOGI(TAG, "AP client joined: " MACSTR, MAC2STR(ev->mac));
            break;
        }
        case WIFI_EVENT_AP_STADISCONNECTED: {
            wifi_event_ap_stadisconnected_t* ev = (wifi_event_ap_stadisconnected_t*)data;
            ESP_LOGI(TAG, "AP client left: " MACSTR, MAC2STR(ev->mac));
            break;
        }
        default: break;
        }
    }
}

static void ip_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* e = (ip_event_got_ip_t*)data;
        ESP_LOGI(TAG, "STA got IP: " IPSTR, IP2STR(&e->ip_info.ip));
        /* Optional: start MQTT when link is usable */
        #ifdef MQTT_H
        mqtt_start();
        #endif
    }
}

void setup_softap(void)
{
    /* Init netif + event loop once per app */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Create default netifs */
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register handlers */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    /* AP config (captive portal side) */
    wifi_config_t ap_cfg = { 0 };
    strncpy((char*)ap_cfg.ap.ssid, AP_SSID, sizeof(ap_cfg.ap.ssid) - 1);
    ap_cfg.ap.channel        = AP_CHANNEL;
    strncpy((char*)ap_cfg.ap.password, AP_PASS, sizeof(ap_cfg.ap.password) - 1);
    ap_cfg.ap.max_connection = AP_MAX_CONN;
    ap_cfg.ap.authmode       = WIFI_AUTH_WPA_WPA2_PSK;
    if (strlen(AP_PASS) == 0) ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

    /* Load STA creds from NVS (empty by default) */
    readString(KEY_STA_SSID, g_sta_ssid, sizeof(g_sta_ssid), "");
    readString(KEY_STA_PASS, g_sta_pass, sizeof(g_sta_pass), "");

    /* Start AP+STA */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* If we have a saved SSID, apply and connect; otherwise we stay AP-only */
    if (g_sta_ssid[0] != '\0') {
        apply_sta_config_and_connect();
        ESP_LOGI(TAG, "AP+STA initialized. AP SSID:%s pass:%s channel:%d | STA SSID:%s",
                 AP_SSID, AP_PASS, AP_CHANNEL, g_sta_ssid);
    } else {
        ESP_LOGI(TAG, "AP-only initialized (no STA creds stored). AP SSID:%s pass:%s channel:%d",
                 AP_SSID, AP_PASS, AP_CHANNEL);
    }
}

/* Apply the currently loaded g_sta_ssid/g_sta_pass to the driver and connect */
static void apply_sta_config_and_connect(void) {
    wifi_config_t sta_cfg = { 0 };
    strncpy((char*)sta_cfg.sta.ssid, g_sta_ssid, sizeof(sta_cfg.sta.ssid) - 1);
    strncpy((char*)sta_cfg.sta.password, g_sta_pass, sizeof(sta_cfg.sta.password) - 1);
    sta_cfg.sta.scan_method = WIFI_FAST_SCAN;
    sta_cfg.sta.pmf_cfg.capable  = true;
    sta_cfg.sta.pmf_cfg.required = false;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

/* Public: save new creds to NVS and apply immediately */
void wifi_set_sta_credentials(const char* ssid, const char* pass) {
    /* Persist */
    writeString(KEY_STA_SSID, ssid ? ssid : "");
    writeString(KEY_STA_PASS, pass ? pass : "");

    /* Update in-RAM copies */
    readString(KEY_STA_SSID, g_sta_ssid, sizeof(g_sta_ssid), "");
    readString(KEY_STA_PASS, g_sta_pass, sizeof(g_sta_pass), "");

    if (g_sta_ssid[0] == '\0') {
        ESP_LOGW(TAG, "Cleared STA credentials; staying AP-only");
        esp_wifi_disconnect();
        return;
    }

    ESP_LOGI(TAG, "Applying new STA credentials: SSID='%s'", g_sta_ssid);
    esp_wifi_disconnect();            // drop any current STA link
    apply_sta_config_and_connect();   // set and reconnect with new creds
}

