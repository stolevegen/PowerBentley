// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_event.h>
#include "esp_netif.h"

#include "storage.h"
#include "captdns.h"
#include "wifi.h"
#include "webserver.h"
#include "spiffs.h"
#include "sntp.h"

#include "websocket.h"
#include "ws_wifi.h"
#include "ws_settings.h"
#include "ws_power_wheel.h"
#include "ws_setup.h"

#include "power_wheel_controller.h"
#include "power_wheel_repository.h"
#include "power_wheel_safety.h"

static const char *TAG = "main";

void app_main()
{
  // To disable all logs, use ESP_LOG_NONE
  esp_log_level_set("*", ESP_LOG_DEBUG);

  ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

  // Init NVS storage
  setup_storage();

  // Init TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_init());
  // Init event mechanism
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Init file storage
  ESP_ERROR_CHECK(setup_spiffs());

  power_wheel_repository_init();
  panic_safety_init();
  power_wheel_controller_init();

  // Setup captive portal - automatically opens the page when we connect to the wifi
  // setup_captive_dns();

  // Setup wifi access point
  setup_wifi();

  // Setup HTTP server
  setup_server();

  // Register websocket callbacks
  register_callback("wifi_status", ws_handle_wifi_status);
  register_callback("wifi_scan", ws_handle_wifi_scan);
  register_callback("wifi_connect", ws_handle_wifi_connect);
  register_callback("wifi_disconnect", ws_handle_wifi_disconnect);

  register_callback("get_settings", ws_handle_get_settings);
  register_callback("time_update", ws_handle_time_update);
  register_callback("get_system_info", ws_handle_system_info);

  // Register power_wheel websocket endpoints
  register_callback("read_all", ws_handle_read_all);
  register_callback("read_throttle", ws_handle_read_throttle);
  register_callback("emergency_stop", ws_handle_emergency_stop);

  // Register settings WebSocket handlers
  register_callback("setup_mode", ws_handle_setup_mode);

  register_callback("get_wiring", ws_handle_get_wiring);
  register_callback("set_wiring", ws_handle_set_wiring);

  register_callback("set_current_profile", ws_handle_set_current_profile);
  register_callback("get_profiles", ws_handle_get_profiles);
  register_callback("save_profile", ws_handle_save_profile);
  register_callback("delete_profile", ws_handle_delete_profile);

  // Retrieve time from network
  // start_ntp_sync();
  // register_time_sync_callback(TBD);
}
