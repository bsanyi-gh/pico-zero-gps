#include "Utils.h"
#include "Config.h"
#include "defines.h"
#include "pins.h"

namespace Utils {

/**
 * @brief TFT háttérvilágítás beállítása DC/PWM módban
 * 255-ös értéknél ténylegesen DC-t ad ki (digitalWrite HIGH),
 * más értékeknél PWM-et használ (analogWrite)
 * @param brightness Fényerő érték (0-255)
 */
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
 * @brief TFT érintőképernyő kalibrálása
 * @param tft A TFT kijelző példánya
 * @param calData A kalibrációs adatok tömbje (5 elem)
 *
 */
void tftTouchCalibrate(TFT_eSPI &tft, uint16_t (&calData)[5]) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextSize(2);
    const char *txt = "TFT touch kalibracio kell!\n";
    tft.setCursor((tft.width() - tft.textWidth(txt)) / 2, tft.height() / 2 - 60);
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.println(txt);

    tft.setTextSize(1);
    txt = "Erintsd meg a jelzett helyeken a sarkokat!\n";
    tft.setCursor((tft.width() - tft.textWidth(txt)) / 2, tft.height() / 2 + 20);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println(txt);

    // TFT_eSPI 'bóti' kalibráció indítása
    tft.calibrateTouch(calData, TFT_YELLOW, TFT_BLACK, 15);

    txt = "Kalibracio befejezodott!";
    tft.fillScreen(TFT_BLACK);
    tft.setCursor((tft.width() - tft.textWidth(txt)) / 2, tft.height() / 2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(1);
    tft.println(txt);

    DEBUG("// Használd ezt a kalibrációs kódot a setup()-ban:\n");
    DEBUG("  uint16_t calData[5] = { ");
    for (uint8_t i = 0; i < 5; i++) {
        DEBUG("%d", calData[i]);
        if (i < 4) {
            DEBUG(", ");
        }
    }
    DEBUG(" };\n");
    DEBUG("  tft.setTouch(calData);\n");
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
    if (!config.data.beeperEnabled) {
        return;
    }
    tone(PIN_BUZZER, 800);
    delay(10);
    noTone(PIN_BUZZER);
}

/**
 * Hiba jelzés
 */
void beepError() {
    // Csak akkor csipogunk, ha a beeper engedélyezve van
    if (!config.data.beeperEnabled) {
        return;
    }
    tone(PIN_BUZZER, 500);
    delay(100);
    tone(PIN_BUZZER, 500);
    delay(100);
    tone(PIN_BUZZER, 500);
    delay(100);
    noTone(PIN_BUZZER);
}

/**
 * @brief CRC16 számítás (CCITT algoritmus)
 * Használhatnánk a CRC könyvtárat is, de itt saját implementációt adunk
 *
 * @param data Adat pointer
 * @param length Adat hossza bájtokban
 * @return Számított CRC16 érték
 */
uint16_t calcCRC16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1);
        }
    }
    return crc;
}

} // namespace Utils
