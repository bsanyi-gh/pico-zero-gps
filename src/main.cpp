#include <Arduino.h>
#include <TFT_eSPI.h>

#include "SensorUtils.h"
#include "Utils.h"
#include "defines.h"
#include "pins.h"

//------------------ TFT
#include <TFT_eSPI.h>
TFT_eSPI tft;

/**
 * @brief Arduino setup() függvény
 */
void setup() {
#ifdef __DEBUG
    Serial.begin(115200);
#endif

    // Beeper
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    // TFT LED háttérvilágítás kimenet
    pinMode(PIN_TFT_BACKGROUND_LED, OUTPUT);
    Utils::setTftBacklight(TFT_BACKGROUND_LED_MAX_BRIGHTNESS); // TFT inicializálása DC módban
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK); // Fekete háttér a splash screen-hez

#if defined(__DEBUG) && defined(DEBUG_WAIT_FOR_SERIAL)
    // Várakozás a soros port megnyitására hibakereséshez
    Utils::debugWaitForSerial(tft);
#endif

    DEBUG("core-0:setup(): System clock: %u MHz\n", (unsigned)clock_get_hz(clk_sys) / 1000000u);
}

/**
 * @brief Arduino loop() függvény
 */
void loop() {

#define SENSOR_DISPLAY_INTERVAL_MS (30 * 1000UL) // 30 másodperc a szenzor adatok kiírási időköze
    static unsigned long lastDisplayTime = 0;
    Utils::timeHasPassed( //
        lastDisplayTime,  //
        SENSOR_DISPLAY_INTERVAL_MS,
        []() { //
            DEBUG("vBus=%.2f V, vSys=%.2f V, coreT=%.2f °C, extT=%.2f °C\n", sharedSensorData.vBus, sharedSensorData.vSys, sharedSensorData.coreTemperature, sharedSensorData.externalTemperature);
        });
}
