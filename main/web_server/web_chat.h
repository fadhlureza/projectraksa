#pragma once

#include "esp_err.h"
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize web chat subsystem with active httpd server handle.
 */
esp_err_t web_chat_init(httpd_handle_t server);

/**
 * @brief Register /ws/chat WebSocket endpoint to the httpd server.
 */
esp_err_t web_chat_register_uri(httpd_handle_t server);

/**
 * @brief Deliver outbound message from AI agent to the connected WebSocket client.
 * Called from mimi_agent.c outbound dispatcher task.
 * @param client_id Target client FD (or empty to broadcast to all clients)
 * @param text Response text from the AI agent
 */
esp_err_t web_chat_deliver(const char *client_id, const char *text);

#ifdef __cplusplus
}
#endif
