#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

#include "storage.h"

nvs_handle_t storage;

// Long term storage that survives restart

void setup_storage(void) {
  // Init NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  
  ret = nvs_open("storage", NVS_READWRITE, &storage);
  if (ret != ESP_OK) {
    printf("Error (%s) opening NVS handle!\n", esp_err_to_name(ret));
  }
}

esp_err_t readFloat(char* key, float *value, float defaultValue) {
  *value = defaultValue;
  size_t required_size = 4;
  return nvs_get_blob(storage, key, value, &required_size);
}

esp_err_t writeFloat(char* key, float value) {
  ESP_LOGI("Storage", "Store value %f for key %s", value, key);
  esp_err_t ret = nvs_set_blob(storage, key, &value, sizeof(float));
  if (ret != ESP_OK) return ret;

  esp_err_t readString(const char* key, char* out, size_t out_len, const char* def) {
  if (!out || out_len == 0) return ESP_ERR_INVALID_ARG;

  // Default
  if (def) {
    strncpy(out, def, out_len - 1);
    out[out_len - 1] = '\0';
  } else {
    out[0] = '\0';
  }

  // Query size
  size_t required = 0;
  esp_err_t ret = nvs_get_str(storage, key, NULL, &required);
  if (ret != ESP_OK) return ret;

  if (required == 0 || required > out_len) return ESP_ERR_NVS_INVALID_LENGTH;

  return nvs_get_str(storage, key, out, &required);
}

esp_err_t writeString(const char* key, const char* value) {
  if (!value) value = "";
  ESP_LOGI("Storage", "Store string for key %s: '%s'", key, value);
  esp_err_t ret = nvs_set_str(storage, key, value);
  if (ret != ESP_OK) return ret;
  return nvs_commit(storage);
}

  return nvs_commit(storage);
}
