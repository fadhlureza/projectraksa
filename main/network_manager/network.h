#pragma once

#include <string>
#include <vector>
#include "esp_err.h"
#include "esp_wifi.h"

struct NetworkInfo {
    std::string ssid;
    int8_t rssi;
    wifi_auth_mode_t auth_mode;
};

namespace NetworkManager {
    esp_err_t init();
    esp_err_t start_ap_sta_mode(const std::string& ap_ssid, const std::string& ap_password);
    esp_err_t connect_sta(const std::string& sta_ssid, const std::string& sta_password);
    esp_err_t scan_networks(std::vector<NetworkInfo>& found_networks);
    enum class WifiState { DISCONNECTED, CONNECTING, CONNECTED, FAILED };
    WifiState get_wifi_state();
}