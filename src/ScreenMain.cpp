#include <Arduino.h>
#include <cmath>
#include <cstring>

#include "ScreenMain.h"
#include "defines.h"

namespace {

constexpr float HUD_MAX_SPEED_KMPH = 180.0f;
constexpr uint32_t HUD_UPDATE_INTERVAL_MS = 120;

// Sebesség widget pozíció és méret
constexpr int16_t TRACK_X = 8;
constexpr int16_t TRACK_Y = 6;
constexpr int16_t TRACK_W = 98;
constexpr int16_t TRACK_H = 40;

// Idő widget pozíció és méret
constexpr int16_t TIME_X = 111;
constexpr int16_t TIME_Y = 6;
constexpr int16_t TIME_W = 98;
constexpr int16_t TIME_H = 40;

// PREC widget pozíció és méret
constexpr int16_t PREC_X = 214;
constexpr int16_t PREC_Y = 6;
constexpr int16_t PREC_W = 98;
constexpr int16_t PREC_H = 40;

// Sebesség widget pozíció és méret
constexpr int16_t SPEED_X = 54;
constexpr int16_t SPEED_Y = 52;
constexpr int16_t SPEED_W = 212;
constexpr int16_t SPEED_H = 118;

// Alsó információs sor
constexpr int16_t INFO_X = 18;
constexpr int16_t INFO_Y = 175;
constexpr int16_t INFO_W = 284;
constexpr int16_t INFO_H = 26;

/**
 * @brief A HUD állapotát tároló struktúra
 *
 */
struct ScreenMainHudState {
    bool initialized = false;
    bool staticPainted = false;
    bool speedSpriteReady = false;
    float smoothSpeed = 0.0f;
    float lastDrawnSpeed = -1000.0f;
    uint32_t lastRedrawMs = 0;

    char lastSatText[24] = "";
    uint16_t lastSatColor = 0;

    char lastTimeText[24] = "";
    uint16_t lastTimeColor = 0;

    char lastHdopText[24] = "";
    uint16_t lastHdopColor = 0;

    char lastBottomText[128] = "";
};

ScreenMainHudState hudState;
TFT_eSprite speedSprite(&tft);

/**
 * @brief Értékeket a megadott tartományba szorítja
 * @param v A szorítani kívánt érték
 * @param lo A minimum érték
 * @param hi A maximum érték
 * @return A szorított érték
 */
float clampf(float v, float lo, float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

/**
 * @brief Sebesség ív színének meghatározása a kitöltési arány alapján
 * @param ratio A kitöltési arány (0.0 - 1.0)
 * @return A kitöltési arányhoz tartozó szín
 */
uint16_t arcColorForRatio(float ratio) {
    if (ratio < 0.55f) {
        return TFT_CYAN;
    }
    if (ratio < 0.80f) {
        return TFT_GREEN;
    }
    if (ratio < 0.93f) {
        return TFT_ORANGE;
    }
    return TFT_RED;
}

/**
 * @brief HUD panel kirajzolása
 * @param x A panel bal felső sarkának X koordinátája
 * @param y A panel bal felső sarkának Y koordinátája
 * @param w A panel szélessége
 * @param h A panel magassága
 * @param title A panel címe
 * @param value A panel értéke
 * @param valueColor Az érték szövegének színe
 *
 */
void drawHudPanel(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, const char *value, uint16_t valueColor) {
    const uint16_t panelBg = tft.color565(8, 16, 26);
    const uint16_t panelBorder = tft.color565(0, 160, 255);

    tft.fillRoundRect(x, y, w, h, 6, panelBg);
    tft.drawRoundRect(x, y, w, h, 6, panelBorder);

    tft.setFreeFont();
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(120, 180, 220), panelBg);
    tft.drawString(title, x + 6, y + 4);

    tft.setTextColor(valueColor, panelBg);
    tft.drawString(value, x + 6, y + 20);
}

/**
 * @brief HUD panel érték kirajzolása
 * @param x A panel bal felső sarkának X koordinátája
 * @param y A panel bal felső sarkának Y koordinátája
 * @param w A panel szélessége
 * @param h A panel magassága
 * @param value A kirajzolni kívánt érték szövege
 * @param valueColor Az érték szövegének színe
 *
 */
void drawHudPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *value, uint16_t valueColor) {
    const uint16_t panelBg = tft.color565(8, 16, 26);
    tft.fillRect(x + 2, y + 17, w - 4, h - 19, panelBg);
    tft.setFreeFont();
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(valueColor, panelBg);
    tft.drawString(value, x + 6, y + 20);
}

/**
 * @brief Sebesség ív kirajzolása a megadott sprite-ra
 * @param canvas A sprite, amire rajzolni kell
 * @param speedKmph Sebesség km/h-ban
 * @param centerX Az ív középpontjának X koordinátája a sprite-on belül
 * @param centerY Az ív középpontjának Y koordinátája a sprite-on belül
 * @param rOuter Külső ív sugara
 * @param rInner Belső ív sugara
 *
 */
template <typename Canvas> void drawSpeedArc(Canvas &canvas, float speedKmph, int16_t centerX, int16_t centerY, int16_t rOuter, int16_t rInner) {
    const float clampedSpeed = clampf(speedKmph, 0.0f, HUD_MAX_SPEED_KMPH);
    const float fillRatio = clampedSpeed / HUD_MAX_SPEED_KMPH;

    constexpr float startDeg = 145.0f;
    constexpr float endDeg = 395.0f;
    constexpr int segments = 52;

    for (int i = 0; i < segments; i++) {
        const float segmentRatio = static_cast<float>(i) / static_cast<float>(segments - 1);
        const float deg = startDeg + (endDeg - startDeg) * segmentRatio;
        const float rad = deg * DEG_TO_RAD;

        const int16_t xOuter = centerX + static_cast<int16_t>(std::cos(rad) * rOuter);
        const int16_t yOuter = centerY + static_cast<int16_t>(std::sin(rad) * rOuter);
        const int16_t xInner = centerX + static_cast<int16_t>(std::cos(rad) * rInner);
        const int16_t yInner = centerY + static_cast<int16_t>(std::sin(rad) * rInner);

        uint16_t color = tft.color565(22, 34, 44);
        if (segmentRatio <= fillRatio) {
            color = arcColorForRatio(segmentRatio);
        }

        canvas.drawLine(xOuter, yOuter, xInner, yInner, color);
    }

    canvas.drawCircle(centerX, centerY, rOuter + 3, tft.color565(18, 70, 110));
    canvas.drawCircle(centerX, centerY, rInner - 4, tft.color565(12, 48, 76));
}

/**
 * @brief Statikus HUD háttér kirajzolása
 * @note Ezt a függvényt csak egyszer kell meghívni, amikor a képernyő először aktiválódik, vagy amikor a képernyő teljes újrarajzolása szükséges.
 *
 */
void drawStaticHudBackground() {
    for (int16_t y = 0; y < tft.height(); y++) {
        const uint16_t bg = tft.color565(0, 10 + (y * 22) / tft.height(), 18 + (y * 34) / tft.height());
        tft.drawFastHLine(0, y, tft.width(), bg);
    }

    for (int16_t x = 0; x < tft.width(); x += 20) {
        tft.drawFastVLine(x, 0, tft.height(), tft.color565(0, 18, 26));
    }

    drawHudPanel(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, "TRACK", "--", TFT_DARKGREY);
    drawHudPanel(TIME_X, TIME_Y, TIME_W, TIME_H, "LOCAL", "--:--", TFT_DARKGREY);
    drawHudPanel(PREC_X, PREC_Y, PREC_W, PREC_H, "PREC", "HDOP --", TFT_DARKGREY);

    tft.fillRoundRect(INFO_X, INFO_Y, INFO_W, INFO_H, 6, tft.color565(6, 14, 22));
    tft.drawRoundRect(INFO_X, INFO_Y, INFO_W, INFO_H, 6, tft.color565(0, 132, 214));
}

/**
 * @brief Sebesség widgethez szükséges sprite inicializálása, ha még nem történt meg
 */
void ensureSpeedSpriteReady() {
    if (hudState.speedSpriteReady) {
        return;
    }

    speedSprite.setColorDepth(8);
    hudState.speedSpriteReady = speedSprite.createSprite(SPEED_W, SPEED_H) != nullptr;
}

/**
 * @brief Sebesség widget kirajzolása
 * @param speedKmph Sebesség km/h-ban
 * @param speedValid Sebesség érvényessége
 */
void drawSpeedWidget(float speedKmph, bool speedValid) {

    // A Sprite létrehozása, ha még nem történt meg
    ensureSpeedSpriteReady();

    // Sebesség widget kirajzolása a sprite-ra, majd a sprite kirajzolása a képernyőre
    if (hudState.speedSpriteReady) {

        // Háttér kirajzolása a sprite-ra
        for (int16_t y = 0; y < SPEED_H; y++) {
            const int16_t globalY = SPEED_Y + y;
            const uint16_t bg = tft.color565(0, 10 + (globalY * 22) / tft.height(), 18 + (globalY * 34) / tft.height());
            speedSprite.drawFastHLine(0, y, SPEED_W, bg);
        }

        // Függőleges vonalak kirajzolása a sprite-ra
        for (int16_t globalX = 0; globalX < tft.width(); globalX += 20) {
            const int16_t localX = globalX - SPEED_X;
            if (localX >= 0 && localX < SPEED_W) {
                speedSprite.drawFastVLine(localX, 0, SPEED_H, tft.color565(0, 18, 26));
            }
        }

        // Sebesség ív kirajzolása
        drawSpeedArc(speedSprite, speedKmph, 106, 72, 58, 42);

        // Sebesség szöveg kirajzolása
        speedSprite.setTextDatum(MC_DATUM);
        speedSprite.setFreeFont(&FreeSansBold24pt7b);
        speedSprite.setTextSize(1);

        char speedText[8];
        snprintf(speedText, sizeof(speedText), "%03d", static_cast<int>(std::lroundf(speedKmph)));
        const uint16_t speedColor = speedValid ? TFT_WHITE : TFT_DARKGREY;

        speedSprite.setTextColor(speedColor, TFT_BLACK);
        speedSprite.drawString(speedText, 106, 60);

        // "KM/H" felirat kirajzolása
        speedSprite.setFreeFont();
        speedSprite.setTextSize(2);
        speedSprite.setTextColor(tft.color565(120, 220, 255), TFT_BLACK);
        speedSprite.drawString("km/h", 106, 92);

        speedSprite.pushSprite(SPEED_X, SPEED_Y);
        return;
    }

    // Ha a sprite nem jött létre, akkor a sebesség widgetet közvetlenül a képernyőre rajzoljuk
    tft.fillRect(SPEED_X, SPEED_Y, SPEED_W, SPEED_H, TFT_BLACK);
    drawSpeedArc(tft, speedKmph, 160, 124, 58, 42);

    // Sebesség szöveg kirajzolása
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSansBold24pt7b);
    tft.setTextSize(1);
    tft.setTextSize(1);

    char speedText[8];
    snprintf(speedText, sizeof(speedText), "%03d", static_cast<int>(std::lroundf(speedKmph)));
    tft.setTextColor(speedValid ? TFT_WHITE : TFT_DARKGREY, TFT_BLACK);
    tft.drawString(speedText, 160, 112);

    // "KM/H" felirat kirajzolása
    tft.setFreeFont();
    tft.setTextSize(2);
    tft.setTextColor(tft.color565(120, 220, 255), TFT_BLACK);
    tft.drawString("km/h", 160, 144);
}

} // namespace

/**
 * @brief ScreenMain konstruktor
 */
ScreenMain::ScreenMain() : UIScreen(SCREEN_NAME_MAIN) {

    DEBUG("ScreenMain: Constructor called\n");

    // Feliratkozás a config változásokra
    configCallbackId = config.registerChangeCallback([this]() { this->onConfigChanged(); });

    // UI komponensek elhelyezése
    layoutComponents();

    // Kezdeti érték beállítása
    onConfigChanged();
}

/**
 * @brief ScreenMain destruktor
 */
ScreenMain::~ScreenMain() { config.unregisterCallback(configCallbackId); }

/**
 * UI komponensek elhelyezése
 */
void ScreenMain::layoutComponents() {
    // Info gomb bal alsó sarokban
    {
        const uint16_t margin = 4;
        addChild(std::make_shared<UIButton>(
            1,                                                                                                                                      // Info gomb azonosítója
            Rect(margin, tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT - margin, UIButton::DEFAULT_BUTTON_WIDTH, UIButton::DEFAULT_BUTTON_HEIGHT), // bal alsó sarok, margóval
            "Info",                                                                                                                                 //
            UIButton::ButtonType::Pushable,
            [this](const UIButton::ButtonEvent &event) {
                if (event.state == UIButton::EventButtonState::Clicked) {
                    getScreenManager()->switchToScreen(SCREEN_NAME_INFO);
                }
            },
            UIColorPalette::createDarkButtonScheme()) // sötét gomb színséma
        );
    }

    // Setup gomb jobb alsó sarokban
    {
        const uint16_t margin = 4;
        addChild(std::make_shared<UIButton>(
            2, // Setup gomb azonosítója
            Rect(tft.width() - UIButton::DEFAULT_BUTTON_WIDTH - margin, tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT - margin, UIButton::DEFAULT_BUTTON_WIDTH,
                 UIButton::DEFAULT_BUTTON_HEIGHT), // jobb alsó sarok, margóval
            "Setup",                               //
            UIButton::ButtonType::Pushable,
            [this](const UIButton::ButtonEvent &event) {
                if (event.state == UIButton::EventButtonState::Clicked) {
                    getScreenManager()->switchToScreen(SCREEN_NAME_SETUP);
                }
            },
            UIColorPalette::createDarkButtonScheme()) // sötét gomb színséma
        );
    }
}

/**
 * @brief Callback függvény, amit a Config hív meg változás esetén
 */
void ScreenMain::onConfigChanged() {
    DEBUG("ScreenMain::onConfigChanged() - Konfiguráció frissítése.\n");
    //_isTraffiAlarmEnabled = config.data.gpsTraffiAlarmEnabled;
    //_isBeeperEnabled = config.data.beeperEnabled;
    //_gpsTraffiAlarmDistance = config.data.gpsTraffiAlarmDistance;
    //_isGpsTraffiSirenAlarmEnabled = config.data.gpsTraffiSirenAlarmEnabled;

    // Ha a mód megváltozott, a méterek újrarajzolásának kényszerítése
    // if (_isExternalVoltageMode != config.data.externalVoltageMode || _isExternalTemperatureMode != config.data.externalTemperatureMode) {
    //    lastVerticalLinearSpriteUpdate = 0;
    //}
    //_isExternalVoltageMode = config.data.externalVoltageMode;
    //_isExternalTemperatureMode = config.data.externalTemperatureMode;
}

/**
 * @brief Képernyő aktiválása
 *
 * Meghívódik amikor a képernyő aktívvá válik (pl. visszatérés Info/Setup képernyőről)
 */
void ScreenMain::activate() {

    hudState.initialized = false;
    hudState.staticPainted = false;
    hudState.lastDrawnSpeed = -1000.0f;
    hudState.lastRedrawMs = 0;
    std::strcpy(hudState.lastSatText, "");
    std::strcpy(hudState.lastTimeText, "");
    std::strcpy(hudState.lastHdopText, "");
    std::strcpy(hudState.lastBottomText, "");
    hudState.lastSatColor = 0;
    hudState.lastTimeColor = 0;
    hudState.lastHdopColor = 0;

    // Beállítjuk a kényszerített újrarajzolás flag-et
    this->forceRedraw = true;

    // a következő ciklusban kényszerítjük az újrarajzolást
    markForRedraw(true); // a képernyőt és a gyerekeit  újrarajzolásra jelöljük

    // Ős activate() metódus hívása
    UIScreen::activate();
}

/**
 * Kezeli a képernyő saját ciklusát (dinamikus frissítés)
 */
void ScreenMain::handleOwnLoop() {

    const float targetSpeed = (c1_sharedGpsData.speedValid) ? c1_sharedGpsData.speedKmph : 0.0f;

    if (!hudState.initialized) {
        hudState.smoothSpeed = targetSpeed;
        hudState.initialized = true;
    } else {
        hudState.smoothSpeed += (targetSpeed - hudState.smoothSpeed) * 0.24f;
    }

    if (!Utils::timeHasPassed(hudState.lastRedrawMs, HUD_UPDATE_INTERVAL_MS)) {
        return;
    }

    hudState.lastRedrawMs = millis();

    const float delta = std::fabs(hudState.smoothSpeed - hudState.lastDrawnSpeed);
    if (!forceRedraw && delta <= 0.35f && c1_sharedGpsData.speedValid) {
        return;
    }

    forceRedraw = false;
    hudState.lastDrawnSpeed = hudState.smoothSpeed;

    const bool speedValid = c1_sharedGpsData.speedValid;
    const bool timeValid = c1_sharedGpsData.timeValid;
    const bool locationValid = c1_sharedGpsData.locationValid;

    const float speedKmph = clampf(hudState.smoothSpeed, 0.0f, HUD_MAX_SPEED_KMPH);
    const uint8_t satelliteCount = c1_sharedGpsData.satelliteCountForUI;
    const float hdop = c1_sharedGpsData.hdop;
    const uint8_t hour = c1_sharedGpsData.hour;
    const uint8_t minute = c1_sharedGpsData.minute;
    const float altitudeM = c1_sharedGpsData.altitudeM;
    const bool altitudeValid = c1_sharedGpsData.altitudeValid;
    const uint8_t fixQuality = c1_sharedGpsData.fixQuality;
    const uint8_t fixMode = c1_sharedGpsData.fixMode;
    const float courseDeg = c1_sharedGpsData.courseDeg;
    const uint32_t bootSec = c1_sharedGpsData.gpsBootTime;

    // Műholdak száma
    char satText[24];
    snprintf(satText, sizeof(satText), "%u SAT", satelliteCount);
    const uint16_t satColor = locationValid ? TFT_GREEN : TFT_ORANGE;
    if (std::strcmp(satText, hudState.lastSatText) != 0 || satColor != hudState.lastSatColor) {
        drawHudPanelValue(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, satText, satColor);
        std::strncpy(hudState.lastSatText, satText, sizeof(hudState.lastSatText) - 1);
        hudState.lastSatText[sizeof(hudState.lastSatText) - 1] = '\0';
        hudState.lastSatColor = satColor;
    }

    // Idő
    char timeText[24];
    if (timeValid) {
        snprintf(timeText, sizeof(timeText), "%02u:%02u", hour, minute);
    } else {
        snprintf(timeText, sizeof(timeText), "--:--");
    }
    const uint16_t timeColor = timeValid ? TFT_CYAN : TFT_DARKGREY;
    if (std::strcmp(timeText, hudState.lastTimeText) != 0 || timeColor != hudState.lastTimeColor) {
        drawHudPanelValue(TIME_X, TIME_Y, TIME_W, TIME_H, timeText, timeColor);
        std::strncpy(hudState.lastTimeText, timeText, sizeof(hudState.lastTimeText) - 1);
        hudState.lastTimeText[sizeof(hudState.lastTimeText) - 1] = '\0';
        hudState.lastTimeColor = timeColor;
    }

    // Pontosság (HDOP)
    char hdopText[24];
    snprintf(hdopText, sizeof(hdopText), "HDOP %.1f", hdop);
    const uint16_t hdopColor = (hdop > 0.0f && hdop < 2.5f) ? TFT_GREEN : TFT_ORANGE;
    if (std::strcmp(hdopText, hudState.lastHdopText) != 0 || hdopColor != hudState.lastHdopColor) {
        drawHudPanelValue(PREC_X, PREC_Y, PREC_W, PREC_H, hdopText, hdopColor);
        std::strncpy(hudState.lastHdopText, hdopText, sizeof(hudState.lastHdopText) - 1);
        hudState.lastHdopText[sizeof(hudState.lastHdopText) - 1] = '\0';
        hudState.lastHdopColor = hdopColor;
    }

    // Sebesség widget kirajzolása
    drawSpeedWidget(speedKmph, speedValid);

    // Alsó információs sor szövege
    char bottomText[128];
    if (altitudeValid) {
        snprintf(bottomText, sizeof(bottomText), "Q:%s  M:%s  CRS:%03d  ALT:%dm  T+%lus", GpsManager::qualityToString(fixQuality).c_str(), GpsManager::modeToString(fixMode).c_str(),
                 static_cast<int>(std::lroundf(courseDeg)), static_cast<int>(std::lroundf(altitudeM)), static_cast<unsigned long>(bootSec));
    } else {
        snprintf(bottomText, sizeof(bottomText), "Q:%s  M:%s  CRS:%03d  ALT:--  T+%lus", GpsManager::qualityToString(fixQuality).c_str(), GpsManager::modeToString(fixMode).c_str(),
                 static_cast<int>(std::lroundf(courseDeg)), static_cast<unsigned long>(bootSec));
    }

    // Alsó információs sor frissítése csak akkor, ha változott
    if (std::strcmp(bottomText, hudState.lastBottomText) != 0) {
        tft.fillRect(INFO_X + 2, INFO_Y + 2, INFO_W - 4, INFO_H - 4, tft.color565(6, 14, 22));
        tft.setTextDatum(TL_DATUM);
        tft.setFreeFont();
        tft.setTextSize(1);
        tft.setTextColor(tft.color565(170, 220, 255), tft.color565(6, 14, 22));
        tft.drawString(bottomText, INFO_X + 8, INFO_Y + 8);
        std::strncpy(hudState.lastBottomText, bottomText, sizeof(hudState.lastBottomText) - 1);
        hudState.lastBottomText[sizeof(hudState.lastBottomText) - 1] = '\0';
    }
}

/**
 * @brief Touch esemény kezelése - hőmérsékleti mód váltás
 * @param event A touch esemény, amely tartalmazza a koordinátákat és a lenyomás állapotát.
 * @return true, ha a touch eseményt kezelte a képernyő, false egyébként
 */
bool ScreenMain::handleTouch(const TouchEvent &event) {

    // Csak lenyomás eseményre reagálunk
    if (!event.pressed) {
        return UIScreen::handleTouch(event); // Továbbítjuk az ősosztálynak
    }

    // Ha nem volt találat, továbbítjuk az alaposztálynak
    return UIScreen::handleTouch(event);
}

/**
 * Kirajzolja a képernyő saját tartalmát (futurisztikus HUD)
 */
void ScreenMain::drawContent() {

    if (!hudState.staticPainted) {
        drawStaticHudBackground();
        hudState.staticPainted = true;
    }

    forceRedraw = true;
    handleOwnLoop();
}
