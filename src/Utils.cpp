#include "Utils.h"
#include "pins.h"

namespace Utils {

void debugWaitForSerial(TFT_eSPI &tft) {
#ifdef __DEBUG
    beepError();
    tft.setFreeFont();
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Nyisd meg a soros portot!", 0, 0);
    while (!Serial) {
    }
    tft.fillScreen(TFT_BLACK);
    beepTick();
#endif
}

/**
 * @brief TFT háttérvilágítás beállítása DC/PWM módban
 * 255-ös értéknél ténylegesen DC-t ad ki (digitalWrite HIGH),
 * más értékeknél PWM-et használ (analogWrite)
 *
 * @param brightness Fényerő érték (0-255)
 */
void setTftBacklight(uint8_t brightness) {
    if (brightness == 255) {
        // Maximum fényerőnél: tiszta DC (digitalWrite HIGH)
        digitalWrite(PIN_TFT_BACKGROUND_LED, HIGH);
    } else if (brightness == 0) {
        // Minimumnál: tiszta DC (digitalWrite LOW)
        digitalWrite(PIN_TFT_BACKGROUND_LED, LOW);
    } else {
        // Köztes értékeknél: PWM használata
        analogWrite(PIN_TFT_BACKGROUND_LED, brightness);
    }
}

/**
 *  Pitty hangjelzés
 */
void beepTick() {
    // Csak akkor csipogunk, ha a beeper engedélyezve van
    // if (!config.data.beeperEnabled) {
    //     return;
    // }
    tone(PIN_BUZZER, 800);
    delay(10);
    noTone(PIN_BUZZER);
}

/**
 * Hiba jelzés
 */
void beepError() {
    // Csak akkor csipogunk, ha a beeper engedélyezve van
    // if (!config.data.beeperEnabled) {
    //     return;
    // }
    tone(PIN_BUZZER, 500);
    delay(100);
    tone(PIN_BUZZER, 500);
    delay(100);
    tone(PIN_BUZZER, 500);
    delay(100);
    noTone(PIN_BUZZER);
}

} // namespace Utils
