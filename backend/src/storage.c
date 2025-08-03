// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#include "storage.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *NVS_NAMESPACE = "storage";

// Long term storage that survives restart

void setup_storage(void)
{
  // Init NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

esp_err_t read_bool(const char *key, bool *value, bool default_value)
{
  *value = default_value;
  size_t required_size = sizeof(bool);
  return read_blob(key, (void *)value, &required_size);
}

esp_err_t write_bool(const char *key, bool value)
{
  return write_blob(key, (void *)&value, sizeof(bool));
}

esp_err_t read_int(const char *key, int *value, int default_value)
{
  *value = default_value;
  size_t required_size = sizeof(int);
  return read_blob(key, (void *)value, &required_size);
}

esp_err_t write_int(const char *key, int value)
{
  return write_blob(key, (void *)&value, sizeof(int));
}

esp_err_t read_float(const char *key, float *value, float default_value)
{
  *value = default_value;
  size_t required_size = sizeof(float);
  return read_blob(key, (void *)value, &required_size);
}

esp_err_t write_float(const char *key, float value)
{
  return write_blob(key, (void *)&value, sizeof(float));
}

esp_err_t read_blob(const char *key, void *out_value, size_t *required_size)
{
  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (ret != ESP_OK)
  {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    return ret;
  }
  ret = nvs_get_blob(nvs_handle, key, out_value, required_size);
  nvs_close(nvs_handle);
  return ret;
}

esp_err_t write_blob(const char *key, const void *value, size_t required_size)
{
  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (ret != ESP_OK)
  {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    return ret;
  }
  ret = nvs_set_blob(nvs_handle, key, value, required_size);
  if (ret == ESP_OK)
  {
    ret = nvs_commit(nvs_handle);
  }

  nvs_close(nvs_handle);
  return ret;
}

esp_err_t delete_blob(const char *key)
{
  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (ret != ESP_OK)
  {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
    return ret;
  }
  ret = nvs_erase_key(nvs_handle, key);
  if (ret == ESP_OK)
  {
    ret = nvs_commit(nvs_handle);
  }

  nvs_close(nvs_handle);
  return ret;
}