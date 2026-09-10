#include "network.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include <cstring>

namespace NetworkManager {

    WifiState current_wifi_state = WifiState::DISCONNECTED;

    WifiState get_wifi_state() {
        return current_wifi_state;
    }

    static const char *TAG = "NETWORK";

    static EventGroupHandle_t wifi_event_group;
    static const int WIFI_CONNECTED_BIT = BIT0;
    static const int WIFI_FAIL_BIT = BIT1;

    static esp_netif_t *ap_netif = nullptr;
    static esp_netif_t *sta_netif = nullptr;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data) {
    
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Sistem nyala. Mencoba auto-reconnect ke Wi-Fi terakhir...");

        current_wifi_state = WifiState::CONNECTING;

        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Connection failed! Incorrect password or network not found.");
        
        // Report failure to Web UI
        current_wifi_state = WifiState::FAILED; 
        
        // Auto-retry disabled
        
        xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        
        // Report success to Web UI
        current_wifi_state = WifiState::CONNECTED;
        
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Device connected to AP: " MACSTR, MAC2STR(event->mac));
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Device disconnected from AP: " MACSTR, MAC2STR(event->mac));
    }
}

    esp_err_t init() {
    // Initialize NVS if not already initialized
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ap_netif = esp_netif_create_default_wifi_ap();
    sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL,
        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL,
        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_LOGI(TAG, "Network Manager initialized successfully.");
    return ESP_OK;
    }

    esp_err_t start_ap_sta_mode(const std::string &ap_ssid,
                                const std::string &ap_password) {
    wifi_config_t ap_config = {};
    strncpy((char *)ap_config.ap.ssid, ap_ssid.c_str(),
            sizeof(ap_config.ap.ssid) - 1);
    ap_config.ap.ssid_len = ap_ssid.length();
    strncpy((char *)ap_config.ap.password, ap_password.c_str(),
            sizeof(ap_config.ap.password) - 1);
    ap_config.ap.max_connection = 4;

    if (ap_password.length() == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_ps(WIFI_PS_NONE);

    ESP_LOGI(TAG, "AP_STA mode active. SSID: %s", ap_ssid.c_str());
    return ESP_OK;
    }

    esp_err_t connect_sta(const std::string &sta_ssid,
                        const std::string &sta_password) {
    wifi_config_t sta_config = {};
    strncpy((char *)sta_config.sta.ssid, sta_ssid.c_str(),
            sizeof(sta_config.sta.ssid) - 1);
    strncpy((char *)sta_config.sta.password, sta_password.c_str(),
            sizeof(sta_config.sta.password) - 1);

    if (sta_password.empty()) {
        sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    } else {
        sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

    ESP_LOGI(TAG, "Attempting to connect to SSID: %s...", sta_ssid.c_str());
    esp_wifi_connect();

    EventBits_t bits =
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                            pdFALSE, pdFALSE, pdMS_TO_TICKS(10000));

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
    }

    esp_err_t scan_networks(std::vector<NetworkInfo> &found_networks) {
    found_networks.clear();

    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = false;

    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK)
        return ret;

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count == 0) {
        ESP_LOGI(TAG, "No Wi-Fi networks found nearby.");
        return ESP_OK;
    }

    wifi_ap_record_t *ap_records = new wifi_ap_record_t[ap_count];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));

    for (int i = 0; i < ap_count; i++) {
        NetworkInfo info;
        info.ssid = (char *)ap_records[i].ssid;
        info.rssi = ap_records[i].rssi;
        info.auth_mode = ap_records[i].authmode;
        found_networks.push_back(info);
    }

    delete[] ap_records;
    ESP_LOGI(TAG, "Successfully found %d Wi-Fi networks.", ap_count);
    return ESP_OK;
    }
} // namespace NetworkManager