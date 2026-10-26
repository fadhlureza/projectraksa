#pragma once

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize time tool.
 */
esp_err_t tool_get_time_init(void);

/**
 * Tool: get_current_time
 * Read real-time clock timestamp from hardware DS3231 RTC.
 * Input JSON: {} (no required parameters)
 */
esp_err_t tool_get_time_execute(const char *input_json, char *output, size_t output_size);

#ifdef __cplusplus
}
#endif
