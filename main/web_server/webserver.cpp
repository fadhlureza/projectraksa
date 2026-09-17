#include "webserver.h"
#include "cJSON.h"   // Built-in ESP-IDF library for JSON parsing
#include "esp_wifi.h"
#include "esp_netif.h"
#include "network.h" // Required for Wi-Fi functions
#include <esp_http_server.h>
#include <esp_log.h>
#include <sys/param.h>
#include "constant.h"

#include "sensor_manager/sensor.h"

namespace WebServer {

static const char *TAG = "WEBSERVER";
static httpd_handle_t server = nullptr;

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");
extern const uint8_t script_js_start[] asm("_binary_script_js_start");
extern const uint8_t script_js_end[] asm("_binary_script_js_end");

static esp_err_t index_get_handler(httpd_req_t *req) {
        httpd_resp_set_type(req, "text/html");
        // Subtract 1 byte to exclude ESP-IDF null-terminator
        httpd_resp_send(req, (const char *)index_html_start, (index_html_end - index_html_start) - 1);
        return ESP_OK;
    }

    static esp_err_t css_get_handler(httpd_req_t *req) {
        httpd_resp_set_type(req, "text/css");
        httpd_resp_send(req, (const char *)style_css_start, (style_css_end - style_css_start) - 1);
        return ESP_OK;
    }

    static esp_err_t js_get_handler(httpd_req_t *req) {
        httpd_resp_set_type(req, "application/javascript");
        httpd_resp_send(req, (const char *)script_js_start, (script_js_end - script_js_start) - 1);
        return ESP_OK;
    }

static esp_err_t api_data_get_handler(httpd_req_t *req) {
    char json_resp[256];
    
    // Copy data safely using SensorManager getter
    SystemData_t safe_data = SensorManager::get_data();

    snprintf(json_resp, sizeof(json_resp),
             "{\"voltage\":%.1f, \"current\":%.3f, \"power\":%.1f, "
             "\"energy\":%.0f, \"frequency\":%.1f, \"power_factor\":%.2f, "
             "\"temp\":%.2f, \"door_open\":%s, \"smoke_detected\":%s, "
             "\"smoke_voltage\":%.2f, \"batt_volt\":%.2f, \"batt_pct\":%d}",
             safe_data.voltage, safe_data.current, safe_data.power,
             safe_data.energy, safe_data.frequency, safe_data.power_factor,
             safe_data.temperature, safe_data.door_open ? "true" : "false",
             safe_data.smoke_voltage > 1.6f ? "true" : "false", safe_data.smoke_voltage,
             safe_data.battery_voltage, safe_data.battery_percentage);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// --- NEW FEATURE: WI-FI SCAN API ---
static esp_err_t api_wifi_scan_handler(httpd_req_t *req) {
  std::vector<NetworkInfo> networks;
  NetworkManager::scan_networks(networks);

  cJSON *root = cJSON_CreateArray();
  for (const auto &net : networks) {
    cJSON *item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "ssid", net.ssid.c_str());
    cJSON_AddNumberToObject(item, "rssi", net.rssi);
    cJSON_AddItemToArray(root, item);
  }

  char *json_str = cJSON_PrintUnformatted(root);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);

  free(json_str);
  cJSON_Delete(root);
  return ESP_OK;
}

// --- NEW FEATURE: WI-FI CONNECT API ---
static esp_err_t api_wifi_connect_handler(httpd_req_t *req) {
  char buf[128];
  int ret, remaining = req->content_len;

  if (remaining >= sizeof(buf))
    return ESP_FAIL;
  if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0)
    return ESP_FAIL;
  buf[ret] = '\0';

  cJSON *root = cJSON_Parse(buf);
  if (root) {
    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass = cJSON_GetObjectItem(root, "password");
    if (ssid && pass) {
      ESP_LOGI(TAG, "Received connection request to: %s", ssid->valuestring);
      // Call connect function from network.cpp
      NetworkManager::connect_sta(ssid->valuestring, pass->valuestring);
    }
    cJSON_Delete(root);
  }

  httpd_resp_sendstr(req, "{\"status\":\"connecting\"}");
  return ESP_OK;
}

static esp_err_t api_wifi_status_handler(httpd_req_t *req) {
        NetworkManager::WifiState state = NetworkManager::get_wifi_state();
        const char* state_str = "disconnected";
        char ssid[33] = "";
        int rssi = 0;
        char ip_str[16] = "";

        if (state == NetworkManager::WifiState::CONNECTING) {
            state_str = "connecting";
        } else if (state == NetworkManager::WifiState::CONNECTED) {
            state_str = "connected";
            
            // Get Wi-Fi name and RSSI
            wifi_ap_record_t ap_info;
            if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
                snprintf(ssid, sizeof(ssid), "%s", ap_info.ssid);
                rssi = ap_info.rssi;
            }
            
            // Get local IP address
            esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif) {
                esp_netif_ip_info_t ip_info;
                esp_netif_get_ip_info(netif, &ip_info);
                snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
            }
        } else if (state == NetworkManager::WifiState::FAILED) {
            state_str = "failed";
        }

        char json_resp[200];
        snprintf(json_resp, sizeof(json_resp), "{\"status\":\"%s\", \"ssid\":\"%s\", \"rssi\":%d, \"ip\":\"%s\"}", state_str, ssid, rssi, ip_str);
        
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_resp, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    // --- NEW FEATURE: DISCONNECT API ---
    static esp_err_t api_wifi_disconnect_handler(httpd_req_t *req) {
        esp_wifi_disconnect();
        httpd_resp_sendstr(req, "{\"status\":\"disconnected\"}");
        return ESP_OK;
    }

static const httpd_uri_t uris[] = {
    {"/", HTTP_GET, index_get_handler, nullptr},
    {"/style.css", HTTP_GET, css_get_handler, nullptr},
    {"/script.js", HTTP_GET, js_get_handler, nullptr},
    {"/api/data", HTTP_GET, api_data_get_handler, nullptr},
    {"/api/wifi/scan", HTTP_GET, api_wifi_scan_handler, nullptr},
    {"/api/wifi/connect", HTTP_POST, api_wifi_connect_handler, nullptr},
    {"/api/wifi/status", HTTP_GET, api_wifi_status_handler, nullptr},
    {"/api/wifi/disconnect", HTTP_POST, api_wifi_disconnect_handler, nullptr}
};

esp_err_t start() {
    if (server != nullptr)
        return ESP_OK;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.max_uri_handlers = 10;

    config.max_open_sockets = 7;
        config.lru_purge_enable = true;

    if (httpd_start(&server, &config) == ESP_OK) {
        for (int i = 0; i < sizeof(uris) / sizeof(httpd_uri_t); i++) {
        httpd_register_uri_handler(server, &uris[i]);
        }
        return ESP_OK;
    }
    return ESP_FAIL;
}

void stop() {
    if (server != nullptr) {
        httpd_stop(server);
        server = nullptr;
    }
}
} // namespace WebServer