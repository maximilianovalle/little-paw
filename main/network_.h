// network_.h: declares the functions to be shared, think of it as the 'interface' or 'menu' for network_.c

// Credit: https://developer.espressif.com/blog/getting-started-with-wifi-on-esp-idf/

#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"

esp_err_t network_init(void);
esp_err_t network_connect(char *wifi_ssid, char *wifi_password);
esp_err_t network_disconnect(void);
esp_err_t network_deinit(void);