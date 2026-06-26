#pragma once

#include <Arduino.h>

/**
 * @brief Nyári időszámítás kezelő osztály
 * @details Ez az osztály a nyári időszámítás (DST) kezelésére szolgál.
 * A nyári időszámítás a CET/CEST időzónákban a március utolsó vasárnapjától október utolsó vasárnapjáig tart.
 * A nyári időszámítás során az órát egy órával előre állítják, így a CET (Central European Time) időzóna CEST (Central European Summer Time) időzónává válik.
 * A nyári időszámítás kezdete: március utolsó vasárnap 01:00 UTC
 * A nyári időszámítás vége: október utolsó vasárnap 01:00 UTC
 */
class DaylightSaving {

  private:
    /**
     * @brief Kiszámítja a hét napját a Zeller-féle kongruencia alapján
     * @param d Nap (1-31)
     * @param m Hónap (1-12)
     * @param y Év (pl. 2024)
     * @return A hét napja (0 = vasárnap, 1 = hétfő, ..., 6 = szombat)
     */
    static uint8_t dayOfWeek(uint8_t d, uint8_t m, uint8_t y) {

        static uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        y -= m < 3;
        // 0 Sunday, 1 Monday, etc...
        return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d + 6) % 7;
    }

    /**
     * @brief Ellenőrzi, hogy nyári időszámítás van-e érvényben
     * @param hours Óra (0-23)
     * @param day Nap (1-31)
     * @param month Hónap (1-12)
     * @param year Év (pl. 2024)
     * @return true, ha nyári időszámítás van érvényben, különben false
     */
    static boolean inSummerTime(uint8_t hours, uint8_t day, uint8_t month, uint8_t year) {

        if (day < 0 || month < 0 || year < 0)
            return false;

        if ((month >= 3) && (month <= 10)) {   // Március-tól Októberig inclusive
            if ((month > 3) && (month < 10)) { // Ha március és október között vagyunk, akkor biztosan nyári időszámítás van érvényben
                return true;
            }
            if (month == 3) {    // Március utolsó vasárnapja, 1 óra UT ?
                if (day >= 25) { // Érdekes lesz
                    uint8_t dw = dayOfWeek(day, month, year);
                    // Mikor van a következő vasárnap ?
                    uint8_t dts = 6 - dw; // Napok száma a következő vasárnapig
                    if (dts == 0)
                        dts = 7;                  // Valójában a következő vasárnapról beszélünk, nem a jelenlegi napról
                    if ((day + dts) > 31) {       // A következő vasárnap a következő hónapban van !
                        if (dw != 6 || hours > 0) // Végül ellenőrizzük, hogy nem a változás napján vagyunk-e a változás ideje előtt
                            return true;
                    }
                }
            }
            if (month == 10) {   // Október utolsó vasárnapja, 1 óra UT ?
                if (day >= 25) { // Érdekes lesz
                    uint8_t dw = dayOfWeek(day, month, year);
                    // Mikor van a következő vasárnap ?
                    uint8_t dts = 6 - dw; // Napok száma a következő vasárnapig
                    if (dts == 0)
                        dts = 7;                  // Valójában a következő vasárnapról beszélünk, nem a jelenlegi napról
                    if ((day + dts) > 31) {       // A következő vasárnap a következő hónapban van !
                        if (dw != 6 || hours > 0) // Végül ellenőrizzük, hogy nem a változás napján vagyunk-e a változás ideje előtt
                            return false;         // Már átléptük a változást
                        else
                            return true;
                    } else
                        return true;
                } else
                    return true;
            }
        }
        return false;
    }

  public:
    /**
     * @brief Korrigálja az időt a nyári időszámításnak megfelelően
     * @param mins Perc (0-59)
     * @param hours Óra (0-23)
     * @param day Nap (1-31)
     * @param month Hónap (1-12)
     * @param year Év (pl. 2024)
     *
     */
    static void correctTime(uint8_t &mins, uint8_t &hours, uint8_t &day, uint8_t &month, uint16_t &year) {
        int timeShift = 1; // Alapértelmezett CET (+1)
        if (inSummerTime(hours, day, month, year)) {
            timeShift += 1; // CEST (+2)
        }

        hours += timeShift;

        // Nap átlépés kezelése
        if (hours >= 24) {
            hours -= 24;
            day++;

            // Hónap végi átlépés kezelése (egyszerűsített)
            uint8_t daysInMonth = 31;
            if (month == 2) {
                // Szökőév ellenőrzése
                bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                daysInMonth = isLeap ? 29 : 28;
            } else if (month == 4 || month == 6 || month == 9 || month == 11) {
                daysInMonth = 30;
            }

            if (day > daysInMonth) {
                day = 1;
                month++;
                if (month > 12) {
                    month = 1;
                    year++;
                }
            }
        }
    }
};
