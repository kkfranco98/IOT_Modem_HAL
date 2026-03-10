#pragma once

#include "gpio.h"

namespace IOT_Modem_HAL::Utility
{
    struct Modem_Hardware_Config
    {
        // UART
        uint32_t baud = 115200;
        GPIO_M rx_gpio;
        GPIO_M tx_gpio;

        // GPIO (opzionali)
        GPIO_M pwrkey_gpio;
        GPIO_M rst_gpio;
        GPIO_M status_gpio;

        // Power control
        Power_Control_Mode power_control_mode = Power_Control_Mode::UNDEFINED;
        Power_Off_Method power_off_method = Power_Off_Method::AUTO;

        bool is_valid() const { return rx_gpio.is_valid() && tx_gpio.is_valid() && power_control_mode != Power_Control_Mode::UNDEFINED; }
    };
}