#pragma once

#include <Arduino.h>

namespace IOT_Modem_HAL::Utility
{
    enum class Power_Control_Mode
    {
        UNDEFINED = -1,
        AUTO_ON,
        PWRKEY_ONLY,
    };

    enum class Power_Off_Method
    {
        AT_COMMAND,
        PWRKEY,
        AUTO
    };

    enum class Result
    {
        OK,
        TIMEOUT,
        NOT_SUPPORTED,
        BAD_CONFIG,
        BUSY,
        FAIL
    };

    enum class Gpio_Mode
    {
        UNDEFINED = -1,
        OUTPUT_M,
        INPUT_FLOATING_M,
        INPUT_PULLUP_M,
        INPUT_PULLDOWN_M
    };

    enum class Power_State
    {
        UNKNOWN,
        OFF,
        TURNING_ON,
        ON,
        TURNING_OFF,
        FAILED
    };
}