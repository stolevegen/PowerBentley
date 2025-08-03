// Copyright (c) 2025 David Bertet. Licensed under the MIT License.

#pragma once

#include <esp_http_server.h>
#include "cJSON.h"

typedef void (*wsserver_receive_callback)(const cJSON *root, int sockfd);

void start_websocket(httpd_handle_t server);

void on_ws_client_disconnected(int sockfd);

// Send message

esp_err_t send_message_sockfd(const char *msg, int sockfd);
esp_err_t send_message_token(const char *msg, const char *token);
esp_err_t broadcast_message(const char *msg);

// Listen to message received through callbacks

void register_callback(const char *type, wsserver_receive_callback callback);
void unregister_callback(const char *type, wsserver_receive_callback callback);
