#include "utils.h"

#include "esp_log.h"

static const char *TAG = "utils";

size_t json_escape_string(const char *input, char *output, size_t output_size)
{
    if (!input || !output || output_size < 3)
        return 0; // Need at least room for quotes and null terminator

    size_t out_pos = 0;
    output[out_pos++] = '"';

    for (size_t i = 0; input[i] && out_pos < output_size - 2; i++)
    {
        if (out_pos >= output_size - 3)
            break; // Need room for escape char, closing quote, and null

        switch (input[i])
        {
        case '"':
            output[out_pos++] = '\\';
            output[out_pos++] = '"';
            break;
        case '\\':
            output[out_pos++] = '\\';
            output[out_pos++] = '\\';
            break;
        case '\n':
            output[out_pos++] = '\\';
            output[out_pos++] = 'n';
            break;
        case '\r':
            output[out_pos++] = '\\';
            output[out_pos++] = 'r';
            break;
        case '\t':
            output[out_pos++] = '\\';
            output[out_pos++] = 't';
            break;
        default:
            if (input[i] < 32)
            {
                // Skip other control characters for safety
            }
            else
            {
                output[out_pos++] = input[i];
            }
            break;
        }
    }

    output[out_pos++] = '"';
    output[out_pos] = '\0';
    return out_pos;
}

// Safe buffer append with size checking
bool safe_append(char *buffer, size_t buffer_size, size_t *current_pos, const char *append_str)
{
    size_t append_len = strlen(append_str);
    if (*current_pos + append_len >= buffer_size)
    {
        ESP_LOGW(TAG, "Buffer overflow prevented in JSON construction");
        return false;
    }

    strcpy(buffer + *current_pos, append_str);
    *current_pos += append_len;
    return true;
}