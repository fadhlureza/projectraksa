#pragma once

#include "esp_err.h"

namespace WebServer {
    esp_err_t start();
    void stop();
} // namespace WebServer