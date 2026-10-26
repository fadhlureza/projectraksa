#pragma once

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tool: raksa_status
 * Get real-time electrical & environmental telemetry from RAKSA sensors.
 * Input JSON: {} (no required parameters)
 */
esp_err_t tool_raksa_status_execute(const char *input_json, char *output, size_t output_size);

#ifdef __cplusplus
}
#endif
