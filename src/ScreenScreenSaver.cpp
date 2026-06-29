#include "ScreenScreenSaver.h"

#include "TftBackLightAdjuster.h"
extern TftBackLightAdjuster tftBackLightAdjuster;

#include "GpsManager.h"

/**
 * @brief Konstruktor
 */
ScreenScreenSaver::ScreenScreenSaver() : UIScreen(SCREEN_NAME_SCREENSAVER) {
    DEBUG("ScreenScreenSaver: Constructor called\n");

    // A tényleges, aktuális fontbeállítással mérjük a szöveg méretét.

    tft.setTextFont(SCREENSAVER_TFT_FONT);
    tft.setTextSize(1);
    this->textW = tft.textWidth("88:88", SCREENSAVER_TFT_FONT);
    this->textH = tft.fontHeight(SCREENSAVER_TFT_FONT);

    layoutComponents();
}

/**
 * @brief Destruktor
 */
ScreenScreenSaver::~ScreenScreenSaver() = default;

/**
 * @brief Képernyő komponensek elrendezése
 */
void ScreenScreenSaver::layoutComponents() {}

/**
 * @brief Képernyő aktiválása
 *
 * Meghívódik amikor a képernyő aktívvá válik (pl. visszatérés Info/Setup képernyőről)
 */
void ScreenScreenSaver::activate() {

    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont();

    // Kényszeríti a háttérvilágítást a minimum szintre
    tftBackLightAdjuster.setBacklightLevel(NIGHTLY_BRIGHTNESS);
}

/**
 * @brief Érintés esemény kezelése
 * @param event Érintés esemény adatok
 * @return true ha kezelte az eseményt (mindig), false egyébként
 */
bool ScreenScreenSaver::handleTouch(const TouchEvent &event) {
    if (event.pressed) {
        if (getScreenManager()) {
            getScreenManager()->goBack();
            Utils::beepTick();
        }
        return true;
    }
    return false;
}

/**
 * @brief Képernyő saját tartalmának kirajzolása
 */
void ScreenScreenSaver::drawContent() {}

/**
 * @brief Loop hívás felülírása
 */
void ScreenScreenSaver::handleOwnLoop() {

    // 15 másodpercenként frissítünk
    static uint32_t lastUpdate = 0;
    if (!Utils::timeHasPassed(lastUpdate, 15000)) {
        return;
    }
    lastUpdate = millis();

    // Idő
    char timeStr[9] = "--:--";
    if (c1_sharedGpsData.timeValid) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", c1_sharedGpsData.hour, c1_sharedGpsData.minute);
    }

    constexpr int32_t MARGIN = 20;

    // Véletlenszerű pozíció generálása a képernyőn belül (a teljes szöveg maradjon látható).
    int32_t maxX = static_cast<int32_t>(::tft.width()) - this->textW - MARGIN;
    int32_t maxY = static_cast<int32_t>(::tft.height()) - this->textH - MARGIN;
    if (maxX < MARGIN) {
        maxX = MARGIN;
    }
    if (maxY < MARGIN) {
        maxY = MARGIN;
    }

    this->lastX = random(MARGIN, maxX + 1);
    this->lastY = random(MARGIN, maxY + 1);

    // Kirajzolás
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(timeStr, this->lastX, this->lastY, SCREENSAVER_TFT_FONT);
}
