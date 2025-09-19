#include "power_wheel.h"

#include <sys/param.h>
#include <math.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#include "websocket.h"
#include "cJSON.h"
#include "storage.h"
#include "utils.h"
#include "wifi.h"

// ================
// ==== MACROS ====
// ================

// ADC throttle capability

#define WITH_ADC_THROTTLE 0

#if WITH_ADC_THROTTLE
#include "driver/adc.h"
#include "esp_adc_cal.h"
#endif

// PIN

#define GAS_PEDAL_FORWARD_PIN ADC1_CHANNEL_4 // GPIO 32
#define GAS_PEDAL_BACKWARD_PIN ADC1_CHANNEL_5 // GPIO 33

#define GAS_PEDAL_FORWARD_PIN GPIO_NUM_32
#define GAS_PEDAL_BACKWARD_PIN GPIO_NUM_33

#define FORWARD_PWM_PIN GPIO_NUM_18
#define BACKWARD_PWM_PIN GPIO_NUM_19
#define STATUS_LED_PIN GPIO_NUM_2

// Behavior

#define FORWARD_SHUTOFF_THRESOLD 15 // %
#define BACKWARD_SHUTOFF_THRESOLD 10 // %

#define DEFAULT_FORWARD_MAX_SPEED 60 // %
#define DEFAULT_BACKWARD_MAX_SPEED 35 // %

#define SPEED_INCREMENT 0.5f // % of increment per loop

// PWM

#define MOTOR_PWM_CHANNEL_FORWARD LEDC_CHANNEL_1
#define MOTOR_PWM_CHANNEL_BACKWARD LEDC_CHANNEL_2
#define MOTOR_PWM_TIMER LEDC_TIMER_1
#define MOTOR_PWM_DUTY_RESOLUTION LEDC_TIMER_10_BIT

// ===============
// ==== STATE ====
// ===============

static const char *TAG = "power_wheel";

static float current_speed = 0.0f;
static float max_forward = DEFAULT_FORWARD_MAX_SPEED;
static float max_backward = DEFAULT_BACKWARD_MAX_SPEED;
static bool emergency_stop = false;

static uint32_t led_sleep_delay = 500;

// Prototypes
static void drive_task(void *pvParameter);
static void broadcast_speed_task(void *pvParameter);
static void led_task(void *pvParameter);
static void sta_status_task(void *pvParameter);
static void broadcast_sta_status(void);

#if WITH_ADC_THROTTLE
static bool adc_calibration_enabled = false;
static bool adc_calibration_init(void);
static void adc_read_pedals(uint8_t* forward_position, uint8_t* backward_position);
#else
static void buttons_read_pedals(uint8_t* forward_position, uint8_t* backward_position);
#endif

static void setup_pin();
static void setup_pwm();

static void send_values_to_motor(float speed);
static void blink_led_running(float speed);
static void broadcast_all_values(void);

static int get_speed_target(uint8_t forward_position, uint8_t backward_position);
static float compute_next_speed(float current, float target, float delta);

// =======================
// ==== WEBSOCKETS RX ====
// =======================

static void data_received(httpd_ws_frame_t* ws_pkt) {
  ESP_LOGI(TAG, "Received packet with message: %s", ws_pkt->payload);

  cJSON *root = cJSON_Parse((char*)ws_pkt->payload);
  char* command = cJSON_GetObjectItem(root, "command")->valuestring;
  ESP_LOGI(TAG, "Command: %s", command);

  // Handle STA Wi-Fi credentials set from UI
  if (strcmp("set_sta", command) == 0) {
    cJSON* parameters = cJSON_GetObjectItem(root, "parameters");
    if (parameters) {
      cJSON* ssid = cJSON_GetObjectItem(parameters, "ssid");
      cJSON* pass = cJSON_GetObjectItem(parameters, "password");
      if (cJSON_IsString(ssid) && cJSON_IsString(pass)) {
        wifi_set_sta_credentials(ssid->valuestring, pass->valuestring);
        char *ack;
        asprintf(&ack, "{\"ok\":true,\"type\":\"set_sta\",\"ssid\":\"%s\"}", ssid->valuestring);
        broadcast_message(ack);
        free(ack);
      } else {
        char *ack;
        asprintf(&ack, "{\"ok\":false,\"type\":\"set_sta\",\"error\":\"invalid parameters\"}");
        broadcast_message(ack);
        free(ack);
      }
    }
    goto end;
  }

  // Provide current saved STA SSID to prefill UI
  if (strcmp("get_sta", command) == 0) {
    char ssid_buf[33] = {0};
    readString("sta_ssid", ssid_buf, sizeof(ssid_buf), "");
    char *msg;
    asprintf(&msg, "{\"type\":\"sta_info\",\"ssid\":\"%s\"}", ssid_buf);
    broadcast_message(msg);
    free(msg);
    goto end;
  }

  // Clear STA credentials (AP-only mode)
  if (strcmp("clear_sta", command) == 0) {
    wifi_set_sta_credentials("", "");
    char *ack;
    asprintf(&ack, "{\"ok\":true,\"type\":\"clear_sta\"}");
    broadcast_message(ack);
    free(ack);
    goto end;
  }

  if (strcmp("update_max", command) == 0) {
    cJSON* parameters = cJSON_GetObjectItem(root, "parameters");
    if (parameters == NULL) {
      goto end;
    }

    cJSON *max_forward_node = cJSON_GetObjectItem(parameters, "max_forward");
    cJSON *max_backward_node = cJSON_GetObjectItem(parameters, "max_backward");
    if (!cJSON_IsNumber(max_forward_node) || !cJSON_IsNumber(max_backward_node)) {
      goto end;
    }

    max_forward = max_forward_node->valuedouble;
    max_backward = max_backward_node->valuedouble;

    writeFloat("max_forward", max_forward);
    writeFloat("max_backward", max_backward);

    broadcast_all_values();
    goto end;
  }

  if (strcmp("emergency_stop", command) == 0) {
    cJSON* parameters = cJSON_GetObjectItem(root, "parameters");
    if (parameters == NULL) {
      goto end;
    }

    cJSON *active_node = cJSON_GetObjectItem(parameters, "active");
    if (!cJSON_IsBool(active_node)) {
      goto end;
    }

    emergency_stop = cJSON_IsTrue(active_node);
    if (emergency_stop) {
      current_speed = 0;
      send_values_to_motor(current_speed);
    }

    broadcast_all_values();
    goto end;
  }

end:
  cJSON_Delete(root);
}

// **********
// **** SETUP
// **********

#if WITH_ADC_THROTTLE
static bool adc_calibration_init(void) {
  esp_err_t ret;
  esp_adc_cal_characteristics_t adc_chars;
  ret = esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP);
  if (ret == ESP_ERR_NOT_SUPPORTED || ret == ESP_ERR_INVALID_VERSION) {
    ESP_LOGW(TAG, "ADC calibration not supported, fallback to raw values");
    return false;
  }
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 1100, &adc_chars);
  return true;
}
#endif

// Setup pin on the board
void setup_pin() {
  #if WITH_ADC_THROTTLE
  adc_calibration_enabled = adc_calibration_init();
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
  ESP_ERROR_CHECK(adc1_config_channel_atten(GAS_PEDAL_FORWARD_PIN, ADC_ATTEN_DB_11));
  ESP_ERROR_CHECK(adc1_config_channel_atten(GAS_PEDAL_BACKWARD_PIN, ADC_ATTEN_DB_11));
  #else
  gpio_reset_pin(GAS_PEDAL_FORWARD_PIN);
  gpio_set_direction(GAS_PEDAL_FORWARD_PIN, GPIO_MODE_INPUT);
  gpio_pullup_en(GAS_PEDAL_FORWARD_PIN);

  gpio_reset_pin(GAS_PEDAL_BACKWARD_PIN);
  gpio_set_direction(GAS_PEDAL_BACKWARD_PIN, GPIO_MODE_INPUT);
  gpio_pullup_en(GAS_PEDAL_BACKWARD_PIN);
  #endif

  gpio_reset_pin(FORWARD_PWM_PIN);
  gpio_set_direction(FORWARD_PWM_PIN, GPIO_MODE_OUTPUT);

  gpio_reset_pin(BACKWARD_PWM_PIN);
  gpio_set_direction(BACKWARD_PWM_PIN, GPIO_MODE_OUTPUT);

  gpio_reset_pin(STATUS_LED_PIN);
  gpio_set_direction(STATUS_LED_PIN, GPIO_MODE_OUTPUT);
}

// Setup PWM timer and channels
void setup_pwm() {
  ledc_channel_config_t ledc_channel_forward = {0};
  ledc_channel_forward.gpio_num = FORWARD_PWM_PIN;
  ledc_channel_forward.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_channel_forward.channel = MOTOR_PWM_CHANNEL_FORWARD;
  ledc_channel_forward.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_forward.timer_sel = MOTOR_PWM_TIMER;
  ledc_channel_forward.duty = 0;

  ledc_channel_config_t ledc_channel_backward = {0};
  ledc_channel_backward.gpio_num = BACKWARD_PWM_PIN;
  ledc_channel_backward.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_channel_backward.channel = MOTOR_PWM_CHANNEL_BACKWARD;
  ledc_channel_backward.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_backward.timer_sel = MOTOR_PWM_TIMER;
  ledc_channel_backward.duty = 0;

  ledc_timer_config_t ledc_timer = {0};
  ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_timer.duty_resolution = MOTOR_PWM_DUTY_RESOLUTION;
  ledc_timer.timer_num = MOTOR_PWM_TIMER;
  ledc_timer.freq_hz = 25000;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_forward));
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_backward));
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
}

// *****************
// **** DRIVING LOOP
// *****************

void setup_driving(void) {
  // Retrieve max values from storage
  readFloat("max_forward", &max_forward, DEFAULT_FORWARD_MAX_SPEED);
  readFloat("max_backward", &max_backward, DEFAULT_BACKWARD_MAX_SPEED);

  // Setup pins
  setup_pin();

  // Setup PWM
  setup_pwm();

  // Listen to Websocket events
  register_callback(data_received);

  // Create a task with the higher priority for the driving task
  xTaskCreate(&drive_task, "drive_task", 2048, NULL, 20, NULL);

  // Create a task for broadcasting the values regularly to the UI
  xTaskCreate(&broadcast_speed_task, "broadcast_speed_task", 2048, NULL, 10, NULL);

  // Create a task for the blinking LED
  xTaskCreate(&led_task, "led_task", 2048, NULL, 5, NULL);

  // Create a task for Wi-Fi STA link status broadcast
  xTaskCreate(&sta_status_task, "sta_status_task", 2048, NULL, 5, NULL);
}

// Return the targeted speed based on the pedal status.
// It is a percentage between -100 and 100 (backward and forward)
int get_speed_target(uint8_t forward_position, uint8_t backward_position) {
  if ((!forward_position && !backward_position) ||
      (forward_position && backward_position)) {
    return 0;
  }
  
  if (forward_position) {
    return min(max_forward, max_forward * (forward_position / 100.0f));
  } 

  // Backward is negative values
  return max(-max_backward, -max_backward * (backward_position / 100.0f));
}

// Calculate next step for a smooth transition from current speed to targeted speed
float compute_next_speed(float current, float target, float delta) {    
  if (current < target) {
    // Slow down backward or speed up forward

    if (current < 0 && current > -BACKWARD_SHUTOFF_THRESOLD) {
      // Between -BACKWARD_SHUTOFF_THRESOLD < current < 0, we stop the car
      return 0;
    } else if (current > 0 && current < FORWARD_SHUTOFF_THRESOLD) {
      // Between 0 < current < FORWARD_SHUTOFF_THRESOLD, we set the car to FORWARD_SHUTOFF_THRESOLD
      return FORWARD_SHUTOFF_THRESOLD;
    } else if (current < 0) {
      // Slow down more aggressively if the car is moving quicker than 50%
      float slowdown_rate = current > 50 ? 0.08 : 0.04;
      return current + slowdown_rate * delta;
    } else {
      return current + SPEED_INCREMENT * delta;
    }
  } else if (current > target) {
    // Slow down forward or speed up backward

    if (current > 0 && current < FORWARD_SHUTOFF_THRESOLD) {
      // Between 0 < current < FORWARD_SHUTOFF_THRESOLD, we stop the car
      return 0;
    } else if (current < 0 && current > -BACKWARD_SHUTOFF_THRESOLD) {
      // Between -BACKWARD_SHUTOFF_THRESOLD < current < 0, we set the car to -BACKWARD_SHUTOFF_THRESOLD
      return -BACKWARD_SHUTOFF_THRESOLD;
    } else if (current > 0) {
      // Slow down more aggressively if the car is moving quicker than 50%
      float slowdown_rate = current > 50 ? 0.08 : 0.04;
      return current - slowdown_rate * delta;
    } else {
      return current - SPEED_INCREMENT * delta;
    }
  }

  return target;
}

static void drive_task(void *pvParameter) {
  int64_t last_update = esp_timer_get_time();
  float target = 0.0f;
  float delta = 0.0f;

  while (true) {
    if (emergency_stop) {
      current_speed = 0;
      send_values_to_motor(current_speed);
      blink_led_running(current_speed);
      vTaskDelay(20 / portTICK_PERIOD_MS);
      continue;
    }

    uint8_t forward_position = 0;
    uint8_t backward_position = 0;

    #if WITH_ADC_THROTTLE
    adc_read_pedals(&forward_position, &backward_position);
    #else
    buttons_read_pedals(&forward_position, &backward_position);
    #endif

    // Update targeted speed accordingly
    target = get_speed_target(forward_position, backward_position);

    // Take into account a loop could take more than expected
    // This is used to slow down within a fixed timeframe, regardless of the loop duration
    delta = (esp_timer_get_time() - last_update) / 1000;

    // Compute next speed based on current speed and targeted speed
    current_speed = compute_next_speed(current_speed, target, delta);

    // Send value to the motor
    send_values_to_motor(current_speed);

    last_update = esp_timer_get_time();

    // Blink embedded led to have some visible status of the speed
    blink_led_running(current_speed);

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Blink the board led to indicate what the car is doing, or at least should be doing
static void led_task(void *pvParameter) {
  while (true) {
    gpio_set_level(STATUS_LED_PIN, 0);
    vTaskDelay(led_sleep_delay / portTICK_PERIOD_MS);
    gpio_set_level(STATUS_LED_PIN, 1);
    vTaskDelay(led_sleep_delay / portTICK_PERIOD_MS);
  }
}

#if WITH_ADC_THROTTLE
static void adc_read_pedals(uint8_t* forward_position, uint8_t* backward_position) {
  int forward_value = 0;
  int backward_value = 0;

  for (int i=0; i<16; i++) {
    forward_value += adc1_get_raw(GAS_PEDAL_FORWARD_PIN);
    backward_value += adc1_get_raw(GAS_PEDAL_BACKWARD_PIN);
  }
  forward_value /= 16;
  backward_value /= 16;

  if (adc_calibration_enabled) {
    // Add calibrated conversion if needed
  }

  *forward_position = (uint8_t)lroundf((forward_value / 4095.0f) * 100.0f);
  *backward_position = (uint8_t)lroundf((backward_value / 4095.0f) * 100.0f);
}
#else
static void buttons_read_pedals(uint8_t* forward_position, uint8_t* backward_position) {
  // With pull-ups enabled, the button pressed pulls to GND (active low)
  int forward_pressed = gpio_get_level(GAS_PEDAL_FORWARD_PIN) == 0;
  int backward_pressed = gpio_get_level(GAS_PEDAL_BACKWARD_PIN) == 0;

  *forward_position = forward_pressed ? 100 : 0;
  *backward_position = backward_pressed ? 100 : 0;
}
#endif

static void send_values_to_motor(float speed) {
  float forward_duty_fraction = 0.0f;
  float backward_duty_fraction = 0.0f;

  if (speed > 0) {
    forward_duty_fraction = speed / 100.0f;
  } else if (speed < 0) {
    backward_duty_fraction = speed / 100.0f;
  }

  uint32_t max_duty = (1 << MOTOR_PWM_DUTY_RESOLUTION) - 1;
  uint32_t forward_duty = lroundf(forward_duty_fraction * (float)max_duty);
  uint32_t backward_duty = -1 * lroundf(backward_duty_fraction * (float)max_duty);

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_FORWARD, forward_duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_FORWARD));

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_BACKWARD, backward_duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_BACKWARD));
}

static void blink_led_running(float speed) {
  if (speed == 0) {
    led_sleep_delay = 500;
  } else {
    float abs_speed = fabsf(speed);
    if (abs_speed < 20) {
      led_sleep_delay = 250;
    } else if (abs_speed < 50) {
      led_sleep_delay = 125;
    } else {
      led_sleep_delay = 60;
    }
  }
}

// Periodically broadcast STA link status for UI indicator
static void broadcast_sta_status(void) {
  bool connected = false;
  char ipstr[16] = "0.0.0.0";

  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    connected = true;
  }

  esp_netif_t* sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (sta) {
    esp_netif_ip_info_t ip;
    if (esp_netif_get_ip_info(sta, &ip) == ESP_OK) {
      snprintf(ipstr, sizeof(ipstr), "%d.%d.%d.%d", IP2STR(&ip.ip));
    }
  }

  char *msg;
  asprintf(&msg, "{\"type\":\"sta_status\",\"connected\":%s,\"ip\":\"%s\"}",
           connected ? "true" : "false", ipstr);
  broadcast_message(msg);
  free(msg);
}

static void sta_status_task(void *pvParameter) {
  while (true) {
    broadcast_sta_status();
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

// ***************
// **** WEBSOCKETS
// ***************

// Broadcast all values
// {
//   "current_speed": 12,
//   "max_forward": 66,
//   "max_backward": 50,
//   "emergency_stop": false
//}
broadcast_all_values() {
  char *message;
  char *format = "{\"current_speed\":%f,\"max_forward\":%f,\"max_backward\":%f,\"emergency_stop\":%s}";
  asprintf(&message, format, current_speed, max_forward, max_backward, emergency_stop ? "true" : "false");
  ESP_LOGI(TAG, "Send %s", message);
  broadcast_message(message);
  free(message);
}

