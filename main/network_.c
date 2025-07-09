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

// IP Callback Function - runs automatically when something happens
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

// WiFi Callback Function - 
static void wifi_event_cb(void *arg, esp_event_base_t event_base, int32t event_id, void *event_date) {

}

// Sets up everything needed before WiFi can work...
esp_err_t network_init(void) {
    // NVS (Non-Volatile Storage): WiFi credentials, etc.
    // TCP/IP Stack: lets ESP32 talk over network
    // Event Loop: required for reacting to WiFi connection/disconnection
    // WiFi Interface: creates a WiFi station interface
    // Registers Callback: two event handler functions defined for IP events and WiFi events
}

esp_err_t network_connect(char *wifi_ssid, char *wifi_password) {

}

esp_err_t network_disconnect(void) {

}

esp_err_t network_deinit(void) {

}