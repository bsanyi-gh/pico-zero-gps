#include <Arduino.h>
#include <TFT_eSPI.h>

#include "Config.h"
#include "GpsManager.h"
#include "SensorUtils.h"
#include "TftBackLightAdjuster.h"
#include "Utils.h"
#include "defines.h"
#include "pins.h"

//------------------ Config
volatile bool configLoaded = false; // Jelezzük, hogy a config betöltődött

//------------------ TFT
TFT_eSPI tft;
TftBackLightAdjuster tftBackLightAdjuster;

//-------------------------------------------------------------------------
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
    // Háttérvilágítás beállítása a TFT kijelzőn
    tftBackLightAdjuster.begin();

    // Várakozás a soros port megnyitására hibakereséshez
    Utils::debugWaitForSerial(tft);
#endif

    // LittleFS filesystem indítása
    LittleFS.begin();

    // Config
    StoreEepromBase<Config_t>::init(); // Meghívjuk a statikus init metódust

    // Kell törölnia a konfigurációt?
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
        delay(3000); // Ha van touch, akkor várunk egy picit, hogy a TFT stabilizálódjon
        if (tft.getTouch(&x, &y)) {
            // Ha még mindig nyomva van..
            DEBUG("Restoring default settings...\n");
            Utils::beepTick();

            // akkor betöltjük a default konfigot
            config.loadDefaults();

            // és el is mentjük
            DEBUG("Save default settings...\n");
            config.checkSave();

            Utils::beepTick();
            DEBUG("Default settings restored!\n");
        }
    } else {
        // Konfig sima betöltése
        config.load();
    }

    // A konfiguráció betöltődött
    configLoaded = true;

    // TFT háttérvilágítás beállítása
    tftBackLightAdjuster.begin();

    // TFT érintőképernyő kalibrálása
    // Kell kalibrálni a TFT Touch-t?
    if (Utils::isZeroArray(config.data.tftCalibrateData)) {
        Utils::beepError();
        Utils::tftTouchCalibrate(tft, config.data.tftCalibrateData);
        config.checkSave(); // el is mentjük a kalibrációs adatokat
    }
    // Beállítjuk a touch scren-t
    tft.setTouch(config.data.tftCalibrateData);

    DEBUG("Core-0: setup(): System clock: %u MHz\n", (unsigned)clock_get_hz(clk_sys) / 1000000u);
}

/**
 * @brief Arduino loop() függvény
 */
void loop() {

    // TFT háttérvilágítás állítgatása a környezeti fényviszonyoknak megfelelően
    tftBackLightAdjuster.loop();

#define SENSOR_DISPLAY_INTERVAL_MS (30 * 1000UL)
    static unsigned long lastDisplayTime = 0;
    Utils::timeHasPassed(lastDisplayTime, SENSOR_DISPLAY_INTERVAL_MS, []() {
        DEBUG("vBus=%.2f V, vSys=%.2f V, coreT=%.2f °C, extT=%.2f °C\n", c1_sharedSensorData.vBus, c1_sharedSensorData.vSys, c1_sharedSensorData.coreTemperature, c1_sharedSensorData.externalTemperature);
    });

#define GPS_DISPLAY_INTERVAL_MS (10 * 1000UL)
    static unsigned long lastGpsDisplayTime = 0;
    Utils::timeHasPassed(lastGpsDisplayTime, GPS_DISPLAY_INTERVAL_MS, []() {
        DEBUG("GPS: valid=%d lat=%.6f lng=%.6f alt=%.1fm spd=%.1fkm/h crs=%.1f° sats=%d hdop=%.1f quality=%s mode=%s bootTime=%lus\n", (int)c1_sharedGpsData.locationValid, c1_sharedGpsData.lat, c1_sharedGpsData.lng,
              c1_sharedGpsData.altitudeM, c1_sharedGpsData.speedKmph, c1_sharedGpsData.courseDeg, (int)c1_sharedGpsData.satelliteCount, c1_sharedGpsData.hdop,
              GpsManager::qualityToString(c1_sharedGpsData.fixQuality).c_str(), GpsManager::modeToString(c1_sharedGpsData.fixMode).c_str(), (unsigned long)c1_sharedGpsData.gpsBootTime);
        if (c1_sharedGpsData.timeValid && c1_sharedGpsData.dateValid) {
            DEBUG("GPS time: %04d-%02d-%02d %02d:%02d:%02d (local)\n", (int)c1_sharedGpsData.year, (int)c1_sharedGpsData.month, (int)c1_sharedGpsData.day, (int)c1_sharedGpsData.hour, (int)c1_sharedGpsData.minute,
                  (int)c1_sharedGpsData.second);
        }
    });
}
