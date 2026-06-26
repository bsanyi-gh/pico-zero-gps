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

//--- Beep ----
/**
 *  Pitty hangjelzés
 */
void beepTick();
void beepError();

} // namespace Utils