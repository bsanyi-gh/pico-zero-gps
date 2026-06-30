#include "Utils.h"
#include "Config.h"
#include "defines.h"
#include "pins.h"

namespace Utils {

/**
 * @brief Váraqkozás a soros port megnyitására
 * Debug módban leblokkolja a programot, amíg a soros port készen nem áll
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
 *
 * @param tft A TFT kijelző példánya
 * @param calData A kalibrációs adatok tömbje (5 elem)
 *
 */
void tftTouchCalibrate(TFT_eSPI &tft, uint16_t (&calData)[5]) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(0);
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
 * @brief Pitty hangjelzés
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
 * @brief Hibajelzés
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
 * (Használhatnánk a CRC könyvtárat is, de itt saját implementációt adunk)
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

/**
 * @brief ISO-8859-2 ékezetes karakterek cseréje
 *
 * @param text A szöveg, amelyet módosítani kell
 */
void removeAccents(char *text) {

    int len = strlen(text);

    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        switch (c) {
            case 0xE1:
                text[i] = 'a';
                break; // á
            case 0xE9:
                text[i] = 'e';
                break; // é
            case 0xED:
                text[i] = 'i';
                break; // í
            case 0xF3:
                text[i] = 'o';
                break; // ó
            case 0xF6:
                text[i] = 'o';
                break; // ö
            case 0xF5:
                text[i] = 'o';
                break; // ő
            case 0xFA:
                text[i] = 'u';
                break; // ú
            case 0xFC:
                text[i] = 'u';
                break; // ü
            case 0xFB:
                text[i] = 'u';
                break; // ű
            case 0xC1:
                text[i] = 'A';
                break; // Á
            case 0xC9:
                text[i] = 'E';
                break; // É
            case 0xCD:
                text[i] = 'I';
                break; // Í
            case 0xD3:
                text[i] = 'O';
                break; // Ó
            case 0xD6:
                text[i] = 'O';
                break; // Ö
            case 0xD5:
                text[i] = 'O';
                break; // Ő
            case 0xDA:
                text[i] = 'U';
                break; // Ú
            case 0xDC:
                text[i] = 'U';
                break; // Ü
            case 0xDB:
                text[i] = 'U';
                break; // Ű
        }
    }
}

/**
 * @brief Egy lebegőpontos számot formáz char bufferbe, a tizedesjegyek számát paraméterként adva meg.
 *
 * @param value A lebegőpontos szám értéke
 * @param decimalPlaces A tizedesjegyek száma
 * @param buffer A kimeneti buffer (min. 16 karakter ajánlott)
 * @param bufferSize A buffer mérete
 * @return A buffer pointere a kényelmesebb használatért
 */
char *floatToString(float value, int decimalPlaces, char *buffer, size_t bufferSize) {
    dtostrf(value, 0, decimalPlaces, buffer);
    return buffer;
}

/**
 * @brief Átalakít egy másodperc értéket "perc:mp" formátumú szöveggé
 *
 * @param sec Időérték másodpercben
 * @param buffer A kimeneti buffer (min. 8 karakter)
 * @param bufferSize A buffer mérete
 * @return A buffer pointere a kényelmesebb használatért
 */
char *secToMinSecString(uint32_t sec, char *buffer, size_t bufferSize) {
    uint32_t minutes = sec / 60;
    uint32_t seconds = sec % 60;
    snprintf(buffer, bufferSize, "%02u:%02u", minutes, seconds);
    return buffer;
}

} // namespace Utils
