#include "mimi_agent.h"
#include "mimi_config.h"
#include "bus/message_bus.h"
#include "memory/memory_store.h"
#include "memory/session_mgr.h"
#include "skills/skill_loader.h"
#include "cron/cron_service.h"
#include "heartbeat/heartbeat.h"
#include "tools/tool_registry.h"
#include "llm/llm_proxy.h"
#include "agent/agent_loop.h"

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "mimi_agent";

/* Weak forward declaration of web chat deliverer (implemented in Phase 6 web_chat.cpp) */
extern esp_err_t web_chat_deliver(const char *client_id, const char *text) __attribute__((weak));

static void outbound_dispatcher_task(void *arg)
{
    ESP_LOGI(TAG, "Outbound dispatcher started on Core %d", xPortGetCoreID());

    while (1) {
        mimi_msg_t msg;
        esp_err_t err = message_bus_pop_outbound(&msg, UINT32_MAX);
        if (err != ESP_OK) continue;

        if (strcmp(msg.channel, MIMI_CHAN_WEB) == 0 || strcmp(msg.channel, "websocket") == 0) {
            if (web_chat_deliver) {
                web_chat_deliver(msg.chat_id, msg.content);
            } else {
                ESP_LOGI(TAG, "[WEB CHAT DISPATCH - %s] %s", msg.chat_id, msg.content);
            }
        } else if (strcmp(msg.channel, MIMI_CHAN_SYSTEM) == 0) {
            ESP_LOGI(TAG, "[SYSTEM EVENT - %s] %s", msg.chat_id, msg.content);
        } else {
            ESP_LOGW(TAG, "Unrecognized channel '%s': %s", msg.channel, msg.content);
        }

        if (msg.content) {
            free(msg.content);
        }
    }
}

esp_err_t mimi_agent_init(void)
{
    ESP_LOGI(TAG, "Initializing RAKSA AI Agent subsystems...");

    esp_err_t err = message_bus_init();
    if (err != ESP_OK) return err;

    memory_store_init();
    session_mgr_init();
    skill_loader_init();
    cron_service_init();
    heartbeat_init();

    err = tool_registry_init();
    if (err != ESP_OK) return err;

    err = llm_proxy_init();
    if (err != ESP_OK) return err;

    err = agent_loop_init();
    if (err != ESP_OK) return err;

    ESP_LOGI(TAG, "RAKSA AI Agent subsystems initialized successfully");
    return ESP_OK;
}

esp_err_t mimi_agent_start(void)
{
    ESP_LOGI(TAG, "Starting RAKSA AI Agent background tasks...");

    /* Start Agent Loop on Core 1 */
    esp_err_t err = agent_loop_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start agent_loop task");
        return err;
    }

    /* Start Outbound Dispatcher on Core 0 */
    BaseType_t ret = xTaskCreatePinnedToCore(
        outbound_dispatcher_task,
        "outbound_dispatch",
        MIMI_OUTBOUND_STACK,
        NULL,
        MIMI_OUTBOUND_PRIO,
        NULL,
        MIMI_OUTBOUND_CORE
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create outbound_dispatch task");
        return ESP_FAIL;
    }

    /* Start background cron & heartbeat */
    cron_service_start();
    heartbeat_start();

    ESP_LOGI(TAG, "RAKSA AI Agent started and listening for incoming queries");
    return ESP_OK;
}
