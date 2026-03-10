#include <Arduino.h>

#include "hardware_config.h"

namespace IOT_Modem_HAL::Utility
{
    inline void print_enum(Print &out, const Power_Control_Mode &m)
    {
        switch (m)
        {
        case Power_Control_Mode::AUTO_ON:
            out.print("AUTO_ON");
            break;
        case Power_Control_Mode::PWRKEY_ONLY:
            out.print("PWRKEY_ONLY");
            break;
        case Power_Control_Mode::UNDEFINED:
            out.print("UNDEFINED");
            break;
        default:
            out.print("ERROR");
            break;
        }
    }

    inline void print_enum(Print &out, const Power_State &s)
    {
        switch (s)
        {
        case Power_State::UNKNOWN:
            out.print("UNKNOWN");
            break;
        case Power_State::OFF:
            out.print("OFF");
            break;
        case Power_State::TURNING_ON:
            out.print("TURNING_ON");
            break;
        case Power_State::ON:
            out.print("ON");
            break;
        case Power_State::TURNING_OFF:
            out.print("TURNING_OFF");
            break;
        case Power_State::FAILED:
            out.print("FAILED");
            break;

        default:
            out.print("ERROR");
            break;
        }
    }

    inline void print_enum(Print &out, const Result &r)
    {
        switch (r)
        {
        case Result::OK:
            out.print("OK");
            break;
        case Result::TIMEOUT:
            out.print("TIMEOUT");
            break;
        case Result::NOT_SUPPORTED:
            out.print("NOT_SUPPORTED");
            break;
        case Result::BAD_CONFIG:
            out.print("BAD_CONFIG");
            break;
        case Result::BUSY:
            out.print("BUSY");
            break;
        case Result::FAIL:
            out.print("FAIL");
            break;
        default:
            out.print("ERROR");
            break;
        }
    }
}