// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#pragma once

#define OTA_PASSWORD "your_secure_password_here"

//
// Power wheel constants
//

#define FORWARD_SHUTOFF_THRESOLD 15  // %
#define BACKWARD_SHUTOFF_THRESOLD 10 // %

#define SPEED_INCREMENT 0.5f           // % of increment per loop
#define DRAG_MODE_SPEED_INCREMENT 2.0f // % of increment per loop

#define MOTOR_PWM_CHANNEL_FORWARD LEDC_CHANNEL_1
#define MOTOR_PWM_CHANNEL_BACKWARD LEDC_CHANNEL_2
#define MOTOR_PWM_TIMER LEDC_TIMER_1
#define MOTOR_PWM_DUTY_RESOLUTION LEDC_TIMER_10_BIT

#define STATUS_LED_PIN GPIO_NUM_2

//
// Wifi constants
//

#define AP_WIFI_SSID "PowerJeep"
#define AP_WIFI_PASS "Rubicon!"
#define AP_WIFI_CHANNEL 10
#define MAX_AP_CONN 2
