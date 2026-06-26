#include <Arduino.h>

#include "Utils.h"
#include "defines.h"
#include "pins.h"

// A Core-1-nek stack külön legyen a Core-0-tól
// https://arduino-pico.readthedocs.io/en/latest/multicore.html#stack-sizes
bool core1_separate_stack = true;

// A konfiguráció betöltődött a Core0-án?
extern volatile bool configLoaded;

// -------------------- SensorUtils
#include "SensorUtils.h"
SensorUtils sensorUtils;

// -------------------- GPS
#include "GpsManager.h"
GpsManager *gpsManager = nullptr;

/**
 * Core-1 szenzor olvasás, itt történik az ADC1 és ADC4 olvasása és a változók frissítése
 */
void readSensorsOnCore1() {

    // Mérés és megosztott adat frissítése (Core 1 az egyetlen írófél)
    sharedSensorData.vBus = sensorUtils.readVBusExternal();
    sharedSensorData.vSys = sensorUtils.readVSysExternal();
    sharedSensorData.coreTemperature = sensorUtils.readCoreTemperature();
    sharedSensorData.externalTemperature = sensorUtils.readExternalTemperature();
}

/**
 * @brief Core-1 setup, itt inicializáljuk a szenzorokat és a változókat
 */
void setup1() {

    // GPS Serial
    Serial1.setRX(PIN_SERIAL1_RX_NEW);
    Serial1.setTX(PIN_SERIAL1_TX_NEW);
    Serial1.begin(9600);

    // Várakozás a konfiguráció betöltésére, hogy a GPS-t be tudjuk állítani
    while (!configLoaded) {
        delay(100);
    }

    // GPS Init + konfiguráció
    gpsManager = new GpsManager(Serial1);

    // Szenzor inicializálása (mutex init is itt történik belül)
    sensorUtils.init();

    // Első mérés a változók feltöltésére
    readSensorsOnCore1();

    CORE1_DEBUG("setup1(): System clock: %u MHz\n", (unsigned)clock_get_hz(clk_sys) / 1000000u);
}

/**
 * @brief Core-1 fő ciklus, itt fut a szenzorok olvasása és a változók frissítése
 */

#define SENSOR_READ_INTERVAL_MS (30 * 1000UL) // 30 másodperc a szenzor mérési időköz

void loop1() {

    // GPS olvasás
    gpsManager->loop();

    // Szenzorok karbantartása (DS18B20 non-blocking loop)
    sensorUtils.loop();

    // Mérés és megosztott adat frissítése (Core 1 az egyetlen írófél)
    static unsigned long lastSensorReadTime = 0;
    Utils::timeHasPassed(lastSensorReadTime, SENSOR_READ_INTERVAL_MS, []() {
        readSensorsOnCore1();
        CORE1_DEBUG("vBus=%.2f V, vSys=%.2f V, coreT=%.2f °C, extT=%.2f °C\n", sharedSensorData.vBus, sharedSensorData.vSys, sharedSensorData.coreTemperature, sharedSensorData.externalTemperature);
    });

    sleep_ms(5);
}
