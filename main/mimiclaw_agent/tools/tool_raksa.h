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

/**
 * Tool: raksa_relay
 * Control main power relay with strict safety confirmation.
 * Input JSON: { "state": 0 or 1, "confirm": true or false }
 * - state 0 = OFF (cut off power load)
 * - state 1 = ON (restore power load)
 * Safety check: Requires explicit confirm=true, and blocks turning ON if hazardous conditions persist.
 */
esp_err_t tool_raksa_relay_execute(const char *input_json, char *output, size_t output_size);

#ifdef __cplusplus
}
#endif
