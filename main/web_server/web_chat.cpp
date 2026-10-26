#include "web_chat.h"
#include "mimiclaw_agent/bus/message_bus.h"
#include "cJSON.h"
#include <esp_http_server.h>
#include <esp_log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "web_chat";

#define MAX_WS_CLIENTS 8
#define MAX_INBOUND_PAYLOAD 2048

static httpd_handle_t s_server = nullptr;
static int s_client_fds[MAX_WS_CLIENTS];
static SemaphoreHandle_t s_client_mutex = nullptr;

struct ws_async_send_ctx {
    httpd_handle_t hd;
    int fd;
    char *payload;
};

static void init_clients(void) {
    if (!s_client_mutex) {
        s_client_mutex = xSemaphoreCreateMutex();
        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
            s_client_fds[i] = -1;
        }
    }
}

static void add_client(int fd) {
    if (!s_client_mutex) init_clients();
    if (xSemaphoreTake(s_client_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Check if already registered
        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
            if (s_client_fds[i] == fd) {
                xSemaphoreGive(s_client_mutex);
                return;
            }
        }
        // Insert into first empty slot
        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
            if (s_client_fds[i] == -1) {
                s_client_fds[i] = fd;
                ESP_LOGI(TAG, "Registered WS client FD %d (slot %d)", fd, i);
                break;
            }
        }
        xSemaphoreGive(s_client_mutex);
    }
}

static void remove_client(int fd) {
    if (!s_client_mutex) return;
    if (xSemaphoreTake(s_client_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < MAX_WS_CLIENTS; i++) {
            if (s_client_fds[i] == fd) {
                s_client_fds[i] = -1;
                ESP_LOGI(TAG, "Unregistered WS client FD %d (slot %d)", fd, i);
                break;
            }
        }
        xSemaphoreGive(s_client_mutex);
    }
}

/* Worker callback executed on httpd server thread for asynchronous sending */
static void ws_async_send_cb(void *arg)
{
    auto *ctx = static_cast<ws_async_send_ctx *>(arg);
    if (!ctx) return;

    if (httpd_ws_get_fd_info(ctx->hd, ctx->fd) == HTTPD_WS_CLIENT_WEBSOCKET) {
        httpd_ws_frame_t ws_pkt = {};
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;
        ws_pkt.payload = (uint8_t *)ctx->payload;
        ws_pkt.len = strlen(ctx->payload);

        esp_err_t err = httpd_ws_send_frame_async(ctx->hd, ctx->fd, &ws_pkt);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to deliver frame to FD %d (%s), unregistering", ctx->fd, esp_err_to_name(err));
            remove_client(ctx->fd);
        } else {
            ESP_LOGI(TAG, "Delivered AI message to WS client FD %d (%d bytes)", ctx->fd, (int)ws_pkt.len);
        }
    } else {
        remove_client(ctx->fd);
    }

    if (ctx->payload) {
        free(ctx->payload);
    }
    free(ctx);
}

/* WebSocket handler for /ws/chat */
static esp_err_t ws_chat_handler(httpd_req_t *req)
{
    int fd = httpd_req_to_sockfd(req);

    if (req->method == HTTP_GET) {
        // Handshake request
        ESP_LOGI(TAG, "Handshake completed on /ws/chat for socket FD %d", fd);
        add_client(fd);
        return ESP_OK;
    }

    // Packet reception
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // First call to determine packet length and frame type
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "httpd_ws_recv_frame length check failed (%d), removing client FD %d", ret, fd);
        remove_client(fd);
        return ret;
    }

    // Handle WebSocket close frames (even if payload length is 0)
    if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "WS client FD %d sent close frame", fd);
        remove_client(fd);
        return ESP_OK;
    }

    if (ws_pkt.len == 0) {
        return ESP_OK;
    }

    if (ws_pkt.len > MAX_INBOUND_PAYLOAD) {
        ESP_LOGW(TAG, "WS payload too large (%u bytes), max is %d", (unsigned)ws_pkt.len, MAX_INBOUND_PAYLOAD);
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t *buf = (uint8_t *)calloc(1, ws_pkt.len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate memory for WS payload");
        return ESP_ERR_NO_MEM;
    }

    ws_pkt.payload = buf;
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
        free(buf);
        return ret;
    }

    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        ESP_LOGI(TAG, "Received WS packet from FD %d: %s", fd, (char *)buf);
        add_client(fd);

        // Check if message is JSON
        char *user_prompt = nullptr;
        cJSON *root = cJSON_Parse((char *)buf);
        if (root) {
            // Check for heartbeat ping
            cJSON *type_item = cJSON_GetObjectItem(root, "type");
            if (type_item && cJSON_IsString(type_item) && strcmp(type_item->valuestring, "ping") == 0) {
                httpd_ws_frame_t pong_pkt = {};
                pong_pkt.type = HTTPD_WS_TYPE_TEXT;
                pong_pkt.payload = (uint8_t *)"{\"type\":\"pong\"}";
                pong_pkt.len = 15;
                httpd_ws_send_frame(req, &pong_pkt);
                cJSON_Delete(root);
                free(buf);
                return ESP_OK;
            }

            cJSON *msg_item = cJSON_GetObjectItem(root, "message");
            if (cJSON_IsString(msg_item) && msg_item->valuestring && strlen(msg_item->valuestring) > 0) {
                user_prompt = strdup(msg_item->valuestring);
            }
            cJSON_Delete(root);
        }

        // Fallback to raw text if not JSON
        if (!user_prompt) {
            user_prompt = strdup((char *)buf);
        }

        if (user_prompt && strlen(user_prompt) > 0) {
            // Send typing/acknowledgment back to client immediately on this request
            cJSON *ack = cJSON_CreateObject();
            cJSON_AddStringToObject(ack, "type", "status");
            cJSON_AddStringToObject(ack, "status", "thinking");
            char *ack_str = cJSON_PrintUnformatted(ack);
            cJSON_Delete(ack);

            if (ack_str) {
                httpd_ws_frame_t ack_pkt = {};
                ack_pkt.type = HTTPD_WS_TYPE_TEXT;
                ack_pkt.payload = (uint8_t *)ack_str;
                ack_pkt.len = strlen(ack_str);
                httpd_ws_send_frame(req, &ack_pkt);
                free(ack_str);
            }

            // Build inbound bus message
            mimi_msg_t in_msg;
            memset(&in_msg, 0, sizeof(in_msg));
            strncpy(in_msg.channel, MIMI_CHAN_WEB, sizeof(in_msg.channel) - 1);
            snprintf(in_msg.chat_id, sizeof(in_msg.chat_id), "%d", fd);
            in_msg.content = user_prompt; // Bus takes ownership

            esp_err_t push_err = message_bus_push_inbound(&in_msg);
            if (push_err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to push message to inbound bus: %s", esp_err_to_name(push_err));
                free(user_prompt);
            } else {
                ESP_LOGI(TAG, "Dispatched user prompt to MimiClaw Agent Loop (chat_id=%d)", fd);
            }
        } else if (user_prompt) {
            free(user_prompt);
        }
    }

    free(buf);
    return ESP_OK;
}

extern "C" esp_err_t web_chat_init(httpd_handle_t server)
{
    s_server = server;
    init_clients();
    ESP_LOGI(TAG, "Web chat subsystem initialized");
    return ESP_OK;
}

extern "C" esp_err_t web_chat_register_uri(httpd_handle_t server)
{
    if (!server) return ESP_ERR_INVALID_ARG;
    s_server = server;
    init_clients();

    httpd_uri_t ws_chat_uri = {
        .uri        = "/ws/chat",
        .method     = HTTP_GET,
        .handler    = ws_chat_handler,
        .user_ctx   = nullptr,
        .is_websocket = true,
        .handle_ws_control_frames = true,
        .supported_subprotocol = nullptr
    };

    esp_err_t err = httpd_register_uri_handler(server, &ws_chat_uri);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Registered WebSocket endpoint: /ws/chat");
    } else {
        ESP_LOGE(TAG, "Failed to register /ws/chat URI: %s", esp_err_to_name(err));
    }
    return err;
}

extern "C" esp_err_t web_chat_deliver(const char *client_id, const char *text)
{
    if (!s_server || !text) {
        return ESP_ERR_INVALID_STATE;
    }

    // Build JSON response
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "chat");
    cJSON_AddStringToObject(root, "role", "ai");
    cJSON_AddStringToObject(root, "content", text);
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json_str) {
        return ESP_ERR_NO_MEM;
    }

    int target_fd = -1;
    if (client_id && strlen(client_id) > 0 && strcmp(client_id, "all") != 0) {
        target_fd = atoi(client_id);
    }

    // Safely collect target FDs
    int clients_to_send[MAX_WS_CLIENTS];
    int count = 0;

    if (xSemaphoreTake(s_client_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (target_fd > 0) {
            // Direct unicast to client_id
            clients_to_send[count++] = target_fd;
        } else {
            // Broadcast to all active clients
            for (int i = 0; i < MAX_WS_CLIENTS; i++) {
                if (s_client_fds[i] != -1) {
                    clients_to_send[count++] = s_client_fds[i];
                }
            }
        }
        xSemaphoreGive(s_client_mutex);
    }

    // Queue delivery through httpd server work queue for thread safety
    for (int i = 0; i < count; i++) {
        int fd = clients_to_send[i];
        auto *ctx = (ws_async_send_ctx *)malloc(sizeof(ws_async_send_ctx));
        if (ctx) {
            ctx->hd = s_server;
            ctx->fd = fd;
            ctx->payload = strdup(json_str);
            if (!ctx->payload || httpd_queue_work(s_server, ws_async_send_cb, ctx) != ESP_OK) {
                if (ctx->payload) free(ctx->payload);
                free(ctx);
            }
        }
    }

    free(json_str);
    return ESP_OK;
}
