// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#include "power_wheel_controller.h"

#include <sys/param.h>
#include <math.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "cJSON.h"
#include "storage.h"
#include "utils.h"
#include "constants.h"

#include "ws_power_wheel.h"
#include "power_wheel_repository.h"

#include "adc_utils.h"

static void drive_task(void *pvParameter);
static void led_task(void *pvParameter);

static const char *TAG = "power_wheel_controller";

int led_sleep_delay = 20;
static bool pins_initialized = false;
static bool pwm_initialized = false;

// **********
// **** SETUP
// **********

static void setup_digital_input(int pin)
{
  gpio_reset_pin(pin);
  gpio_set_direction(pin, GPIO_MODE_INPUT);
  gpio_pullup_en(pin);
}

// Setup pins based on current wiring configuration
static void setup_pins_from_wiring()
{
  wiring_t *wiring;
  get_wiring(&wiring);

  if (!wiring)
  {
    ESP_LOGE(TAG, "Failed to get wiring configuration");
    return;
  }

  if (wiring->is_speed_direction)
  {
    // In speed/direction mode, throttle_pin controls speed
    if (wiring->is_adc_throttle)
    {
      adc_init_single_pin(wiring->throttle_pin);
    }
    else
    {
      setup_digital_input(wiring->throttle_pin);
    }

    setup_digital_input(wiring->forward_pin);
    setup_digital_input(wiring->backward_pin);
  }
  else
  {
    // Otherwise forward/backward pins control speed directly
    if (wiring->is_adc_throttle)
    {
      adc_init_two_pins(wiring->forward_pin, wiring->backward_pin);
    }
    else
    {
      setup_digital_input(wiring->forward_pin);
      setup_digital_input(wiring->backward_pin);
    }
  }

  // Setup motor pins
  gpio_reset_pin(wiring->forward_motor_pin);
  gpio_set_direction(wiring->forward_motor_pin, GPIO_MODE_OUTPUT);

  gpio_reset_pin(wiring->backward_motor_pin);
  gpio_set_direction(wiring->backward_motor_pin, GPIO_MODE_OUTPUT);

  // Setup status LED pin (assuming it's still defined in constants.h)
  gpio_reset_pin(STATUS_LED_PIN);
  gpio_set_direction(STATUS_LED_PIN, GPIO_MODE_OUTPUT);

  pins_initialized = true;

  if (wiring->is_speed_direction)
  {
    ESP_LOGI(TAG, "Pins configured for speed/direction mode: throttle=%d, forward=%d, backward=%d, forward_motor=%d, backward_motor=%d",
             wiring->throttle_pin, wiring->forward_pin, wiring->backward_pin, wiring->forward_motor_pin, wiring->backward_motor_pin);
  }
  else
  {
    ESP_LOGI(TAG, "Pins configured for direct mode: forward=%d, backward=%d, forward_motor=%d, backward_motor=%d",
             wiring->forward_pin, wiring->backward_pin, wiring->forward_motor_pin, wiring->backward_motor_pin);
  }
}

// Setup PWM channels based on current wiring configuration
static void setup_pwm_from_wiring()
{
  wiring_t *wiring;
  get_wiring(&wiring);

  if (!wiring)
  {
    ESP_LOGE(TAG, "Failed to get wiring configuration for PWM setup");
    return;
  }

  // Clean up existing PWM configuration if it exists
  if (pwm_initialized)
  {
    ledc_stop(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_FORWARD, 0);
    ledc_stop(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_BACKWARD, 0);
  }

  ledc_channel_config_t ledc_channel_forward = {0}, ledc_channel_backward = {0};

  ledc_channel_forward.gpio_num = wiring->forward_motor_pin;
  ledc_channel_forward.speed_mode = LEDC_HIGH_SPEED_MODE;
  ledc_channel_forward.channel = MOTOR_PWM_CHANNEL_FORWARD;
  ledc_channel_forward.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_forward.timer_sel = MOTOR_PWM_TIMER;
  ledc_channel_forward.duty = 0;

  ledc_channel_backward.gpio_num = wiring->backward_motor_pin;
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

  pwm_initialized = true;
  ESP_LOGI(TAG, "PWM configured for motor pins: forward=%d, backward=%d",
           wiring->forward_motor_pin, wiring->backward_motor_pin);
}

// Function to reconfigure pins and PWM when wiring changes
void power_wheel_controller_reconfigure_wiring(void)
{
  ESP_LOGI(TAG, "Reconfiguring wiring...");

  // Stop current PWM output
  if (pwm_initialized)
  {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_FORWARD, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_FORWARD);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_BACKWARD, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_BACKWARD);
  }

  // Reset current speed to stop the motor
  set_current_speed(0);

  // Reconfigure pins and PWM
  setup_pins_from_wiring();
  setup_pwm_from_wiring();

  ESP_LOGI(TAG, "Wiring reconfiguration complete");
}

void power_wheel_controller_init(void)
{
  power_wheel_controller_reconfigure_wiring();

  // Create a task with the higher priority for the driving task
  xTaskCreate(&drive_task, "drive_task", 2048, NULL, 20, NULL);

  // Create a task for the led
  xTaskCreate(&led_task, "led_task", 2048, NULL, 10, NULL);
}

// **********
// **** LOGIC
// **********

// Speed is a percentage between -100 and 100 (backward and forward)
static void send_values_to_motor(int speed)
{
  float forward_duty_fraction = 0;
  float backward_duty_fraction = 0;

  if (speed > 100 || speed < -100)
  {
    return;
  }

  if (speed > 0)
  {
    forward_duty_fraction = speed / 100.0f;
  }
  else if (speed < 0)
  {
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

// Return the targeted speed based on the pedal status.
// It is a percentage between -100 and 100 (backward and forward)
static int get_speed_target_dual_input(uint8_t forward_throttle, uint8_t backward_throttle, float max_forward, float max_backward)
{
  // Safety
  if ((!forward_throttle && !backward_throttle) ||
      (forward_throttle && backward_throttle))
  {
    return 0;
  }

  if (forward_throttle)
  {
    return min(max_forward, max_forward * (forward_throttle / 100.0f));
  }

  // Backward is negative values
  return max(-max_backward, -max_backward * (backward_throttle / 100.0f));
}

// Get speed target for speed/direction mode (throttle controls speed, direction pins control direction)
static int get_speed_target_speed_direction(uint8_t forward_direction, uint8_t backward_direction, uint8_t throttle_position, float max_forward, float max_backward)
{
  // Safety
  if (throttle_position == 0 ||
      (!forward_direction && !backward_direction) ||
      (forward_direction && backward_direction))
  {
    return 0;
  }

  if (forward_direction)
  {
    return min(max_forward, max_forward * (throttle_position / 100.0f));
  }
  else // backward_direction
  {
    // Backward is negative values
    return max(-max_backward, -max_backward * (throttle_position / 100.0f));
  }
}

static uint8_t get_throttle_position(uint8_t gpio)
{
  wiring_t *wiring;
  get_wiring(&wiring);

  if (!wiring)
  {
    ESP_LOGE(TAG, "Failed to get wiring configuration for throttle");
    return 0;
  }

  if (wiring->is_adc_throttle)
  {
    float currentThrottle = get_adc_value(gpio) / 1000.0f;
    return min(100,
               max(0,
                   (int8_t)((currentThrottle - wiring->min_threshold) / (wiring->max_threshold - wiring->min_threshold) * 100)));
  }
  else
  {
    return !gpio_get_level(gpio) ? 100 : 0;
  }
}

// Blink the led to indicate an emergency stop
static void blink_led_emergency_stop()
{
  led_sleep_delay = 200;
}

// Blink the led to indicate the car is running
static void blink_led_running(int speed)
{
  led_sleep_delay = speed == 0 ? 1000 : (1.0f - fabsf(speed / 100.0f)) * 160 + 20 /*ms mini*/;
}

// Calculate next step for a smooth transition from current speed to targeted speed
static float compute_next_speed(float current, float target, float delta, bool is_drag_mode)
{
  if (current < target)
  {
    // Slow down backward or speed up forward

    if (current < 0 && current > -BACKWARD_SHUTOFF_THRESOLD)
    {
      // Between -BACKWARD_SHUTOFF_THRESOLD < current < 0, we stop the car
      return 0;
    }
    else if (current > 0 && current < FORWARD_SHUTOFF_THRESOLD)
    {
      // Between 0 < current < FORWARD_SHUTOFF_THRESOLD, we set the car to FORWARD_SHUTOFF_THRESOLD
      return FORWARD_SHUTOFF_THRESOLD;
    }
    else if (current < 0)
    {
      // Slow down more aggressively if the car is moving quicker than 50%
      float slowdown_rate = current > 50 ? 0.08 : 0.04;
      // Safety! Slowing down backward, we must stop the car within a time frame
      return current + delta * slowdown_rate;
    }
    else
    {
      // Else we update speed incrementaly
      float speed_increment = is_drag_mode ? DRAG_MODE_SPEED_INCREMENT : SPEED_INCREMENT;
      return current + speed_increment;
    }

    // Check if we went too far
    if (current > target)
      return target;
  }
  else if (current > target)
  {
    // Slow down forward or speed up backward

    if (current > 0 && current < FORWARD_SHUTOFF_THRESOLD)
    {
      // Between 0 < current < FORWARD_SHUTOFF_THRESOLD, we stop the car
      return 0;
    }
    else if (current < 0 && current > -BACKWARD_SHUTOFF_THRESOLD)
    {
      // Between -BACKWARD_SHUTOFF_THRESOLD < current < 0, we set the car to -BACKWARD_SHUTOFF_THRESOLD
      return -BACKWARD_SHUTOFF_THRESOLD;
    }
    else if (current > 0)
    {
      // Slow down more aggressively if the car is moving quicker than 50%
      float slowdown_rate = current > 50 ? 0.08 : 0.04;
      // Safety! Slowing down forward, we must stop the car within a time frame
      return current - delta * slowdown_rate;
    }
    else
    {
      // Else we update speed incrementaly
      return current - SPEED_INCREMENT;
    }

    // Check if we went too far
    if (current < target)
      return target;
  }

  return current;
}

// **********
// **** TASKS
// **********

// Task that drives the car
static void drive_task(void *pvParameter)
{
  int64_t last_update = esp_timer_get_time();
  float delta;

  int forward_position = 0;
  int backward_position = 0;
  int throttle_position = 0;

  int target = 0;
  float current_speed;

  while (true)
  {
    // Check if required configuration is available
    wiring_t *wiring;
    get_wiring(&wiring);

    stored_profile_t *profile;
    get_current_profile(&profile);

    if (!wiring || !profile || !pins_initialized || !pwm_initialized)
    {
      ESP_LOGW(TAG, "Wiring not configured or pins/PWM not initialized, waiting...");
      vTaskDelay(100 / portTICK_PERIOD_MS);
      continue;
    }

    // Manage emergency stop & setup mode
    if (get_emergency_stop() || get_setup_mode())
    {
      set_current_speed(0);
      send_values_to_motor(0);

      last_update = esp_timer_get_time();

      blink_led_emergency_stop();

      vTaskDelay(50 / portTICK_PERIOD_MS);
      continue;
    }

    current_speed = get_current_speed();

    // Update targeted speed accordingly
    if (wiring->is_speed_direction)
    {
      throttle_position = get_throttle_position(wiring->throttle_pin);
      bool forward_pressed = !gpio_get_level(wiring->forward_pin);
      bool backward_pressed = !gpio_get_level(wiring->backward_pin);
      forward_position = forward_pressed ? 100 : 0;
      backward_position = backward_pressed ? 100 : 0;

      target = get_speed_target_speed_direction(forward_position, backward_position, throttle_position, profile->profile.max_forward, profile->profile.max_backward);
    }
    else
    {
      // Update pedal & direction status using dynamic wiring
      forward_position = get_throttle_position(wiring->forward_pin);
      backward_position = get_throttle_position(wiring->backward_pin);

      target = get_speed_target_dual_input(forward_position, backward_position, profile->profile.max_forward, profile->profile.max_backward);
    }

    // Take into account a loop could take more than expected
    // This is used to slow down within a fixed timeframe, regardless of the loop duration
    delta = (esp_timer_get_time() - last_update) / 1000;

    // Compute next speed based on current speed and targeted speed
    current_speed = compute_next_speed(current_speed, target, delta, profile->profile.is_drag_mode);
    set_current_speed(current_speed);

    // Send value to the motor
    send_values_to_motor(current_speed);

    last_update = esp_timer_get_time();

    // Blink embedded led to have some visible status of the speed
    blink_led_running(current_speed);

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

// Blink the board led to indicate what the car is doing, or at least should be doing
static void led_task(void *pvParameter)
{
  while (true)
  {
    gpio_set_level(STATUS_LED_PIN, 0);
    vTaskDelay(led_sleep_delay / portTICK_PERIOD_MS);
    gpio_set_level(STATUS_LED_PIN, 1);
    vTaskDelay(led_sleep_delay / portTICK_PERIOD_MS);
  }
}