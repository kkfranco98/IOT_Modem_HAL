#include "imodem.hpp"

namespace IOT_Modem_HAL::Core
{
    bool Modem_Interface::raw_at_ok(uint32_t timeout_ms)
    {
        if (!_modem_hw_config.is_valid())
            return false;

        HardwareSerial &ser = _used_serial;

        drain_serial(80, 5);
        serial_write_line("AT");

        // Cerchiamo "\nOK\r" e "\nERROR\r"
        uint8_t ok_idx = 0;
        uint8_t err_idx = 0;

        const char OK_PAT[] = "\nOK\r";
        const char ERR_PAT[] = "\nERROR\r";

        const uint32_t start = millis();
        while ((millis() - start) < timeout_ms)
        {
            while (ser.available() > 0)
            {
                const int v = ser.read();
                if (v < 0)
                    break;
                const char c = (char)v;

                if (feed_match_seq(ok_idx, c, OK_PAT))
                {
                    _modem_status.at_ok = true;
                    return true;
                }

                if (feed_match_seq(err_idx, c, ERR_PAT))
                {
                    _modem_status.at_ok = false;
                    return false;
                }
            }
            delay(1);
        }

        _modem_status.at_ok = false;
        return false;
    }

    Utility::Result Modem_Interface::init_gpio()
    {
        using namespace Utility;

        // --- Validazione in base alla modalità di controllo ---
        switch (_modem_hw_config.power_control_mode)
        {
        case Power_Control_Mode::AUTO_ON:
            // Nessun pin obbligatorio
            break;

        case Power_Control_Mode::PWRKEY_ONLY:

            // In questa modalità PWRKEY deve esistere
            if (!_modem_hw_config.pwrkey_gpio.connection_exists())
                return Result::BAD_CONFIG;

            if (setup_optional_gpio(_modem_hw_config.pwrkey_gpio) != Result::OK)
                return Result::BAD_CONFIG;

            break;

        default:
            return Result::FAIL;
        }

        // --- GPIO opzionali globali (se presenti li configuro) ---
        if (setup_optional_gpio(_modem_hw_config.rst_gpio) != Result::OK)
            return Result::BAD_CONFIG;

        if (setup_optional_gpio(_modem_hw_config.status_gpio) != Result::OK)
            return Result::BAD_CONFIG;

        return Result::OK;
    }

    bool Modem_Interface::drain_serial(uint32_t max_duration_ms, uint32_t idle_gap_ms)
    {
        HardwareSerial &ser = _used_serial;

        const uint32_t start = millis();
        uint32_t last_rx = millis();
        bool drained_any = false;

        while ((millis() - start) < max_duration_ms)
        {
            while (ser.available() > 0)
            {
                (void)ser.read();
                drained_any = true;
                last_rx = millis();
            }

            // se non arriva nulla da idle_gap_ms, consideriamo drain completo
            if ((millis() - last_rx) >= idle_gap_ms)
                break;

            delay(1); // yield FreeRTOS / WDT safe
        }

        return drained_any;
    }

    Utility::Result Modem_Interface::generate_pulse(const Utility::GPIO_M &gpio, uint32_t start_wait_ms, uint32_t pulse_duration_ms, uint32_t end_wait_ms)
    {
        if (!gpio.is_valid())
        {
            return Utility::Result::BAD_CONFIG;
        }
        if (gpio.mode != Utility::Gpio_Mode::OUTPUT_M)
        {
            return Utility::Result::BAD_CONFIG;
        }

        modem_digital_write(gpio.pin, false, gpio.active_high);
        this->wait_ms(start_wait_ms);

        modem_digital_write(gpio.pin, true, gpio.active_high);
        this->wait_ms(pulse_duration_ms);

        modem_digital_write(gpio.pin, false, gpio.active_high);
        this->wait_ms(end_wait_ms);

        return Utility::Result::OK;
    }

    Utility::Result Modem_Interface::generate_double_pulse(const Utility::GPIO_M &gpio,
                                                           uint32_t start_wait_ms, uint32_t pulse_1_duration_ms,
                                                           uint32_t gap_duration_ms,
                                                           uint32_t pulse_2_duration_ms, uint32_t end_wait_ms)
    {
        if (!gpio.is_valid())
        {
            return Utility::Result::BAD_CONFIG;
        }
        if (gpio.mode != Utility::Gpio_Mode::OUTPUT_M)
        {
            return Utility::Result::BAD_CONFIG;
        }

        modem_digital_write(gpio.pin, false, gpio.active_high);
        this->wait_ms(start_wait_ms);

        modem_digital_write(gpio.pin, true, gpio.active_high);
        this->wait_ms(pulse_1_duration_ms);

        modem_digital_write(gpio.pin, false, gpio.active_high);
        this->wait_ms(gap_duration_ms);

        modem_digital_write(gpio.pin, true, gpio.active_high);
        this->wait_ms(pulse_2_duration_ms);

        modem_digital_write(gpio.pin, false, gpio.active_high);
        this->wait_ms(end_wait_ms);

        return Utility::Result::OK;
    }

    bool Modem_Interface::setup_gpio_pin_mode(const Utility::GPIO_M &gpio)
    {
        if (!gpio.is_valid())
            return false;

        switch (gpio.mode)
        {
        case Utility::Gpio_Mode::INPUT_FLOATING_M:
            pinMode(gpio.pin, INPUT);
            break;

        case Utility::Gpio_Mode::INPUT_PULLUP_M:
            pinMode(gpio.pin, INPUT_PULLUP);
            break;

        case Utility::Gpio_Mode::INPUT_PULLDOWN_M:
            pinMode(gpio.pin, INPUT_PULLDOWN);
            break;

        case Utility::Gpio_Mode::OUTPUT_M:
            pinMode(gpio.pin, OUTPUT);
            break;

        default:
            return false;
        }

        return true;
    }

    bool Modem_Interface::send_at_command(const char *cmd, uint32_t timeout_ms)
    {
        while (_used_serial.available())
            _used_serial.read();

        this->_used_serial.print(cmd);
        this->_used_serial.print("\r");

        return wait_for_ok(timeout_ms);
    }

    bool Modem_Interface::wait_for_ok(uint32_t timeout_ms)
    {
        const uint32_t start = millis();
        char buffer[8];
        uint8_t idx = 0;

        while (millis() - start < timeout_ms)
        {
            while (_used_serial.available())
            {
                char c = _used_serial.read();

                if (c == '\r')
                    continue;

                if (c == '\n')
                {
                    buffer[idx] = '\0';

                    if (strcmp(buffer, "OK") == 0)
                        return true;

                    if (strcmp(buffer, "ERROR") == 0)
                        return false;

                    idx = 0;
                }
                else if (idx < sizeof(buffer) - 1)
                {
                    buffer[idx++] = c;
                }
            }

            delay(1);
        }

        return false;
    }
};
