#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize all MimiClaw AI agent subsystems:
 * message bus, memory store, session manager, skill loader, cron, heartbeat, tools, and LLM proxy.
 */
esp_err_t mimi_agent_init(void);

/**
 * Start the agent loop and outbound message dispatcher tasks.
 * Should be called once system is booted and network is ready.
 */
esp_err_t mimi_agent_start(void);

#ifdef __cplusplus
}
#endif
