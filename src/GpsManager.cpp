#include "GpsManager.h"
#include "Config.h"
#include "Utils.h"
#include "defines.h"
#include "pins.h"

// ============================================================================
// DEBUG MAKRÓK
// ============================================================================
#define __GPSMANAGER_DEBUG
#if defined(__DEBUG) && defined(__GPSMANAGER_DEBUG)
#define GPSMANAGER_DEBUG(fmt, ...) DEBUG("GpsManager::" fmt __VA_OPT__(, ) __VA_ARGS__)
#else
#define GPSMANAGER_DEBUG(fmt, ...) // Üres makró, ha __DEBUG nincs definiálva
#endif

#define INTERNAL_RGB_LED_BRIGHTNESS 10
#define INTERNAL_RGB_LED_NUM 1

// PICO ZERO WS2812 RGB LED driver
#define FASTLED_INTERNAL
#include <FastLED.h>
#define FASTLED_FORCE_SOFTWARE_SPI
#define FASTLED_FORCE_SOFTWARE_PINS
CRGB leds[INTERNAL_RGB_LED_NUM];
#define INTERNAL_LED_COLOR CRGB::Red // Pirosan villogjon, ha GPS adat érkezik

constexpr uint8_t MAX_SATELLITES = 50;

/**
 * @brief Konstruktor
 * @param serial A GPS soros port referenciája
 */
GpsManager::GpsManager(HardwareSerial &serial) : gpsSerial(serial) {
    // Feliratkozás a config változásokra
    configCallbackId = config.registerChangeCallback([this]() { this->onConfigChanged(); });

    // Kezdeti értékek felvétele
    onConfigChanged();

    // inicializáljuk a FastLED-et: Pico Zero WS2812 RGB LED
    FastLED.addLeds<WS2812, INTERNAL_RGB_LED_PIN, GRB>(leds, INTERNAL_RGB_LED_NUM);
    FastLED.setBrightness(INTERNAL_RGB_LED_BRIGHTNESS); // Fényerő beállítása
    FastLED.clear();
    FastLED.show();

    // TinyGPSCustom objektumok inicializálása GSV feldolgozáshoz - fő GSV mezők
    gsv_msg_num.begin(gps, "GPGSV", 1);          // Üzenet sorszáma
    gsv_total_msgs.begin(gps, "GPGSV", 2);       // Üzenetek száma
    gsv_num_sats_in_view.begin(gps, "GPGSV", 3); // Látható műholdak száma

    // TinyGPSCustom objektumok inicializálása GSV feldolgozáshoz
    //    Minden GSV mondat legfeljebb 4 műhold adatait tartalmazza
    //    A mezők: PRN, Eleváció, Azimut, SNR
    //    A mező indexek 0-tól indulnak, tehát $GPGSV,1,2,3,4,5,6,7,...
    //    4. mező: PRN, 5. mező: Eleváció, 6. mező: Azimut, 7. mező: SNR

    // Inicializáljuk az összes TinyGPSCustom objektumot
    for (byte i = 0; i < 4; ++i) {
        gsv_prn[i].begin(gps, "GPGSV", 4 + 4 * i);       // offsets 4, 8, 12, 16
        gsv_elevation[i].begin(gps, "GPGSV", 5 + 4 * i); // offsets 5, 9, 13, 17
        gsv_azimuth[i].begin(gps, "GPGSV", 6 + 4 * i);   // offsets 6, 10, 14, 18
        gsv_snr[i].begin(gps, "GPGSV", 7 + 4 * i);       // offsets 7, 11, 15, 19
    }

    // Ekkor indultunk
    bootStartTime = millis();
    gpsBootTime = 0;
}

/**
 * Destruktor
 */
GpsManager::~GpsManager() { config.unregisterCallback(configCallbackId); }

/**
 * @brief Callback függvény, amit a Config hív meg változás esetén
 */
void GpsManager::onConfigChanged() {
    GPSMANAGER_DEBUG("onConfigChanged() - Debug flag-ek frissítése.\n");
    debugGpsSerialOnInternalFastLed = config.data.debugGpsSerialOnInternalFastLed;
    debugGpsSerialData = config.data.debugGpsSerialData;
    debugGpsSatellitesDatabase = config.data.debugGpsSatellitesDatabase;
}

/**
 * @brief Frissíti a megosztott GPS adatokat (Core 1 hívja)
 */
void GpsManager::updateSharedData() {
    c1_sharedGpsData.locationValid = gps.location.isValid() && gps.location.age() < GPS_DATA_MAX_AGE;
    c1_sharedGpsData.lat = gps.location.lat();
    c1_sharedGpsData.lng = gps.location.lng();
    c1_sharedGpsData.fixQuality = (uint8_t)gps.location.FixQuality();
    c1_sharedGpsData.fixMode = (uint8_t)gps.location.FixMode();

    c1_sharedGpsData.speedKmph = gps.speed.kmph();
    c1_sharedGpsData.courseDeg = gps.course.deg();
    c1_sharedGpsData.altitudeM = gps.altitude.meters();
    c1_sharedGpsData.altitudeValid = gps.altitude.isValid();

    c1_sharedGpsData.satelliteCount = gps.satellites.isValid() ? (uint8_t)gps.satellites.value() : 0;
    c1_sharedGpsData.hdop = gps.hdop.isValid() ? (float)gps.hdop.hdop() : 99.9f;

    c1_sharedGpsData.timeValid = false;
    if (gps.time.isValid() && gps.time.age() < GPS_DATA_MAX_AGE && gps.time.hour() <= 23 && gps.time.minute() <= 59 && gps.time.second() <= 59) {
        c1_sharedGpsData.hour = gps.time.hour();
        c1_sharedGpsData.minute = gps.time.minute();
        c1_sharedGpsData.second = gps.time.second();
        c1_sharedGpsData.timeValid = true;
    }
    c1_sharedGpsData.dateValid = false;
    if (gps.date.isValid() && gps.date.age() < GPS_DATA_MAX_AGE && gps.date.year() > 2020 && gps.date.year() < 2100 && gps.date.month() >= 1 && gps.date.month() <= 12 && gps.date.day() >= 1 && gps.date.day() <= 31) {
        c1_sharedGpsData.day = gps.date.day();
        c1_sharedGpsData.month = gps.date.month();
        c1_sharedGpsData.year = gps.date.year();
        c1_sharedGpsData.dateValid = true;
    }
    if (c1_sharedGpsData.dateValid && c1_sharedGpsData.timeValid) {
        uint8_t h = c1_sharedGpsData.hour, m = c1_sharedGpsData.minute;
        uint8_t d = c1_sharedGpsData.day, mo = c1_sharedGpsData.month;
        uint16_t y = c1_sharedGpsData.year;
        DaylightSaving::correctTime(m, h, d, mo, y);
        c1_sharedGpsData.hour = h;
        c1_sharedGpsData.minute = m;
        c1_sharedGpsData.day = d;
        c1_sharedGpsData.month = mo;
        c1_sharedGpsData.year = y;
    }

    c1_sharedGpsData.gpsBootTime = gpsBootTime;
}

/**
 * @brief Fix quality értékének szöveges reprezentációja
 * @param fixQuality A fix quality értéke
 * @return A fix quality szöveges reprezentációja
 */
String GpsManager::qualityToString(uint8_t fixQuality) {
    switch (fixQuality) {
        case TinyGPSLocation::Invalid:
            return "Invalid";
        case TinyGPSLocation::GPS:
            return "GPS";
        case TinyGPSLocation::DGPS:
            return "DGPS";
        case TinyGPSLocation::PPS:
            return "PPS";
        case TinyGPSLocation::RTK:
            return "RTK";
        case TinyGPSLocation::FloatRTK:
            return "FloatRTK";
        case TinyGPSLocation::Estimated:
            return "Estimated";
        case TinyGPSLocation::Manual:
            return "Manual";
        case TinyGPSLocation::Simulated:
            return "Simulated";
        default:
            return "Unknown";
    }
}

/**
 * @brief Fix mode értékének szöveges reprezentációja
 * @param fixMode A fix mode értéke
 * @return A fix mode szöveges reprezentációja
 */
String GpsManager::modeToString(uint8_t fixMode) {
    switch (fixMode) {
        case TinyGPSLocation::N:
            return "No Fix";
        case TinyGPSLocation::A:
            return "Auto 2D/3D";
        case TinyGPSLocation::D:
            return "Differential";
        case TinyGPSLocation::E:
            return "Estimated";
        default:
            return "Unknown";
    }
}

/**
 * @brief Read GSV messages from the GPS module
 */
void GpsManager::processGSVMessages() {

    // Check if all GSV messages have been received
    if (!gsv_total_msgs.isValid()) {
        // GPSMANAGER_DEBUG("Not all $GPGSV messages processed yet\n");
        return;
    }

    // Check if this message is a new GSV block
    if (!gsv_msg_num.isUpdated()) {
        return;
    }

    uint8_t msg_num = atoi(gsv_msg_num.value());
    uint8_t total_msgs = atoi(gsv_total_msgs.value());
    uint8_t num_sats_in_view = atoi(gsv_num_sats_in_view.value());

    // Process up to 4 satellites per GSV message
    for (byte i = 0; i < 4; ++i) {
        if (gsv_prn[i].isUpdated() && gsv_prn[i].isValid()) {
            int _prn = atoi(gsv_prn[i].value());

            // ez valami szemét adat?
            if (_prn == 0) {
                continue;
            }

            int _elevation = atoi(gsv_elevation[i].value());
            int _azimuth = atoi(gsv_azimuth[i].value());
            int _snr = atoi(gsv_snr[i].value());

            satelliteDb.insertSatellite(_prn, _elevation, _azimuth, _snr);
        }
    }

    // totalMessages ==  currentMessage ?
    if (total_msgs != msg_num) {
        // GPSMANAGER_DEBUG("Not complete yet\n");
        return;
    }

    if (debugGpsSatellitesDatabase) {
        satelliteDb.debugSatDb(num_sats_in_view);
    }
}

/**
 * @brief GPS olvasása
 */
void GpsManager::loop() {

    bool isValidSentence = false;

    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        if (gps.encode(c)) {
            isValidSentence = true;
        }

        // Debug: kiírjuk a GPS soros porton küldött karaktereit
        if (debugGpsSerialData) {
            GPSMANAGER_DEBUG("%c", c);
        }
    }

    // Ha van érvényes GPS NMEA mondat
    if (isValidSentence) {

        // GPS mondatok feldolgozása
        processGSVMessages();

        // Megosztott adatok frissítése (Core 0 olvassa)
        updateSharedData();

        // GPS boot idő számítása (első érvényes műholdadat)
        if (gpsBootTime == 0 && gps.satellites.isValid() && gps.satellites.age() < GPS_DATA_MAX_AGE && gps.satellites.value() > 0) {
            gpsBootTime = (millis() - bootStartTime) / 1000;
        }

        // beépített RGB LED villogtatása, ha volt érvényes bejövő GPS mondat
        if (debugGpsSerialOnInternalFastLed) {
            leds[0] = INTERNAL_LED_COLOR;
            FastLED.show();

            leds[0] = CRGB::Black;
            FastLED.show();
        }
    }

    // Műhold adatbázis karbantartása másodpercenként
    static long lastWiseSatellitesData = millis();
    if (Utils::timeHasPassed(lastWiseSatellitesData, 1000)) {
        satelliteDb.deleteUntrackedSatellites();
    }
}