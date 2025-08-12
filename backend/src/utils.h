// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#pragma once

#include <string.h>
#include <stdbool.h>

#define max(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })

#define min(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })

size_t json_escape_string(const char *input, char *output, size_t output_size);
bool safe_append(char *buffer, size_t buffer_size, size_t *current_pos, const char *append_str);