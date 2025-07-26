// Credit
// - https://developer.espressif.com/blog/getting-started-with-wifi-on-esp-idf/
// - https://www.espboards.dev/sensors/aht20/#esp-idf
// - https://github.com/SIMS-IOT-Devices/FreeRTOS-ESP-IDF-MySQL/blob/main/proj4.c

#include <stdio.h>
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "network_.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "_credentials.h"

#include <sys/param.h>
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "ca_cert.c"

#define I2C_MASTER_NUM I2C_NUM_0                                    // I2C port number, two ESP-IDF defaults: I2C_NUM_0 and I2C_NUM_!
#define I2C_MASTER_SDA_IO 21                                        // ESP32 SDA GPIO
#define I2C_MASTER_SCL_IO 22                                        // ESP32 SCL GPIO
#define I2C_MASTER_FREQ_HZ 100000                                   // I2C clock speed, standard: 100kHz
#define AHT20_ADDR 0x38                                             // AHT20 default I2C address: 0x38
#define AHT20_CMD_TRIGGER 0xAC                                      // Command: take measurement (AHT20 datasheet, section 5.3)
#define AHT20_CMD_SOFTRESET 0xBA                                    // Command: reset sensor
#define AHT20_CMD_INIT 0xBE                                         // Command: initialize sensor (AHT20 datasheet, section 5.3)

#define TAG "little-paw"



// Initialize I2C
void i2c_master_init() {
    // configuration object "conf" of type i2c_config_t
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,                                    // sets ESP32 as master device
        .sda_io_num = I2C_MASTER_SDA_IO,                            // uses "I2C_MASTER_SDA_IO" for data line
        .scl_io_num = I2C_MASTER_SCL_IO,                            // uses "I2C_MASTER_SCL_IO" for clock line
        .sda_pullup_en = GPIO_PULLUP_ENABLE,                        // turn on pull-up resistors for data line
        .scl_pullup_en = GPIO_PULLUP_ENABLE,                        // turn on pull-up resistors for clock line
        .master.clk_speed = I2C_MASTER_FREQ_HZ,                     // sets "I2C_MASTER_FREQ_HZ" as I2C clock speed
    };

    i2c_param_config(I2C_MASTER_NUM, &conf);                        // apply the defined configurations to "I2C_MASTER_NUM"
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);         // install I2C driver to ESP32
}

// Write Data to AHT20
esp_err_t aht20_write_command() {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();                                    // create new I2C transaction
    i2c_master_start(handle);                                                           // send start condition, start I2C transaction
    i2c_master_write_byte(handle, (AHT20_ADDR << 1) | I2C_MASTER_WRITE, true);          // SEND address byte
    i2c_master_write_byte(handle, 0xAC, true);                                          // 0xAC command (AHT20 datasheet, section 5.4)
    i2c_master_write_byte(handle, 0x33, true);                                          // first command parameter byte: 0x33 (AHT20 datasheet, section 5.4)
    i2c_master_write_byte(handle, 0x00, true);                                          // second command parameter byte: 0x00 (AHT20 datasheet, section 5.4)
    i2c_master_stop(handle);                                                            // send stop condition, end I2C transaction
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(1000));  // execute the above steps
    i2c_cmd_link_delete(handle);                                                        // clean up
    return ret;
}

// Read Data from AHT20
esp_err_t aht20_read_data(uint8_t *data, size_t length) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();                                    // create new I2C transaction
    i2c_master_start(handle);                                                           // send start condition, start I2C transaction
    i2c_master_write_byte(handle, (AHT20_ADDR << 1) | I2C_MASTER_READ, true);           // READ address byte
    i2c_master_read(handle, data, length, I2C_MASTER_LAST_NACK);                        // READ data byte
    i2c_master_stop(handle);                                                            // send stop condition, end I2C transaction
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(1000));  // execute the above steps
    i2c_cmd_link_delete(handle);                                                        // clean up
    return ret;
}

// Reset + Initialize AHT20
void aht20_init() {
    aht20_write_command(AHT20_CMD_SOFTRESET);                           // reset AHT20 using "AHT20_CMD_SOFTRESET"
    vTaskDelay(pdMS_TO_TICKS(40));                                      // wait after reset, give AHT20 time to process
    aht20_write_command(AHT20_CMD_INIT);                                // initialize AHT20 using "AHT20_CMD_INIT"
    vTaskDelay(pdMS_TO_TICKS(40));                                      // wait after initialization, give AHT20 time to process
}

// Get temperature + humidity
void aht20_get_temp_humidity(float *tempFahrenheit, float *humidity) {
    uint8_t data[6];
    aht20_write_command();                                                              // start measuring temperature + humidity
    vTaskDelay(pdMS_TO_TICKS(80));                                                      // wait for measurement

    if (aht20_read_data(data, 6) == ESP_OK) {                                           // retrieve measurement
        uint32_t raw_humidity = ((data[1] << 16) | (data[2] << 8) | data[3]) >> 4;      // decodes humidity
        uint32_t raw_temperature = ((data[3] & 0x0F) << 16) | (data[4] << 8) | data[5]; // decodes temperature

        // raw numbers converted to real-world units
        *humidity = ((float)raw_humidity / 1048576.0) * 100.0;
        float tempCelcius = ((float)raw_temperature / 1048576.0) * 200.0 - 50.0;
        *tempFahrenheit = (tempCelcius * 1.8) + 32;
    } else {
        *humidity = -1.0;
        *tempFahrenheit = -1.0;
    }
}



// Handle POST
esp_err_t client_post_handler(esp_http_client_event_handle_t evnt) {
    switch (evnt->event_id) {
        case HTTP_EVENT_ON_DATA:
            printf("HTTP_EVENT_ON_DATA: %.*s\n", evnt->data_len, (char *)evnt->data);
            break; 
        default:
            break;
    }

    return ESP_OK;
}

// POST to backend
static void client_post_function(float temp, float humidity) {
    esp_http_client_config_t config_post = {
        .url = BACKEND_POST_URL,
        .method = HTTP_METHOD_POST,
        .event_handler = client_post_handler,
        .cert_pem = (const char *)ca_pem
    };

    esp_http_client_handle_t client_post = esp_http_client_init(&config_post);

    // Format sensor data to JSON
    char post_data[128];
    snprintf(post_data, sizeof(post_data), "{\"tempFahrenheit\": %.2f, \"humidityPercentage\": %.2f}", temp, humidity);

    esp_http_client_set_post_field(client_post, post_data, strlen(post_data));
    esp_http_client_set_header(client_post, "Content-Type", "application/json");
    esp_http_client_set_header(client_post, "x-device-secret", ESP32_KEY);

    esp_http_client_perform(client_post);
    esp_http_client_cleanup(client_post);
}



void app_main(void) {

    // WiFi Connection

    ESP_ERROR_CHECK(network_init());
    esp_err_t ret = network_connect(WIFI_SSID, WIFI_PASSWORD);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to WiFi network");
    }

    wifi_ap_record_t ap_info;
    ret = esp_wifi_sta_get_ap_info(&ap_info);

    if (ret == ESP_ERR_WIFI_CONN) {
        ESP_LOGE(TAG, "WiFi station interface not initialized");
    } else if (ret == ESP_ERR_WIFI_NOT_CONNECT) {
        ESP_LOGE(TAG, "WiFi station is not connected");
    } else {
        ESP_LOGI(TAG, "--- Access Point Information ---");
        ESP_LOG_BUFFER_HEX("MAC Address", ap_info.bssid, sizeof(ap_info.bssid));
        ESP_LOG_BUFFER_CHAR("SSID", ap_info.ssid, sizeof(ap_info.ssid));
        ESP_LOGI(TAG, "Primary Channel: %d", ap_info.primary);
        ESP_LOGI(TAG, "RSSI: %d", ap_info.rssi);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // AHT20 Sensor Data

    float temperature = 0.0;
    float humidity = 0.0;

    i2c_master_init();
    aht20_init();

    while (true) {
        aht20_get_temp_humidity(&temperature, &humidity);
        printf("\nTemperature: %.2f °F, Humidity: %.2f%%", temperature, humidity);
        vTaskDelay(pdMS_TO_TICKS(2000));

        // POST to Coogcast
        if (humidity != -1) {
            client_post_function(temperature, humidity);
        }

        vTaskDelay(pdMS_TO_TICKS(3600000 - 2000));   // 1 hour delay between readings, accounting for vTaskDelay(pdMS_TO_TICKS(2000))
    }
}