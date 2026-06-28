#pragma once

#include <Arduino.h>
#include <TinyGPS++.h>

#include "Config.h"
#include "DayLightSaving.h"
#include "SatelliteDb.h"

/**
 * @class GpsManager
 * @brief A GPS modul kezeléséért felelős osztály
 * @details A GpsManager osztály a GPS modul kezeléséért felelős.
 *          A TinyGPS++ könyvtárat használja a GPS adatok feldolgozására, és a műholdak adatait a SatelliteDb osztályban tárolja.
 *          A GpsManager osztály a GPS modul soros kommunikációját kezeli, feldolgozza a beérkező NMEA üzeneteket, és frissíti a műhold adatbázist.
 *          A GpsManager osztály lehetővé tesz a GPS adatok lekérdezését, mint például a helyzet, sebesség, dátum és idő, valamint a műholdak számát és minőségét.
 *          A GpsManager osztály a Config osztály segítségével képes reagálni a konfigurációs változásokra, például a debug mód bekapcsolására vagy kikapcsolására.
 */
class GpsManager {

  public:
    struct Satellites_type {
        int prn;
        int elevation;
        int azimuth;
        int snr;
        bool updated; // Flag ami jelzi, hogy a műhold adatai frissültek-e az aktuális GSV blokkban
    };

    SatelliteDb satelliteDb;

    /**
     * Konstruktor
     */
    GpsManager(HardwareSerial &serial);

    /**
     * Destruktor
     */
    ~GpsManager();

    /**
     * loop
     */
    void loop();

    /**
     * @brief Callback függvény, amit a Config hív meg változás esetén
     */
    void onConfigChanged();

    /**
     * Thread-safe hozzáférés a műhold adatbázishoz UI számára (Core0)
     */
    std::vector<SatelliteDb::SatelliteData> getSatelliteSnapshotForUI(SatelliteDb::SortType_t sortType = SatelliteDb::NONE) const { return satelliteDb.getSnapshotForUI(sortType); }

    /** GPS minőségi szint szövege (a sharedGpsData.fixQuality alapján) */
    static String qualityToString(uint8_t fixQuality);

    /** GPS üzemmód szövege (a sharedGpsData.fixMode alapján) */
    static String modeToString(uint8_t fixMode);

  private:
    HardwareSerial &gpsSerial;
    TinyGPSPlus gps;

    uint8_t currentSatelliteCount = 0;  // Number of satellites currently being tracked
    TinyGPSCustom gsv_msg_num;          // gsv_msg_num
    TinyGPSCustom gsv_total_msgs;       // gsv_total_msgs
    TinyGPSCustom gsv_num_sats_in_view; // gsv_num_sats_in_view

    TinyGPSCustom gsv_prn[4];       // gsv_prn
    TinyGPSCustom gsv_elevation[4]; // gsv_elevation
    TinyGPSCustom gsv_azimuth[4];   // gsv_azimuth
    TinyGPSCustom gsv_snr[4];       // gsv_snr

    // Debugging GPS adatok kiírása
    bool debugGpsSerialData;

    // Debugging a beépített RGB LED-el
    bool debugGpsSerialOnInternalFastLed;

    bool debugGpsSatellitesDatabase;

    // Boot time - a GPS mikor látott érvényes műholdat?
    uint32_t bootStartTime;
    uint32_t gpsBootTime;

    // Config callback token az automatikus leiratkozáshoz
    size_t configCallbackId;

    void processGSVMessages();
    void updateSharedData(); // sharedGpsData frissítése (Core 1 hívja)
};

// Megosztott GPS adatok - Core 1 ír, Core 0 olvas (volatile)
struct GpsData {
    // Pozíció
    volatile bool locationValid; // TinyGPSLocation::isValid() && TinyGPSLocation::age() < GPS_DATA_MAX_AGE
    volatile double lat;         // WGS84 fokokban
    volatile double lng;         // WGS84 fokokban
    volatile uint8_t fixQuality; // TinyGPSLocation::FixQuality_t
    volatile uint8_t fixMode;    // TinyGPSLocation::FixMode_t

    // Sebesség
    volatile bool speedValid; // TinyGPSSpeed::isValid()
    volatile float speedKmph; // km/h
    volatile float courseDeg; // irány (fok)

    // Magasság
    volatile bool altitudeValid; // TinyGPSAltitude::isValid()
    volatile float altitudeM;    // m

    // Műholdak és jel
    volatile uint8_t satelliteCount;      // TinyGPSSatellites::value()
    volatile uint8_t satelliteCountForUI; // UI számára, hogy ne ugráljon a szám
    volatile float hdop;

    // Helyi dátum és idő (CET/CEST korrigált)
    volatile bool timeValid; // TinyGPSTime::isValid() && TinyGPSDate::isValid()
    volatile uint8_t hour;   // 0-23
    volatile uint8_t minute; // 0-59
    volatile uint8_t second; // 0-59

    volatile bool dateValid; // TinyGPSDate::isValid()
    volatile uint16_t year;  // 4-digit year
    volatile uint8_t month;  // 1-12
    volatile uint8_t day;    // 1-31

    // Az első valid fix-ig eltelt indulási idő (másodperc)
    volatile uint32_t gpsBootTime; // A GPS mikor látott érvényes műholdat (másodperc)
};

// main-c1-ben definiálva van a változó, hogy Core1 írja, Core0 olvassa
extern GpsData c1_sharedGpsData;