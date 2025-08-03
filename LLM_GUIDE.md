# LLM Guide

# Generic rules that must always be applied

## Frontend

- Write valid Svelte 5 code
  - Use runes like $state, $props. Don't import them
  - Remove colon when necessary, ex: `onclick` instead of `on:click`
- You must use shadcn-ui to create a slick and modern UI
- Make sure you use the right imports for Svelte & Shadcn-UI

## Backend

- You must use ESP-IDF to access hardware
- Use #pragma once for header files

# Adding a new tab

## Frontend

- Add new tab in `frontend/src/components/tab`
- You must take example on `frontend/src/components/tab/SystemTab.svelte` for the file structure
- A table must have a `SectionHeader` to expose tab context
- Declare tab in `const tabs` on `frontend/src/App.svelte`. You must add it at the top

# Adding a WebSocket Endpoint

## Frontend

- Mock: Add a case for `<endpoint>` in `generateMockResponse(data)` (`frontend/src/lib/mockdata.js`). Returns a list, even if one response
  In any Svelte file, import `import { sendMessage, onMessageType } from 'src/lib/ws.svelte.js'`, then
- Send: `sendMessage({ type: '<endpoint>', ...payload });`
- Listen: Use `onMessageType('<response_type>', callback)` to consume responses

## Backend

- You can group related endpoints (ex: write/read) in the same file
- Handler: Implement `void ws_handle_<endpoint>(const cJSON *root, int sockfd);` in `backend/src/ws_<endpoint_context>.c/.h`. Take example on `backend/src/ws_settings.c`
- Register: Add `register_callback("<endpoint>", ws_handle_<endpoint>);` in `app_main()` (`backend/src/main.c`). Don't forget the include
- Respond: Use `send_message_sockfd(char* json, sockfd);` (single user), `broadcast_message(char* json);` (all users), or `send_message_token(char* json, token);` (by token). Token is used to target a user from an http request.

## Example

```js
import { sendMessage, onMessageType } from 'src/lib/ws.svelte.js'
// Send
sendMessage({ type: '<endpoint>', ...payload })
// Listen
const unsub = onMessageType('<response_type>', (data) => {
  /* handle */
})
```

## Summary

| Step     | File(s)                                 | Action                                                       |
| -------- | --------------------------------------- | ------------------------------------------------------------ |
| Handler  | backend/src/ws\_<endpoint_context>.c/.h | ws*handle*<endpoint>(...)                                    |
| Register | backend/src/main.c                      | register_callback(...)                                       |
| Respond  | backend/src/websocket.c                 | send_message_sockfd / broadcast_message / send_message_token |
| Mock     | frontend/src/lib/mockdata.js            | generateMockResponse                                         |
| Send     | frontend/src/lib/ws.svelte.js           | sendMessage                                                  |
| Listen   | frontend/src/lib/ws.svelte.js           | onMessageType                                                |

# Storage

## Backend

storage.h can be used to store floats and blobs in the NVS flash memory

```c
esp_err_t read_float(const char *key, float *value, float default_value);
esp_err_t write_float(const char *key, float value);

esp_err_t read_blob(const char *key, void *out_value, size_t *required_size);
esp_err_t write_blob(const char *key, const void *value, size_t required_size);
esp_err_t delete_blob(const char *key);
```
