// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#include "adc_utils.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "utils.h"

static const char *TAG = "adc_utils";

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle;
static bool adc_calibration_enabled = false;
static uint32_t adc_average = 0;
static int adc_raw = 0;
static uint32_t adc_voltage = 0;

bool adc_calibration_init(void)
{
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "Calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated)
    {
        ESP_LOGI(TAG, "Calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }
#endif

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "ADC calibration initialized successfully");
    }
    else
    {
        ESP_LOGW(TAG, "ADC calibration initialization failed, using raw values");
    }

    return calibrated;
}

// Function to convert GPIO pin number to ADC channel
// Returns the ADC1 channel number, or -1 if the pin is not a valid ADC pin
int gpio_to_adc1_channel(uint8_t gpio_pin)
{
    switch (gpio_pin)
    {
    // ADC1 channels (GPIO 32-39)
    case 36:
        return ADC_CHANNEL_0; // GPIO36
    case 37:
        return ADC_CHANNEL_1; // GPIO37
    case 38:
        return ADC_CHANNEL_2; // GPIO38
    case 39:
        return ADC_CHANNEL_3; // GPIO39
    case 32:
        return ADC_CHANNEL_4; // GPIO32
    case 33:
        return ADC_CHANNEL_5; // GPIO33
    case 34:
        return ADC_CHANNEL_6; // GPIO34
    case 35:
        return ADC_CHANNEL_7; // GPIO35

    default:
        ESP_LOGE(TAG, "GPIO%d is not a valid ADC1 pin", gpio_pin);
        return -1;
    }
}

static void adc_deinit(void)
{
    if (adc1_handle != NULL)
    {
        adc_oneshot_del_unit(adc1_handle);
        adc1_handle = NULL;
        adc_calibration_enabled = false;
        ESP_LOGI(TAG, "ADC unit deinitialized");
    }
}

static esp_err_t adc_init_pins_internal(uint8_t *pins, uint8_t pin_count)
{
    adc_deinit();

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc1_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "ADC unit initialized");

    // Configure each ADC channel
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    for (uint8_t i = 0; i < pin_count; i++)
    {
        int channel = gpio_to_adc1_channel(pins[i]);

        // Check if pin is valid ADC pin
        if (channel == -1)
        {
            ESP_LOGE(TAG, "Invalid ADC pin provided: %d", pins[i]);
            continue; // Skip invalid pins but continue with others
        }

        esp_err_t ret = adc_oneshot_config_channel(adc1_handle, channel, &config);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to configure channel %d (pin %d): %s", channel, pins[i], esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGI(TAG, "ADC channel %d (pin %d) configured successfully", channel, pins[i]);
        }
    }

    adc_calibration_enabled = adc_calibration_init();
    if (adc_calibration_enabled)
    {
        ESP_LOGI("ADC", "ADC calibration initialized");
    }
    else
    {
        ESP_LOGW("ADC", "ADC calibration not available");
    }

    return ESP_OK;
}

void adc_init_single_pin(uint8_t pin)
{
    adc_init_pins_internal(&pin, 1);
}

void adc_init_two_pins(uint8_t forward_pin, uint8_t backward_pin)
{
    uint8_t pins[] = {forward_pin, backward_pin};
    adc_init_pins_internal(pins, 2);
}

uint32_t get_adc_value(uint8_t gpio)
{
    if (adc1_handle == NULL)
    {
        ESP_LOGE(TAG, "ADC not initialized");
        return -1;
    }

    int channel = gpio_to_adc1_channel(gpio);
    adc_average = 0;

    for (int i = 0; i < 5; ++i)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &adc_raw));

        if (adc_calibration_enabled)
        {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, (int *)&adc_voltage));
            adc_average += adc_voltage;
        }
        else
        {
            // If no calibration, use raw values scaled appropriately
            // This is a rough approximation - you may need to adjust based on your specific setup
            adc_voltage = (adc_raw * 3300) / 4095; // Assuming 3.3V reference and 12-bit ADC
            adc_average += adc_voltage;
        }
    }

    return adc_average / 5;
}
