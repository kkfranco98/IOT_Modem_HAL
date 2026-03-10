#pragma once

#include "utility/include.h"

namespace IOT_Modem_HAL::Core
{
    class Modem_Interface
    {
    protected:
        HardwareSerial &_used_serial;
        Utility::Modem_Hardware_Config _modem_hw_config;
        Utility::Modem_Status _modem_status;

    public:
        explicit Modem_Interface(HardwareSerial &ser) : _used_serial(ser) {}
        virtual ~Modem_Interface() {}

        // Identità modem
        virtual const char *name() const = 0;

        // ---- One-time setup ----
        Utility::Result begin(const Utility::Modem_Hardware_Config &cfg)
        {
            if (!cfg.is_valid())
                return Utility::Result::BAD_CONFIG;

            this->_modem_hw_config = cfg;

            this->_used_serial.end();
            delay(10);
            this->_used_serial.begin(cfg.baud, SERIAL_8N1, cfg.rx_gpio.pin, cfg.tx_gpio.pin);

            auto r = init_gpio();
            if (r != Utility::Result::OK)
                return r;

            return Utility::Result::OK;
        }

        // ---- Commons ----
        virtual bool raw_at_ok(uint32_t timeout_ms);

        // ---- Specifici ----
        virtual Utility::Power_State get_power_state() = 0;
        virtual Utility::Result power_on() = 0;
        virtual Utility::Result power_off() = 0;
        virtual Utility::Result reset() = 0;

        const Utility::Modem_Hardware_Config &get_modem_config() const { return this->_modem_hw_config; }
        const Utility::Modem_Status &get_modem_status() const { return this->_modem_status; }

    protected:
        virtual void update_power_state() = 0;

        bool drain_serial(uint32_t max_duration_ms = 100, uint32_t idle_gap_ms = 5);

        bool send_at_command(const char *cmd, uint32_t timeout_ms);
        bool wait_for_ok(uint32_t timeout_ms) ;

        Utility::Result init_gpio();

        virtual bool read_status_pin_for_power_good() const
        {
            if (!_modem_hw_config.status_gpio.is_valid())
                return false;

            const bool level = (digitalRead(_modem_hw_config.status_gpio.pin) == HIGH);
            return _modem_hw_config.status_gpio.active_high ? level : !level;
        }

        static void wait_ms(uint32_t ms)
        {
            const uint32_t start = millis();
            while ((millis() - start) < ms)
                delay(1);
        }

        Utility::Result generate_pulse(const Utility::GPIO_M &gpio, uint32_t start_wait_ms, uint32_t pulse_duration_ms, uint32_t end_wait_ms);

        Utility::Result generate_double_pulse(const Utility::GPIO_M &gpio,
                                              uint32_t start_wait_ms, uint32_t pulse_1_duration_ms,
                                              uint32_t gap_duration_ms,
                                              uint32_t pulse_2_duration_ms, uint32_t end_wait_ms);

        void modem_digital_write(int pin, bool value, bool is_active_high) { digitalWrite(pin, is_active_high ? value : !value); }

        bool setup_gpio_pin_mode(const Utility::GPIO_M &gpio);

        Utility::Result setup_optional_gpio(const Utility::GPIO_M &gpio)
        {
            if (!gpio.connection_exists())
                return Utility::Result::OK;

            if (!setup_gpio_pin_mode(gpio))
                return Utility::Result::BAD_CONFIG;

            if (gpio.mode == Utility::Gpio_Mode::OUTPUT_M)
                modem_digital_write(gpio.pin, false, gpio.active_high);

            return Utility::Result::OK;
        }

        void serial_write_line(const char *s)
        {
            _used_serial.print(s);
            _used_serial.print("\r\n");
        }

        // cerca una sequenza (pattern) in streaming: es. "OK" o "ERROR"
        static bool feed_match_seq(uint8_t &idx, char c, const char *pattern)
        {
            // pattern deve essere stringa C terminata da '\0'
            if (c == pattern[idx])
            {
                idx++;
                if (pattern[idx] == '\0')
                {
                    idx = 0;
                    return true;
                }
                return false;
            }

            // se mismatch ma il char corrente coincide col primo char del pattern,
            // riparti da 1 (utile per pattern tipo "OOOK")
            idx = (c == pattern[0]) ? 1 : 0;
            return false;
        }

        // pin validity helpers
        bool has_pwrkey_pin() const { return _modem_hw_config.pwrkey_gpio.is_valid(); }
        bool has_rst_pin() const { return _modem_hw_config.rst_gpio.is_valid(); }
        bool has_status_pin() const { return _modem_hw_config.status_gpio.is_valid(); }
    };
}
