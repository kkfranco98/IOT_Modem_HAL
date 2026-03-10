#pragma once

#include "hardware_config.h"

namespace IOT_Modem_HAL::Utility
{
    struct Modem_Status
    {
        Power_State power_state = Power_State::UNKNOWN;

        bool at_ok = false;
    };
}