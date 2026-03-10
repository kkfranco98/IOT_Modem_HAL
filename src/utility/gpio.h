#pragma once

#include "enums.h"

namespace IOT_Modem_HAL::Utility
{
    struct GPIO_M
    {
        int pin = -1;
        Gpio_Mode mode = Gpio_Mode::UNDEFINED;
        bool active_high = true;

        GPIO_M() = default;
        GPIO_M(int p, Gpio_Mode m, bool ah = true) : pin(p), mode(m), active_high(ah) {}

        bool is_valid() const { return pin >= 0 && mode != Gpio_Mode::UNDEFINED; }
        bool connection_exists() const { return pin >= 0; }
    };
}