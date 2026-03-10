#pragma once

#include "core/imodem.hpp"

namespace IOT_Modem_HAL::Modems
{
    class SIM7600GH : public Core::Modem_Interface
    {
    public:
        struct Timing
        {
            // PWRKEY pulse
            uint32_t pwrkey_start_wait_ms = 500; //! ok
            uint32_t pwrkey_ton_pulse_ms = 500;  //! ok
            uint32_t pwrkey_end_wait_ms = 100;   //! ok

            uint32_t ton_status_wait_ms = 13 * 1000UL; //! ok

            uint32_t toff_grace_at_ms = 17 * 1000UL; // TODO da verificare ma per il momento funziona
            uint32_t toff_grace_pwrkey_ms = 500;     // TODO in teoria non serve perché è bloccante ma lascio un tempo per sicurezza

            // reset pulse (datasheet: min 50ms, typ 100ms, max 500ms)
            uint32_t rst_start_wait_ms = 50;
            uint32_t rst_pulse_ms = 100; // tipico
            uint32_t rst_end_wait_ms = 50;

            // Boot / AT probe
            uint32_t at_probe_wait_ms = 200;
            uint32_t at_retry_wait_ms = 1000;
            uint32_t at_retry_count = 3;
            uint32_t stable_check_period_ms = 1000; // ogni 1s controlla ON/OFF

            // power off
            uint32_t _1_power_off_start_wait_ms = 100;                                          //! ok
            uint32_t _2_power_off_Ton_ms = 3000;                                                //! ok
            uint32_t _3_power_off_Toff_status_plus_Toff_on_ms = (26 * 1000UL) + (0.5 * 1000UL); //! ok
            uint32_t _4_power_off_Ton_ms = 500;                                                 //! ok
            uint32_t _5_power_off_end_wait_ms = 100;                                            //! ok
        };

    private:
        Timing _timing;

        uint32_t _power_on_start_ms = 0;
        uint32_t _power_off_start_ms = 0; // TODO: usato quando implementi TURNING_OFF

        uint8_t _unknown_at_tries = 0;
        uint32_t _unknown_last_probe_ms = 0;

        uint8_t _turnon_at_tries = 0;
        uint32_t _turnon_last_probe_ms = 0;

        uint8_t _turnoff_at_tries = 0;
        uint32_t _turnoff_last_probe_ms = 0;

        uint32_t _stable_last_check_ms = 0;

        Utility::Power_Off_Method _last_power_off_method =
            Utility::Power_Off_Method::AUTO;

    public:
        explicit SIM7600GH(HardwareSerial &ser)
            : Modem_Interface(ser) {}

        const char *name() const override { return "SIM7600GH"; }

        Timing &timing() { return _timing; }
        const Timing &timing() const { return _timing; }

        // ---- Specifici ----
        Utility::Power_State get_power_state() override
        {
            update_power_state();
            return _modem_status.power_state;
        }

        Utility::Result power_on() override
        {
            using namespace Utility;

            update_power_state();

            switch (_modem_status.power_state)
            {
            case Power_State::ON:
                return Result::OK;

            case Power_State::TURNING_ON:
            case Power_State::TURNING_OFF:
                return Result::BUSY;

            case Power_State::UNKNOWN:
            case Power_State::OFF:
            case Power_State::FAILED:
            {
                // Avvio accensione (decisa qui)
                switch (_modem_hw_config.power_control_mode)
                {
                case Power_Control_Mode::AUTO_ON:
                    // Nuova accensione: reset contatori (evita ereditare retry vecchi)
                    _unknown_at_tries = 0;
                    _unknown_last_probe_ms = 0;
                    _turnon_at_tries = 0;
                    _turnon_last_probe_ms = 0;

                    _power_on_start_ms = millis();
                    _modem_status.power_state = Power_State::TURNING_ON;
                    return Result::BUSY;

                case Power_Control_Mode::PWRKEY_ONLY:
                    if (!has_pwrkey_pin())
                        return Result::BAD_CONFIG;

                    {
                        auto r = generate_pulse(_modem_hw_config.pwrkey_gpio,
                                                _timing.pwrkey_start_wait_ms,
                                                _timing.pwrkey_ton_pulse_ms,
                                                _timing.pwrkey_end_wait_ms);
                        if (r != Result::OK)
                            return r;
                    }

                    // Nuova accensione: reset contatori (evita ereditare retry vecchi)
                    _unknown_at_tries = 0;
                    _unknown_last_probe_ms = 0;
                    _turnon_at_tries = 0;
                    _turnon_last_probe_ms = 0;

                    _turnoff_at_tries = 0;
                    _turnoff_last_probe_ms = 0;
                    _last_power_off_method = Power_Off_Method::AUTO;
                    _stable_last_check_ms = 0;

                    _power_on_start_ms = millis();
                    _modem_status.power_state = Power_State::TURNING_ON;
                    return Result::BUSY;

                default:
                    return Result::NOT_SUPPORTED;
                }
            }

            default:
                return Result::NOT_SUPPORTED;
            }
        }

        Utility::Result power_off() override
        {
            using namespace Utility;

            update_power_state();

            switch (_modem_status.power_state)
            {
            case Power_State::OFF:
                return Result::OK;

            case Power_State::TURNING_ON:
            case Power_State::TURNING_OFF:
                return Result::BUSY;

            case Power_State::UNKNOWN:
            case Power_State::ON:
            case Power_State::FAILED:
            {
                // 1) Risolvi metodo effettivo (AUTO -> AT se possibile, altrimenti PWRKEY)
                Power_Off_Method method = _modem_hw_config.power_off_method;

                if (method == Power_Off_Method::AUTO)
                {
                    // AUTO gentile: prova AT solo se risponde DAVVERO.
                    // Se UART staccata / modem non risponde, ripiego su PWRKEY.
                    const bool at_alive = raw_at_ok(_timing.at_probe_wait_ms);
                    method = at_alive ? Power_Off_Method::AT_COMMAND : Power_Off_Method::PWRKEY;
                }

                // 2) Esegui metodo (senza ulteriori switch)
                if (method == Power_Off_Method::AT_COMMAND)
                {
                    // Comando AT di spegnimento (TinyGSM). Non significa ancora OFF.
                    const bool sent = send_at_command("AT+CPOF", _timing.at_probe_wait_ms); // TODO da testare
                    if (!sent)
                        return Result::FAIL;

                    // Memorizza metodo usato (serve per grace period in TURNING_OFF)
                    _last_power_off_method = Power_Off_Method::AT_COMMAND;

                    // Avvia transizione
                    _power_off_start_ms = millis();
                    _modem_status.power_state = Power_State::TURNING_OFF;

                    // Reset retry per verifica OFF
                    _turnoff_at_tries = 0;
                    _turnoff_last_probe_ms = 0;

                    return Result::BUSY;
                }

                if (method == Power_Off_Method::PWRKEY)
                {
                    if (!has_pwrkey_pin())
                        return Result::NOT_SUPPORTED; // Se anche PWRKEY non è disponibile → NON è BAD_CONFIG, ma NOT_SUPPORTED

                    // Sequenza PWRKEY da datasheet (double pulse).
                    // NOTA: è blocking. Va bene se chiamato da task dedicato al modem.
                    auto r = generate_double_pulse(_modem_hw_config.pwrkey_gpio,
                                                   _timing._1_power_off_start_wait_ms,
                                                   _timing._2_power_off_Ton_ms,
                                                   _timing._3_power_off_Toff_status_plus_Toff_on_ms,
                                                   _timing._4_power_off_Ton_ms,
                                                   _timing._5_power_off_end_wait_ms);
                    if (r != Result::OK)
                        return r;

                    // Memorizza metodo usato (serve per grace period in TURNING_OFF)
                    _last_power_off_method = Power_Off_Method::PWRKEY;

                    // Avvia transizione (anche se la sequenza è finita, vogliamo verificare OFF)
                    _power_off_start_ms = millis();
                    _modem_status.power_state = Power_State::TURNING_OFF;

                    // Reset retry per verifica OFF
                    _turnoff_at_tries = 0;
                    _turnoff_last_probe_ms = 0;

                    return Result::BUSY;
                }

                return Result::NOT_SUPPORTED;
            }

            default:
                return Result::NOT_SUPPORTED;
            }
        }

        Utility::Result reset() override
        {
            using namespace Utility;

            if (!has_rst_pin())
                return Result::NOT_SUPPORTED;

            // RESET HARD: non mi interessa lo stato attuale
            auto r = generate_pulse(_modem_hw_config.rst_gpio,
                                    _timing.rst_start_wait_ms,
                                    _timing.rst_pulse_ms,
                                    _timing.rst_end_wait_ms);
            if (r != Result::OK)
                return r;

            // Dopo reset è come un boot: riparti pulito
            _unknown_at_tries = 0;
            _unknown_last_probe_ms = 0;
            _turnon_at_tries = 0;
            _turnon_last_probe_ms = 0;

            _turnoff_at_tries = 0;
            _turnoff_last_probe_ms = 0;

            _stable_last_check_ms = 0;
            _last_power_off_method = Power_Off_Method::AUTO;

            _power_on_start_ms = millis();
            _modem_status.power_state = Power_State::TURNING_ON;

            return Result::OK;
        }

    protected:
        void update_power_state() override
        {
            using namespace Utility;

            switch (_modem_status.power_state)
            {
            case Power_State::UNKNOWN:
            {
                // Se hai STATUS pin: risolvi subito senza AT
                if (has_status_pin())
                {
                    _modem_status.power_state = read_status_pin_for_power_good()
                                                    ? Power_State::ON
                                                    : Power_State::OFF;

                    _unknown_at_tries = 0;
                    _unknown_last_probe_ms = 0;
                    return;
                }

                // Senza STATUS: usa AT, ma leggero e con retry “spalmati”
                const uint32_t now = millis();

                const bool first_try = (_unknown_at_tries == 0);
                const bool time_to_retry = (now - _unknown_last_probe_ms) >= _timing.at_retry_wait_ms;

                if (first_try || time_to_retry)
                {
                    _unknown_last_probe_ms = now;

                    if (raw_at_ok(_timing.at_probe_wait_ms))
                    {
                        _modem_status.power_state = Power_State::ON;

                        _unknown_at_tries = 0;
                        _unknown_last_probe_ms = 0;
                        return;
                    }

                    _unknown_at_tries++;

                    if (_unknown_at_tries >= (uint8_t)_timing.at_retry_count)
                    {
                        _modem_status.power_state = Power_State::OFF;

                        _unknown_at_tries = 0;
                        _unknown_last_probe_ms = 0;
                        return;
                    }
                }

                return;
            }

            case Power_State::ON:
            case Power_State::OFF:
            {
                const uint32_t now = millis();
                if ((now - _stable_last_check_ms) < _timing.stable_check_period_ms)
                    return;

                _stable_last_check_ms = now;

                bool on = false;
                if (has_status_pin())
                {
                    on = read_status_pin_for_power_good();
                }
                else
                {
                    // probe leggero: se vuoi più robusto, usa 300-500ms
                    on = raw_at_ok(_timing.at_probe_wait_ms);
                }

                _modem_status.power_state = on ? Power_State::ON : Power_State::OFF;
                return;
            }

            case Power_State::TURNING_ON:
            {
                const uint32_t now = millis();
                const uint32_t elapsed = now - _power_on_start_ms;

                if (elapsed < _timing.ton_status_wait_ms)
                    return;

                // Se hai STATUS pin, chiudi subito (super affidabile e non blocca)
                if (has_status_pin())
                {
                    const bool on = read_status_pin_for_power_good();
                    _modem_status.power_state = on ? Power_State::ON : Power_State::FAILED;

                    _turnon_at_tries = 0;
                    _turnon_last_probe_ms = 0;
                    return;
                }

                // Senza STATUS: retry spalmati (non bloccare tutto in un colpo)
                const bool first_try = (_turnon_at_tries == 0);
                const bool time_to_retry = (now - _turnon_last_probe_ms) >= _timing.at_retry_wait_ms;

                if (first_try || time_to_retry)
                {
                    _turnon_last_probe_ms = now;

                    if (raw_at_ok(_timing.at_probe_wait_ms))
                    {
                        _modem_status.power_state = Power_State::ON;

                        _turnon_at_tries = 0;
                        _turnon_last_probe_ms = 0;
                        return;
                    }

                    _turnon_at_tries++;

                    if (_turnon_at_tries >= (uint8_t)_timing.at_retry_count)
                    {
                        _modem_status.power_state = Power_State::FAILED;

                        _turnon_at_tries = 0;
                        _turnon_last_probe_ms = 0;
                        return;
                    }
                }

                return;
            }

            case Power_State::TURNING_OFF:
            {
                const uint32_t now = millis();
                const uint32_t elapsed = now - _power_off_start_ms;

                // 1️⃣ Grace period diverso a seconda del metodo
                uint32_t grace_ms =
                    (_last_power_off_method == Power_Off_Method::AT_COMMAND)
                        ? _timing.toff_grace_at_ms
                        : _timing.toff_grace_pwrkey_ms;

                if (elapsed < grace_ms)
                    return;

                // 2️⃣ Se hai STATUS pin → verifica hardware diretta
                if (has_status_pin())
                {
                    const bool on = read_status_pin_for_power_good();
                    _modem_status.power_state = on ? Power_State::FAILED : Power_State::OFF;

                    _turnoff_at_tries = 0;
                    _turnoff_last_probe_ms = 0;
                    return;
                }

                // 3️⃣ Senza STATUS → OFF quando AT non risponde più
                const bool first_try = (_turnoff_at_tries == 0);
                const bool time_to_retry =
                    (now - _turnoff_last_probe_ms) >= _timing.at_retry_wait_ms;

                if (first_try || time_to_retry)
                {
                    _turnoff_last_probe_ms = now;

                    const bool at_alive = raw_at_ok(_timing.at_probe_wait_ms);

                    if (!at_alive)
                    {
                        _modem_status.power_state = Power_State::OFF;
                        _turnoff_at_tries = 0;
                        _turnoff_last_probe_ms = 0;
                        return;
                    }

                    _turnoff_at_tries++;

                    if (_turnoff_at_tries >= (uint8_t)_timing.at_retry_count)
                    {
                        _modem_status.power_state = Power_State::FAILED;
                        _turnoff_at_tries = 0;
                        _turnoff_last_probe_ms = 0;
                        return;
                    }
                }

                return;
            }

            default:
                // OFF / ON / FAILED: per ora non faccio nulla (FAILED latched)
                return;
            }
        }
    };
}
