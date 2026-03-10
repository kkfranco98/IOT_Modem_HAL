/*
   MinimalStart.ino

   Minimal example for IOT_Modem_HAL.
   It shows how to:
   - configure the modem hardware
   - initialize the HAL
   - power on the modem
   - verify AT communication

   Update the pins below according to your board.
*/

#include <Arduino.h>
#include <IOT_Modem_HAL.h>

// ============================================================
// Select the serial port used by the modem
// Change this depending on your board/platform
// ============================================================
HardwareSerial ModemSerial(2);

// ============================================================
// Select your modem class
// ============================================================
static IOT_Modem_HAL::Modems::SIM7600GH modem_hw(ModemSerial);

// ============================================================
// User configuration
// Replace these values with the correct ones for your board
// Set unused pins to -1
// ============================================================
static const uint32_t MODEM_BAUD = 115200;

static const int MODEM_RX_PIN = 16;     // ESP RX  <- Modem TX
static const int MODEM_TX_PIN = 17;     // ESP TX  -> Modem RX
static const int MODEM_PWRKEY_PIN = 4;  // Modem PWRKEY
static const int MODEM_RST_PIN = 5;     // Modem RESET (optional)
static const int MODEM_STATUS_PIN = 18; // Modem STATUS (optional)

// ============================================================
// Helper
// ============================================================
static void print_result(const char *label, IOT_Modem_HAL::Utility::Result r)
{
    Serial.print(label);
    Serial.print(": ");

    switch (r)
    {
    case IOT_Modem_HAL::Utility::Result::OK:
        Serial.println("OK ✅");
        break;
    case IOT_Modem_HAL::Utility::Result::BAD_CONFIG:
        Serial.println("BAD_CONFIG ❌");
        break;
    case IOT_Modem_HAL::Utility::Result::TIMEOUT:
        Serial.println("TIMEOUT ⏳");
        break;
    case IOT_Modem_HAL::Utility::Result::NOT_SUPPORTED:
        Serial.println("NOT_SUPPORTED ⚠️");
        break;
    default:
        Serial.print("ERROR (");
        Serial.print((int)r);
        Serial.println(") ❌");
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1500);

    Serial.println();
    Serial.println("==================================");
    Serial.println("IOT_Modem_HAL - MinimalStart 📡");
    Serial.println("==================================");

    using namespace IOT_Modem_HAL::Utility;

    Modem_Hardware_Config cfg;
    cfg.baud = MODEM_BAUD;

    cfg.rx_gpio.pin = MODEM_RX_PIN;
    cfg.rx_gpio.mode = Gpio_Mode::INPUT_FLOATING_M;
    cfg.rx_gpio.active_high = true;

    cfg.tx_gpio.pin = MODEM_TX_PIN;
    cfg.tx_gpio.mode = Gpio_Mode::OUTPUT_M;
    cfg.tx_gpio.active_high = true;

    // Choose how the modem is powered on
    cfg.power_control_mode = (MODEM_PWRKEY_PIN >= 0) ? Power_Control_Mode::PWRKEY_ONLY
                                                     : Power_Control_Mode::AUTO_ON;

    // Let the library choose the best power-off method
    cfg.power_off_method = Power_Off_Method::AUTO;

    if (MODEM_PWRKEY_PIN >= 0)
    {
        cfg.pwrkey_gpio.pin = MODEM_PWRKEY_PIN;
        cfg.pwrkey_gpio.mode = Gpio_Mode::OUTPUT_M;
        cfg.pwrkey_gpio.active_high = true;
    }

    if (MODEM_RST_PIN >= 0)
    {
        cfg.rst_gpio.pin = MODEM_RST_PIN;
        cfg.rst_gpio.mode = Gpio_Mode::OUTPUT_M;
        cfg.rst_gpio.active_high = true;
    }

    if (MODEM_STATUS_PIN >= 0)
    {
        cfg.status_gpio.pin = MODEM_STATUS_PIN;
        cfg.status_gpio.mode = Gpio_Mode::INPUT_FLOATING_M;
        cfg.status_gpio.active_high = true;
    }

    Serial.println("Initializing modem HAL...");
    Result r = modem_hw.begin(cfg);
    print_result("begin()", r);

    if (r != Result::OK)
    {
        Serial.println("Cannot continue. Check pin configuration.");
        return;
    }

    Serial.println("Powering on modem...");
    r = modem_hw.power_on();
    print_result("power_on()", r);

    Serial.println("Waiting for AT response...");
    bool at_ok = modem_hw.raw_at_ok(1000);

    if (at_ok)
    {
        Serial.println("Modem is ready 🎉");
        Serial.print("Detected modem: ");
        Serial.println(modem_hw.name());
    }
    else
    {
        Serial.println("No AT response from modem ❌");
        Serial.println("Check wiring, power supply, baud rate and timing.");
    }
}

void loop()
{
    static uint32_t last_check = 0;

    if (millis() - last_check >= 5000)
    {
        last_check = millis();

        bool at_ok = modem_hw.raw_at_ok(500);

        Serial.print("[AT CHECK] ");
        if (at_ok)
            Serial.println("OK ✅");
        else
            Serial.println("NO RESPONSE ❌");
    }
}