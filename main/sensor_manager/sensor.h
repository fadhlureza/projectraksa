#pragma once

#include "constant.h" // For SystemData_t

namespace SensorManager {

    /**
     * @brief Initialize all hardware sensors and mutexes
     */
    void init();

    /**
     * @brief Start the FreeRTOS task that polls sensors
     */
    void start_task();

    /**
     * @brief Thread-safe getter for current sensor data
     * @return SystemData_t a copy of the latest sensor values
     */
    SystemData_t get_data();

} // namespace SensorManager
