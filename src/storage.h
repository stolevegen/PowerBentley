#ifndef STORAGE_H
#define STORAGE_H

void setup_storage(void);

esp_err_t readFloat(char* key, float *value, float defaultValue);
esp_err_t writeFloat(char* key, float value);
esp_err_t readString(const char* key, char* out, size_t out_len, const char* def);
esp_err_t writeString(const char* key, const char* value);
esp_err_t readUInt64(const char* key, uint64_t* out, uint64_t def);
esp_err_t writeUInt64(const char* key, uint64_t value);


#endif
