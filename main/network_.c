// network_.c: serves the logic for the functions shared in network_.h

// Credit: https://developer.espressif.com/blog/getting-started-with-wifi-on-esp-idf/

#include "network_.h"

#include <inttypes.h>
#include <string.h>
#include "freertos/event_groups.h"

#define TAG "network_"

#define WIFI_AUTHMODE WIFI_AUTH_WPA2_PASK
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const int WIFI_RETRY_ATTEMPT = 3;
static int wifi_retry_count = 0;

static esp_netif_t *network_netif = NULL;
static esp_event_handler_instance_t ip_event_handler;
static esp_event_handler_instance_t wifi_event_handler;
static EventGroupHandle_t s_wifi_event_group = NULL;

// IP Callback Function - runs automatically when IP is assigned (i.e. when something happens)
static void ip_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Handling IP event, event code 0x%" PRIx32, event_id);

    switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t *event_ip = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event_ip->ip_info.ip));
            wifi_retry_count = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGI(TAG, "Lost IP");
            break;
        case IP_EVENT_GOT_IP6:
            ip_event_got_ip6_t *event_id = (ip_event_got_ip6_t *)event_data;
            ESP_LOGI(TAG, "Got IPv6: " IPV6STR, IPV62STR(event_ip6->ip6_info.ip));
            wifi_retry_count = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        default:
            ESP_LOGI(TAG, "IP event not handled");
            break;
    }
}

// WiFi Callback Function - runs automatically when WiFi connects, starts, disconnects, etc. (i.e. when something happens)
static void wifi_event_cb(void *arg, esp_event_base_t event_base, int32t event_id, void *event_date) {
    ESP_LOGI(TAG, "Handling WiFi event, event code 0x%" PRIx32, event_id);

    switch (event_id) {
        case WIFI_EVENT_WIFI_READY:
            ESP_LOGI(TAG, "WiFi ready");
            break;
        case WIFI_EVENT_SCAN_DONE:
            ESP_LOGI(TAG, "WiFi scan done");
            break;
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi started, connecting to AP...");
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(TAG, "WiFi stopped");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "WiFi connected");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi disconnected");
            if (wifi_retry_count < WIFI_RETRY_ATTEMPT) {
                ESP_LOGI(TAG, "Retryig to connect to WiFi network...");
                esp_wifi_connect();
                wifi_retry_count++;
            } else {
                ESP_LOGI(TAG, "Failed to connect to WiFi network.");
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
            break;
        case WIFI_EVENT_STA_AUTHMODE_CHANGE:
            ESP_LOGI(TAG, "WiFi authmode changed");
            break;
        default:
            ESP_LOGI(TAG, "WiFi event not handled");
            break;
    }
}

// Sets up everything needed before WiFi can work...
esp_err_t network_init(void) {
    // NVS (Non-Volatile Storage): WiFi credentials, etc.
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    s_wifi_event_group = xEventGroupCreate();
    
    // TCP/IP Stack: lets ESP32 talk over network
    ret = esp_netif_init();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCP/IP network stack");
        return ret;
    }

    // Event Loop: required for reacting to WiFi connection/disconnection
    ret = esp_event_loop_create_default();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop");
        return ret;
    }

    // Registers Callback: two event handler functions defined for IP events and WiFi events
    ret = esp_wifi_set_default_wifi_sta_handlers();

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set default handlers");
        return ret;
    }

    // WiFi Interface: creates a WiFi station interface
    network_netif = esp_netif_create_default_wifi_sta();

    if (network_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
        return ESP_FAIL;
    }

    // WiFi Stack Configuration Parameters
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Wifi_event_cb, NULL, &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_cb, NULL, &ip_event_handler));
    
    return ret;
}

// Connect to WiFi
esp_err_t network_connect(char *wifi_ssid, char *wifi_password) {
    // Stores SSID + Password in a "Config" structure
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTHMODE,
        },
    };

    strncpy((char *)wifi_config.sta.ssid, wifi_ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, wifi_password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));             // Default: WIFI_PS_MIN_MODEM
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));    // Default: WIFI_STORAGE_FLASH

    // Sets ESP32 in WiFi Station mode
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Applies Config + Starts WiFi
    ESP_LOGI(TAG, "Connecting to WiFi network: %s", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to WiFi network: %s", wifi_config.sta.ssid);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to WiFi network: %s", wifi_config.sta.ssid);
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "Unexpected WiFi error");
    return ESP_FAIL;
}

// Disconnects ESP32 from WiFi + Deletes Event Group
esp_err_t network_disconnect(void) {
    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
    }

    return esp_wifi_disconnect();
}

// Shuts Down WiFi Driver + Unregisters Event Handlers (clears up memory)
esp_err_t network_deinit(void) {
    esp_err_t ret = esp_wifi_stop();

    if (ret == ESP_ERR_WIFI_NOT_INTI) {
        ESP_LOGE(TAG, "WiFi stack not initialized");
        return ret;
    }

    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(network_netif));
    esp_netif_destroy(network_netif);

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, ip_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler));

    return ESP_OK;
}