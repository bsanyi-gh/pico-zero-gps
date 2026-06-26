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
    volatile bool locationValid;
    volatile double lat;
    volatile double lng;
    volatile uint8_t fixQuality; // TinyGPSLocation::FixQuality_t
    volatile uint8_t fixMode;    // TinyGPSLocation::FixMode_t

    // Mozgás
    volatile float speedKmph;
    volatile float courseDeg;
    volatile float altitudeM;
    volatile bool altitudeValid;

    // Műholdak és jel
    volatile uint8_t satelliteCount;
    volatile float hdop;

    // Helyi dátum és idő (CET/CEST korrigált)
    volatile uint8_t hour;
    volatile uint8_t minute;
    volatile uint8_t second;
    volatile bool timeValid;
    volatile uint8_t day;
    volatile uint8_t month;
    volatile uint16_t year;
    volatile bool dateValid;

    // Az első valid fix-ig eltelt indulási idő (másodperc)
    volatile uint32_t gpsBootTime;
};

// main-c1-ben definiálva van a változó, hogy Core1 írja, Core0 olvassa
extern GpsData c1_sharedGpsData;