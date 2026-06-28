#include <Arduino.h>
#include <TFT_eSPI.h>

#include "Config.h"
#include "GpsManager.h"
#include "ScreenManager.h"
#include "SensorUtils.h"
#include "TftBackLightAdjuster.h"
#include "TraffipaxManager.h"
#include "Utils.h"
#include "defines.h"
#include "pins.h"

//------------------ Config
volatile bool configLoaded = false; // Jelezzük, hogy a config betöltődött

//------------------ TFT
TFT_eSPI tft;

// TFT Háttérvilágítás állítgatása a környezeti fényviszonyoknak megfelelően
TftBackLightAdjuster tftBackLightAdjuster;

// TrafipaxManager példány, automatikusan betölti a CSV-t
TraffipaxManager traffipaxManager;

ScreenManager *screenManager = nullptr;
IScreenManager **iScreenManager = (IScreenManager **)&screenManager; // A UIComponent használja

//-------------------------------------------------------------------------

/**
 * @brief Splash screen kirajzolása
 */
void drawSplashScreen() {

    // Gradient háttér - sötét kék → világos kék
    for (int y = 0; y < tft.height(); y++) {
        uint16_t color = tft.color565(0, 40 + (y * 60) / tft.height(), 80 + (y * 100) / tft.height());
        tft.drawFastHLine(0, y, tft.width(), color);
    }

    const int centerX = tft.width() / 2;
    const int titleY = tft.height() / 5;
    const int subtitleY = titleY + 50;
    const int infoY = tft.height() - 80;
    const int countY = tft.height() - 18;

    tft.setTextDatum(MC_DATUM);

    constexpr uint8_t titleSize = 5;

    // A splash saját maga állítja be a használt fontot és méretet,
    // hogy ne örököljön semmit az előtte futó debug képernyőről.
    tft.setFreeFont();
    tft.setTextSize(titleSize);

    // Doboz mérete konstansok - könnyű hangolás
    constexpr uint8_t TITLE_BOX_WIDTH_PADDING = 10; // Oldalsó padding - csökkentsd a keskenyebb dobozért
    constexpr uint8_t TITLE_BOX_HEIGHT = 60;        // Doboz magassága - növeld a magasabbért
    constexpr uint8_t TITLE_BOX_Y_OFFSET = 30;      // Y pozíció offset a centerből

    // Nagy cím árnyékkal és háttér sávval
    int titleWidth = tft.textWidth(PROGRAM_NAME);
    int titleBoxW = titleWidth + TITLE_BOX_WIDTH_PADDING;
    int titleBoxH = TITLE_BOX_HEIGHT;

    // Árnyék effekt (sötét szürke eltolás)
    tft.fillRoundRect(centerX - titleBoxW / 2 + 3, titleY - TITLE_BOX_Y_OFFSET + 3, titleBoxW, titleBoxH, 8, tft.color565(30, 30, 30));

    // Fő doboz - sötét kék / fekete gradiens
    tft.fillRoundRect(centerX - titleBoxW / 2, titleY - TITLE_BOX_Y_OFFSET, titleBoxW, titleBoxH, 8, tft.color565(0, 60, 120));
    tft.drawRoundRect(centerX - titleBoxW / 2, titleY - TITLE_BOX_Y_OFFSET, titleBoxW, titleBoxH, 8, TFT_CYAN);
    tft.drawRoundRect(centerX - titleBoxW / 2 + 1, titleY - TITLE_BOX_Y_OFFSET + 1, titleBoxW - 2, titleBoxH - 2, 8, tft.color565(100, 200, 255));

    // Szöveg - fehér + árnyék
    tft.setTextColor(tft.color565(30, 30, 30)); // Sötét árnyék
    tft.drawString(PROGRAM_NAME, centerX + 1, titleY + 1);

    tft.setTextColor(TFT_WHITE); // Fehér szöveg
    tft.drawString(PROGRAM_NAME, centerX, titleY);

    // Alcím és leírás - sárga
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString(PROGRAM_DESC, centerX, subtitleY);

    // Verzió és author
    tft.setTextDatum(BC_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(200, 200, 255)); // Világos kék
    tft.drawString(PROGRAM_VERSION, centerX, infoY);
    tft.setTextColor(TFT_CYAN);
    tft.drawString(PROGRAM_AUTHOR, centerX, infoY + 18);

    // Build időpont - halvány szürke
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%s %s", __DATE__, __TIME__);
    tft.setTextColor(tft.color565(150, 150, 150));
    tft.drawString(buffer, centerX, countY - 14);

    // Traffipax számláló - nagy és világos
    snprintf(buffer, sizeof(buffer), "Traffipax count: %d", traffipaxManager.count());
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(tft.color565(255, 200, 0)); // Arany sárga
    tft.drawString(buffer, centerX, countY);
}

/**
 * @brief Arduino Core0 setup() függvény
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

    // Trafipax adatok betöltése CSV-ből ha a LittleFS fájl létezik
    if (traffipaxManager.checkFile(TraffipaxManager::CSV_FILE_NAME)) {
        traffipaxManager.loadFromCSV(TraffipaxManager::CSV_FILE_NAME);
    }
    DEBUG("Ismert traffipaxok száma: %d\n", traffipaxManager.count());

    // Splash screen
    drawSplashScreen();

    // Még egy picit mutatjuk a splash screent
    delay(1000);

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

    // Még egy picit mutatjuk a splash screent
    delay(3000);

    // ScreenManager inicializálása itt, amikor minden más már kész
    if (screenManager == nullptr) {
        screenManager = new ScreenManager();
    }
    screenManager->switchToScreen(SCREEN_NAME_MAIN); // A kezdő képernyőre kapcsolás

    // Pittyentünk egyet, hogy üzemkészek vagyunk
    Utils::beepTick();

    DEBUG("Core-0: setup(): System clock: %u MHz\n", (unsigned)clock_get_hz(clk_sys) / 1000000u);
}

/**
 * @brief Arduino Core0 loop() függvény
 */
void loop() {

    //------------------- Touch esemény kezelése
    uint16_t touchX, touchY;
    bool touchedRaw = tft.getTouch(&touchX, &touchY);
    bool validCoordinates = true;
    if (touchedRaw) {
        if (touchX > tft.width() || touchY > tft.height()) {
            validCoordinates = false;
        }
    }

    static bool lastTouchState = false;
    static uint16_t lastTouchX = 0, lastTouchY = 0;
    bool touched = touchedRaw && validCoordinates;

    // Touch press event (immediate response)
    if (touched && !lastTouchState) {
        TouchEvent touchEvent(touchX, touchY, true);
        screenManager->handleTouch(touchEvent);
        lastTouchX = touchX;
        lastTouchY = touchY;
    } else if (!touched && lastTouchState) { // Touch release event (immediate response)
        TouchEvent touchEvent(lastTouchX, lastTouchY, false);
        screenManager->handleTouch(touchEvent);
    }

    lastTouchState = touched;

    if (screenManager) {
        // Deferred actions feldolgozása - biztonságos képernyőváltások végrehajtása
        screenManager->loop();
    }

//------------------- EEPROM mentés figyelése
#define EEPROM_SAVE_CHECK_INTERVAL 1000 * 60 * 5 // 5 perc
    static uint32_t lastEepromSaveCheck = 0;
    if (Utils::timeHasPassed(lastEepromSaveCheck, EEPROM_SAVE_CHECK_INTERVAL)) {
        config.checkSave();
        lastEepromSaveCheck = millis();
    }

    // Háttérvilágitás vezérlése - csak ha nem screensaver aktív
    if (screenManager && !screenManager->isCurrentScreenScreensaver()) {
        tftBackLightAdjuster.loop();
    }

    //---------------------------------------------------------------------------------------------------

#define SENSOR_DISPLAY_INTERVAL_MS (30 * 1000UL)
    static unsigned long lastDisplayTime = 0;
    Utils::timeHasPassed(lastDisplayTime, SENSOR_DISPLAY_INTERVAL_MS, []() {
        DEBUG("vBus=%.2f V, vSys=%.2f V, coreT=%.2f °C, extT=%.2f °C\n", c1_sharedSensorData.vBus, c1_sharedSensorData.vSys, c1_sharedSensorData.coreTemperature, c1_sharedSensorData.externalTemperature);
    });

#define GPS_DISPLAY_INTERVAL_MS (10 * 1000UL)
    static unsigned long lastGpsDisplayTime = 0;
    Utils::timeHasPassed(lastGpsDisplayTime, GPS_DISPLAY_INTERVAL_MS, []() {
        // GPS adatok kiírása
        DEBUG("GPS: satCnt=%d, valid=%d lat=%.6f lng=%.6f alt=%.1fm spd=%.1fkm/h crs=%.1f° sats=%d hdop=%.1f quality=%s mode=%s bootTime=%lus\n",
              c1_sharedGpsData.satelliteCount,                                  //
              (int)c1_sharedGpsData.locationValid,                              //
              c1_sharedGpsData.lat,                                             //
              c1_sharedGpsData.lng,                                             //
              c1_sharedGpsData.altitudeM,                                       //
              c1_sharedGpsData.speedKmph,                                       //
              c1_sharedGpsData.courseDeg,                                       //
              (int)c1_sharedGpsData.satelliteCount,                             //
              c1_sharedGpsData.hdop,                                            //
              GpsManager::qualityToString(c1_sharedGpsData.fixQuality).c_str(), //
              GpsManager::modeToString(c1_sharedGpsData.fixMode).c_str(),       //
              (unsigned long)c1_sharedGpsData.gpsBootTime                       //
        );                                                                      //

        // Ha van érvényes GPS idő és dátum, akkor kiírjuk
        if (c1_sharedGpsData.timeValid && c1_sharedGpsData.dateValid) {
            DEBUG("GPS time: %04d-%02d-%02d %02d:%02d:%02d (local)\n",
                  (int)c1_sharedGpsData.year,   //
                  (int)c1_sharedGpsData.month,  //
                  (int)c1_sharedGpsData.day,    //
                  (int)c1_sharedGpsData.hour,   //
                  (int)c1_sharedGpsData.minute, //
                  (int)c1_sharedGpsData.second  //
            );
        }
    });
}
