#pragma once
#include <stdbool.h>

void mqtt_start(void);      // call when STA gets an IP
void mqtt_stop(void);       // call when STA disconnects
bool mqtt_is_running(void);

int  mqtt_publish_str(const char *topic, const char *payload, int qos, bool retain);
int  mqtt_publish_f(const char *topic, float value, int qos, bool retain);
