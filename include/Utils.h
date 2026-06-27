#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <functional>

//--- Utils ---
namespace Utils {

/**
 * Eltelt már annyi idő?
 */
inline bool timeHasPassed(long fromWhen, int howLong) { return millis() - fromWhen >= howLong; }

/**
 * Eltelt már annyi idő? Ha igen, meghívja a callbacket és reseteli az időt.
 */
inline void timeHasPassed(unsigned long &lastTime, unsigned long interval, std::function<void()> callback) {
    if (millis() - lastTime >= interval) {
        lastTime = millis();
        callback();
    }
}

/**
 * @brief TFT háttérvilágítás beállítása DC/PWM módban
 * 255-ös értéknél ténylegesen DC-t ad ki (digitalWrite HIGH),
 * más értékeknél PWM-et használ (analogWrite)
 *
 * @param brightness Fényerő érték (0-255)
 */
void setTftBacklight(uint8_t brightness);

/**
 * Várakozás a soros port megnyitására
 * Leblokkolja a programot, amíg a soros port készen nem áll
 *
 * @param pTft a TFT kijelző példánya
 */
void debugWaitForSerial(TFT_eSPI &tft);

/**
 * @brief TFT érintőképernyő kalibrálása
 * @param tft A TFT kijelző példánya
 */
void tftTouchCalibrate(TFT_eSPI &tft, uint16_t (&calData)[5]);

//--- Beep ----
/**
 *  Pitty hangjelzés
 */
void beepTick();
void beepError();

/**
 * Tömb elemei nullák?
 */
template <typename T, size_t N> bool isZeroArray(T (&arr)[N]) {
    for (size_t i = 0; i < N; ++i) {
        if (arr[i] != 0) {
            return false; // Ha bármelyik elem nem nulla, akkor false-t adunk vissza
        }
    }
    return true; // Ha minden elem nulla, akkor true-t adunk vissza
}

/**
 * @brief CRC16 számítás (CCITT algoritmus)
 * Használhatnánk a CRC könyvtárat is, de itt saját implementációt adunk
 *
 * @param data Adat pointer
 * @param length Adat hossza bájtokban
 * @return Számított CRC16 érték
 */
uint16_t calcCRC16(const uint8_t *data, size_t length);

/**
 * @brief Ékezetes karakterek ASCII karakterekre konvertálása
 * @param text A konvertálandó szöveg (in-place módosítás)
 */
void removeAccents(char *text);

/**
 * @brief Egy lebegőpontos számot formáz char bufferbe, a tizedesjegyek számát paraméterként adva meg.
 * @param value A lebegőpontos szám értéke
 * @param decimalPlaces A tizedesjegyek száma
 * @param buffer A kimeneti buffer (min. 16 karakter ajánlott)
 * @param bufferSize A buffer mérete
 * @return A buffer pointere a kényelmesebb használatért
 */
char *floatToString(float value, int decimalPlaces, char *buffer, size_t bufferSize);

} // namespace Utils