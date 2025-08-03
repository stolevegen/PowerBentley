#include "power_wheel_safety.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"

#include "constants.h"
#include "power_wheel_repository.h"

static const char *TAG = "power_wheel_safety";

static void panic_emergency_shutdown_hook(void)
{
    // Disable interrupts to ensure atomic operation during emergency shutdown
    portDISABLE_INTERRUPTS();

    // Stop PWM channels immediately using ESP-IDF APIs
    ledc_stop(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_FORWARD, 0);
    ledc_stop(LEDC_HIGH_SPEED_MODE, MOTOR_PWM_CHANNEL_BACKWARD, 0);

    // Get wiring configuration
    wiring_t *wiring = NULL;
    get_wiring(&wiring);

    if (wiring)
    {
        // Force GPIO pins to safe state (LOW) using standard ESP-IDF functions
        gpio_set_level(wiring->forward_motor_pin, 0);
        gpio_set_level(wiring->backward_motor_pin, 0);
    }

    // Add a small delay to ensure hardware has time to respond
    // Note: In panic context, standard delays might not work, so use a simple loop
    volatile int delay_count = 1000;
    while (delay_count--)
    {
        __asm__ __volatile__("nop");
    }

    // Re-enable interrupts
    portENABLE_INTERRUPTS();
}

void panic_safety_init(void)
{
    // Verify critical menuconfig settings
#ifndef CONFIG_ESP_PANIC_HANDLER_IRAM
    ESP_LOGW(TAG, "WARNING: Panic handler not in IRAM - may not work during flash operations");
    ESP_LOGW(TAG, "Enable: Component config → ESP System Settings → Place panic handler code in IRAM");
#endif

#if CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS < 2
    ESP_LOGW(TAG, "WARNING: Panic reboot delay is very short (%d sec) - increase for motor safety",
             CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS);
#endif

    // Register the panic hook
    esp_err_t ret = esp_register_shutdown_handler(panic_emergency_shutdown_hook);

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Panic emergency shutdown hook registered successfully");
#ifdef CONFIG_ESP_PANIC_HANDLER_IRAM
        ESP_LOGI(TAG, "Panic handler is in IRAM - optimal for motor safety");
#endif
    }
    else
    {
        ESP_LOGE(TAG, "Failed to register panic shutdown hook: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Panic reboot delay: %d seconds", CONFIG_ESP_SYSTEM_PANIC_REBOOT_DELAY_SECONDS);
}