#include <Arduino.h>
#include <cmath>
#include <cstring>

#include "ScreenMain.h"
#include "defines.h"

extern TraffipaxManager traffipaxManager;

/**
 * @brief ScreenMain konstruktor
 */
ScreenMain::ScreenMain() : UIScreen(SCREEN_NAME_MAIN), speedSprite(&tft), sensorBarSprite(&tft) {

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
    constexpr uint16_t MARGIN = 1;

    // Info gomb bal alsó sarokban

    addChild(std::make_shared<UIButton>(
        1,                                                            // Info gomb azonosítója
        Rect(MARGIN,                                                  // x
             tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT - MARGIN, // y
             UIButton::DEFAULT_BUTTON_WIDTH,                          // szélesség
             UIButton::DEFAULT_BUTTON_HEIGHT),                        // magasság
        "Info",                                                       //
        UIButton::ButtonType::Pushable,
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->switchToScreen(SCREEN_NAME_INFO);
            }
        },
        UIColorPalette::createDarkButtonScheme()) // sötét gomb színséma
    );

    // Setup gomb jobb alsó sarokban

    addChild(std::make_shared<UIButton>(
        2,                                                            // Setup gomb azonosítója
        Rect(tft.width() - UIButton::DEFAULT_BUTTON_WIDTH - MARGIN,   // x
             tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT - MARGIN, // y
             UIButton::DEFAULT_BUTTON_WIDTH,                          // szélesség
             UIButton::DEFAULT_BUTTON_HEIGHT),                        // magasság
        "Setup",                                                      //
        UIButton::ButtonType::Pushable,
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->switchToScreen(SCREEN_NAME_SETUP);
            }
        },
        UIColorPalette::createDarkButtonScheme()) // sötét gomb színséma
    );
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

    forceRedraw = true;
}

/**
 * @brief Képernyő aktiválása
 *
 * Meghívódik amikor a képernyő aktívvá válik (pl. visszatérés Info/Setup képernyőről)
 */
void ScreenMain::activate() {

    hudState.initialized = false;
    hudState.staticPainted = false;
    hudState.lastSpeedValue = -1000.0f;
    std::strcpy(hudState.lastSpeedText, "");
    hudState.lastSpeedColor = 0;
    hudState.lastRedrawMs = 0;
    hudState.lastVoltageValue = -1000.0f;
    hudState.lastVoltageMode = !config.data.externalVoltageMode;
    hudState.lastTemperatureValue = -1000.0f;
    hudState.lastTemperatureMode = !config.data.externalTemperatureMode;
    std::strcpy(hudState.lastSatText, "");
    std::strcpy(hudState.lastTimeText, "");
    std::strcpy(hudState.lastAltText, "");
    std::strcpy(hudState.lastBottomText, "");
    hudState.lastSatColor = 0;
    hudState.lastTimeColor = 0;
    hudState.lastAltColor = 0;
    traffipaxAlertController.reset();

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

    if (!Utils::timeHasPassed(hudState.lastRedrawMs, HUD_UPDATE_INTERVAL_MS)) {
        return;
    }

    hudState.lastRedrawMs = millis();

    const float delta = std::fabs(targetSpeed - hudState.lastSpeedValue);
    const bool forceUpdate = forceRedraw;
    if (!forceUpdate && delta <= 0.35f && c1_sharedGpsData.speedValid) {
        return;
    }

    forceRedraw = false;
    hudState.lastSpeedValue = targetSpeed;

    const bool speedValid = c1_sharedGpsData.speedValid;
    const bool timeValid = c1_sharedGpsData.timeValid;
    const bool locationValid = c1_sharedGpsData.locationValid;
    const bool voltageMode = config.data.externalVoltageMode;
    const bool temperatureMode = config.data.externalTemperatureMode;

    const float speedKmph = (targetSpeed < 0.0f) ? 0.0f : targetSpeed;
    const float voltageValue = voltageMode ? c1_sharedSensorData.vBus : c1_sharedSensorData.vSys;
    const float temperatureValue = temperatureMode ? c1_sharedSensorData.externalTemperature : c1_sharedSensorData.coreTemperature;
    const uint8_t satelliteCount = c1_sharedGpsData.satelliteCountForUI;
    const float hdop = c1_sharedGpsData.hdop;
    const uint8_t hour = c1_sharedGpsData.hour;
    const uint8_t minute = c1_sharedGpsData.minute;
    const float altitudeM = c1_sharedGpsData.altitudeM;
    const bool altitudeValid = c1_sharedGpsData.altitudeValid;
    const uint8_t fixQuality = c1_sharedGpsData.fixQuality;
    const uint8_t fixMode = c1_sharedGpsData.fixMode;
    const bool voltageNeedsUpdate = forceUpdate || voltageMode != hudState.lastVoltageMode || std::fabs(voltageValue - hudState.lastVoltageValue) > 0.02f;
    const bool temperatureNeedsUpdate = forceUpdate || temperatureMode != hudState.lastTemperatureMode || std::fabs(temperatureValue - hudState.lastTemperatureValue) > 0.05f;
    const bool anyBarNeedsUpdate = voltageNeedsUpdate || temperatureNeedsUpdate;

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

    // Jobb felső panel: magasság
    char altText[24];
    if (altitudeValid) {
        snprintf(altText, sizeof(altText), "%d m", static_cast<int>(std::lroundf(altitudeM)));
    } else {
        snprintf(altText, sizeof(altText), "-- m");
    }
    const uint16_t altColor = altitudeValid ? TFT_CYAN : TFT_DARKGREY;
    if (std::strcmp(altText, hudState.lastAltText) != 0 || altColor != hudState.lastAltColor) {
        drawHudPanelValue(PREC_X, PREC_Y, PREC_W, PREC_H, altText, altColor);
        std::strncpy(hudState.lastAltText, altText, sizeof(hudState.lastAltText) - 1);
        hudState.lastAltText[sizeof(hudState.lastAltText) - 1] = '\0';
        hudState.lastAltColor = altColor;
    }

    // Sebesség widget kirajzolása
    drawSpeedWidget(speedKmph, speedValid, forceUpdate);

    if (anyBarNeedsUpdate) {
        ensureSensorBarSpriteReady();
        if (hudState.sensorBarSpriteReady) {
            drawSensorBarSprite(voltageValue, voltageMode ? 3.5f : 2.7f, voltageMode ? 15.5f : 5.5f, voltageMode ? "Vbus" : "Vsys", "V", false);
            drawSensorBarSprite(temperatureValue, -20.0f, 70.0f, temperatureMode ? "EXT" : "CPU", "C", true);
        } else {
            tft.fillRect(SENSOR_BAR_X, SENSOR_BAR_Y, SENSOR_BAR_W, SENSOR_BAR_H, TFT_BLACK);
            tft.fillRect(SENSOR_BAR_RIGHT_X, SENSOR_BAR_Y, SENSOR_BAR_W, SENSOR_BAR_H, TFT_BLACK);
        }

        hudState.lastVoltageValue = voltageValue;
        hudState.lastVoltageMode = voltageMode;
        hudState.lastTemperatureValue = temperatureValue;
        hudState.lastTemperatureMode = temperatureMode;
    }

    // Alsó információs sor szövege
    char bottomText[128];
    const bool hdopValid = (hdop > 0.0f && hdop < 99.0f);
    if (hdopValid) {
        snprintf(bottomText, sizeof(bottomText), "Q:%s  M:%s  HDOP:%.1f", GpsManager::qualityToString(fixQuality).c_str(), GpsManager::modeToString(fixMode).c_str(), hdop);
    } else {
        snprintf(bottomText, sizeof(bottomText), "Q:%s  M:%s  HDOP:--", GpsManager::qualityToString(fixQuality).c_str(), GpsManager::modeToString(fixMode).c_str());
    }

    // Alsó információs sor frissítése csak akkor, ha változott
    if (std::strcmp(bottomText, hudState.lastBottomText) != 0) {
        tft.fillRect(INFO_X + 2, INFO_Y + 2, INFO_W - 4, INFO_H - 4, tft.color565(6, 14, 22));
        tft.setTextDatum(MC_DATUM);
        tft.setFreeFont();
        tft.setTextSize(1);
        tft.setTextColor(tft.color565(170, 220, 255), tft.color565(6, 14, 22));
        tft.drawString(bottomText, INFO_X + (INFO_W / 2), INFO_Y + (INFO_H / 2));
        std::strncpy(hudState.lastBottomText, bottomText, sizeof(hudState.lastBottomText) - 1);
        hudState.lastBottomText[sizeof(hudState.lastBottomText) - 1] = '\0';
    }

    // Traffipax közeledés / távolodás figyelmeztetés
    const TraffipaxAlertController::ConfigSnapshot traffiCfg{config.data.gpsTraffiAlarmEnabled, config.data.gpsTraffiSirenAlarmEnabled, config.data.beeperEnabled, config.data.gpsTraffiAlarmDistance};
    const auto traffiResult = traffipaxAlertController.update(c1_sharedGpsData.lat, c1_sharedGpsData.lng, c1_sharedGpsData.locationValid, traffiCfg, millis(), tft, traffipaxManager);
    if (traffiResult.baseAreaNeedsRestore) {
        drawTraffipaxBaseArea();
    }
    if (traffiResult.hudNeedsRepaint) {
        hudState.staticPainted = false;
        hudState.lastRedrawMs = 0;
        forceRedraw = true;
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

    // Bal oldali függőleges bar: feszültségmérés mód váltás
    if (event.x >= SENSOR_BAR_X && event.x < SENSOR_BAR_X + SENSOR_BAR_W && event.y >= SENSOR_BAR_Y && event.y < SENSOR_BAR_Y + SENSOR_BAR_H) {
        config.data.externalVoltageMode = !config.data.externalVoltageMode;
        config.checkSave();
        forceRedraw = true;
        hudState.lastRedrawMs = 0;
        if (config.data.beeperEnabled) {
            Utils::beepTick();
        }
        return true;
    }

    // Jobb oldali függőleges bar: hőmérsékletmérés mód váltás
    if (event.x >= SENSOR_BAR_RIGHT_X && event.x < SENSOR_BAR_RIGHT_X + SENSOR_BAR_W && event.y >= SENSOR_BAR_Y && event.y < SENSOR_BAR_Y + SENSOR_BAR_H) {
        config.data.externalTemperatureMode = !config.data.externalTemperatureMode;
        config.checkSave();
        forceRedraw = true;
        hudState.lastRedrawMs = 0;
        if (config.data.beeperEnabled) {
            Utils::beepTick();
        }
        return true;
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

/**
 * @brief Traffipax riasztás sáv kirajzolása a képernyő tetejére
 */
void ScreenMain::drawTraffipaxBaseArea() {

    constexpr int16_t TRAFFI_ALERT_H = 48;
    for (int16_t y = 0; y < TRAFFI_ALERT_H; y++) {
        const uint16_t bg = tft.color565(0, 40 + (y * 60) / tft.height(), 80 + (y * 100) / tft.height());
        tft.drawFastHLine(0, y, tft.width(), bg);
    }

    // A top HUD elemei, amelyek az alert sáv alatt is látszanak, ha nincs riasztás
    drawHudPanel(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, "TRACK", "--", TFT_DARKGREY);
    drawHudPanel(TIME_X, TIME_Y, TIME_W, TIME_H, "LOCAL", "--:--", TFT_DARKGREY);
    drawHudPanel(PREC_X, PREC_Y, PREC_W, PREC_H, "ALT", "-- m", TFT_DARKGREY);
}

/**
 * @brief Értékeket a megadott tartományba szorítja
 * @param v A szorítani kívánt érték
 * @param lo A minimum érték
 * @param hi A maximum érték
 * @return A szorított érték
 */
float ScreenMain::clampf(float v, float lo, float hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

/**
 * @brief Színezés a függőleges meterhez a kitöltési arány alapján
 *
 * @param ratio A kitöltési arány
 * @param temperatureBar true, ha hőmérséklet meter, false ha feszültség meter
 */
uint16_t ScreenMain::meterColorForRatio(float ratio, bool temperatureBar) {
    if (temperatureBar) {
        if (ratio < 0.25f) {
            return TFT_CYAN;
        }
        if (ratio < 0.50f) {
            return TFT_GREEN;
        }
        if (ratio < 0.75f) {
            return TFT_YELLOW;
        }
        if (ratio < 0.90f) {
            return TFT_ORANGE;
        }
        return TFT_RED;
    }

    if (ratio < 0.25f) {
        return TFT_RED;
    }
    if (ratio < 0.50f) {
        return TFT_ORANGE;
    }
    if (ratio < 0.75f) {
        return TFT_YELLOW;
    }
    if (ratio < 0.90f) {
        return TFT_GREENYELLOW;
    }
    return TFT_GREEN;
}

/**
 * @brief HUD (Head-Up Display) panel kirajzolása
 *
 * @param x A panel bal felső sarkának X koordinátája
 * @param y A panel bal felső sarkának Y koordinátája
 * @param w A panel szélessége
 * @param h A panel magassága
 * @param title A panel címe
 * @param value A panel értéke
 * @param valueColor Az érték szövegének színe
 *
 */
void ScreenMain::drawHudPanel(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, const char *value, uint16_t valueColor) {
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
 * @brief HUD (Head-Up Display) panel érték kirajzolása
 *
 * @param x A panel bal felső sarkának X koordinátája
 * @param y A panel bal felső sarkának Y koordinátája
 * @param w A panel szélessége
 * @param h A panel magassága
 * @param value A kirajzolni kívánt érték szövege
 * @param valueColor Az érték szövegének színe
 *
 */
void ScreenMain::drawHudPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *value, uint16_t valueColor) {
    const uint16_t panelBg = tft.color565(8, 16, 26);
    tft.fillRect(x + 2, y + 17, w - 4, h - 19, panelBg);
    tft.setFreeFont();
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(valueColor, panelBg);
    tft.drawString(value, x + 6, y + 20);
}

/**
 * @brief Függőleges meter sprite rajzolása
 * @param value Mért érték
 * @param minVal Minimum érték
 * @param maxVal Maximum érték
 * @param title Címke a meter tetején
 * @param unit Mértékegység a bottom textben
 * @param temperatureBar true, ha hőmérséklet meter
 */
void ScreenMain::drawSensorBarSprite(float value, float minVal, float maxVal, const char *title, const char *unit, bool temperatureBar) {
    const uint16_t spriteBg = tft.color565(6, 14, 22);
    const uint16_t frameColor = tft.color565(0, 132, 214);
    const uint16_t textColor = tft.color565(170, 220, 255);

    sensorBarSprite.fillSprite(spriteBg);
    sensorBarSprite.drawRoundRect(0, 0, SENSOR_BAR_W, SENSOR_BAR_H, 4, frameColor);

    sensorBarSprite.setFreeFont();
    sensorBarSprite.setTextSize(1);
    sensorBarSprite.setTextDatum(MC_DATUM);
    sensorBarSprite.setTextColor(textColor, spriteBg);
    sensorBarSprite.drawString(title, SENSOR_BAR_W / 2, 8);

    constexpr uint8_t BAR_SEGMENTS = 8;
    constexpr int16_t BAR_W = 18;
    constexpr int16_t BAR_H = 8;
    constexpr int16_t BAR_GAP = 1;
    const int16_t barX = (SENSOR_BAR_W - BAR_W) / 2;
    const int16_t topSpace = 18;
    const int16_t valueSpace = 16;
    const int16_t barTotalH = BAR_SEGMENTS * BAR_H + (BAR_SEGMENTS - 1) * BAR_GAP;
    const int16_t barStartY = topSpace + (SENSOR_BAR_H - topSpace - valueSpace - barTotalH) / 2;

    float normalized = 0.0f;
    if (maxVal > minVal) {
        normalized = clampf((value - minVal) / (maxVal - minVal), 0.0f, 1.0f);
    }
    int filledSegments = static_cast<int>(std::lroundf(normalized * BAR_SEGMENTS));
    if (filledSegments < 0) {
        filledSegments = 0;
    }
    if (filledSegments > BAR_SEGMENTS) {
        filledSegments = BAR_SEGMENTS;
    }

    for (int segment = 0; segment < BAR_SEGMENTS; segment++) {
        const int16_t y = barStartY + (BAR_SEGMENTS - 1 - segment) * (BAR_H + BAR_GAP);
        uint16_t color = TFT_DARKGREY;
        if (segment < filledSegments) {
            const float segmentRatio = static_cast<float>(segment + 1) / BAR_SEGMENTS;
            color = meterColorForRatio(segmentRatio, temperatureBar);
        }
        sensorBarSprite.fillRoundRect(barX, y, BAR_W, BAR_H, 2, color);
    }

    char valueText[20];
    snprintf(valueText, sizeof(valueText), "%.1f%s", value, unit);
    sensorBarSprite.setTextColor(temperatureBar ? TFT_ORANGE : TFT_GREENYELLOW, spriteBg);
    sensorBarSprite.drawString(valueText, SENSOR_BAR_W / 2, SENSOR_BAR_H - 10);

    sensorBarSprite.pushSprite(temperatureBar ? SENSOR_BAR_RIGHT_X : SENSOR_BAR_X, SENSOR_BAR_Y);
}

/**
 * @brief Sensor bar sprite előkészítése, ha még nincs létrehozva
 */
void ScreenMain::ensureSensorBarSpriteReady() {
    if (hudState.sensorBarSpriteReady) {
        return;
    }

    sensorBarSprite.setColorDepth(8);
    hudState.sensorBarSpriteReady = sensorBarSprite.createSprite(SENSOR_BAR_W, SENSOR_BAR_H) != nullptr;
}

/**
 * @brief Statikus HUD (Head-Up Display) háttér kirajzolása
 * @note Ezt a függvényt csak egyszer kell meghívni, amikor a képernyő először aktiválódik, vagy amikor a képernyő teljes újrarajzolása szükséges.
 *
 */
void ScreenMain::drawStaticHudBackground() {
    for (int16_t y = 0; y < tft.height(); y++) {
        const uint16_t bg = tft.color565(0, 10 + (y * 22) / tft.height(), 18 + (y * 34) / tft.height());
        tft.drawFastHLine(0, y, tft.width(), bg);
    }

    for (int16_t x = 0; x < tft.width(); x += 20) {
        tft.drawFastVLine(x, 0, tft.height(), tft.color565(0, 18, 26));
    }

    drawHudPanel(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, "TRACK", "--", TFT_DARKGREY);
    drawHudPanel(TIME_X, TIME_Y, TIME_W, TIME_H, "LOCAL", "--:--", TFT_DARKGREY);
    drawHudPanel(PREC_X, PREC_Y, PREC_W, PREC_H, "ALT", "-- m", TFT_DARKGREY);

    updateSpeedValueLayoutForFont();

    // Statikus sebesség mértékegység felirat
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont();
    tft.setTextSize(2);
    tft.setTextColor(tft.color565(120, 220, 255));
    const int16_t kmhY = hudState.speedValueY + hudState.speedValueH + 12;
    tft.drawString("km/h", SPEED_X + (SPEED_W / 2), kmhY <= (SPEED_Y + SPEED_H - 8) ? kmhY : SPEED_UNIT_BASELINE_Y);

    tft.fillRoundRect(INFO_X, INFO_Y, INFO_W, INFO_H, 6, tft.color565(6, 14, 22));
    tft.drawRoundRect(INFO_X, INFO_Y, INFO_W, INFO_H, 6, tft.color565(0, 132, 214));
}

void ScreenMain::updateSpeedValueLayoutForFont() {
    constexpr int16_t PAD_X = 10;
    constexpr int16_t PAD_Y = 8;
    constexpr int16_t RESERVED_FOR_UNIT = 28;
    constexpr int16_t MIN_W = 96;
    constexpr int16_t MIN_H = 54;

    // A sprite tényleges fontbeállításával mérünk, így a méret mindig konzisztens.
    speedSprite.setTextFont(SPEED_VALUE_FONT);
    speedSprite.setTextSize(SPEED_VALUE_TEXT_SIZE);
    int16_t textW = speedSprite.textWidth("888");
    int16_t textH = speedSprite.fontHeight();

    if (textW <= 0) {
        textW = MIN_W - (PAD_X * 2);
    }
    if (textH <= 0) {
        textH = MIN_H - (PAD_Y * 2);
    }

    int16_t desiredW = textW + PAD_X * 2;
    int16_t desiredH = textH + PAD_Y * 2;

    const int16_t maxH = SPEED_H - RESERVED_FOR_UNIT;
    desiredW = static_cast<int16_t>(clampf(desiredW, MIN_W, SPEED_W));
    desiredH = static_cast<int16_t>(clampf(desiredH, MIN_H, maxH));

    hudState.speedValueW = desiredW;
    hudState.speedValueH = desiredH;
    hudState.speedValueX = SPEED_X + (SPEED_W - desiredW) / 2;
    hudState.speedValueY = SPEED_Y + ((maxH - desiredH) / 2);
}

/**
 * @brief Sebesség widgethez szükséges sprite inicializálása, ha még nem történt meg
 */
void ScreenMain::ensureSpeedSpriteReady() {
    updateSpeedValueLayoutForFont();

    if (hudState.speedSpriteReady && (speedSprite.width() != hudState.speedValueW || speedSprite.height() != hudState.speedValueH)) {
        speedSprite.deleteSprite();
        hudState.speedSpriteReady = false;
    }

    if (hudState.speedSpriteReady) {
        return;
    }

    speedSprite.setColorDepth(8);
    hudState.speedSpriteReady = speedSprite.createSprite(hudState.speedValueW, hudState.speedValueH) != nullptr;
}

/**
 * @brief Sebesség widget kirajzolása
 * @param speedKmph Sebesség km/h-ban
 * @param speedValid Sebesség érvényessége
 */
void ScreenMain::drawSpeedWidget(float speedKmph, bool speedValid, bool forceUpdate) {

    char speedText[8];
    snprintf(speedText, sizeof(speedText), "%d", static_cast<int>(std::lroundf(speedKmph)));
    const uint16_t speedColor = speedValid ? TFT_WHITE : TFT_DARKGREY;

    // Ha a sebesség szöveg és szín nem változott, és nincs kényszerített frissítés, akkor nem rajzolunk újra
    if (!forceUpdate && std::strcmp(speedText, hudState.lastSpeedText) == 0 && speedColor == hudState.lastSpeedColor) {
        return;
    }

    // Sprite előkészítése a sebesség widgethez
    ensureSpeedSpriteReady();

    if (hudState.speedSpriteReady) {
        // Háttér kirajzolása a sebesség szám területére sprite-ra
        for (int16_t y = 0; y < hudState.speedValueH; y++) {
            const int16_t globalY = hudState.speedValueY + y;
            const uint16_t bg = tft.color565(0, 10 + (globalY * 22) / tft.height(), 18 + (globalY * 34) / tft.height());
            speedSprite.drawFastHLine(0, y, hudState.speedValueW, bg);
        }

        // Függőleges vonalak kirajzolása sprite-ra
        for (int16_t globalX = 0; globalX < tft.width(); globalX += 20) {
            const int16_t localX = globalX - hudState.speedValueX;
            if (localX >= 0 && localX < hudState.speedValueW) {
                speedSprite.drawFastVLine(localX, 0, hudState.speedValueH, tft.color565(0, 18, 26));
            }
        }

        // Sebesség számérték kirajzolása sprite-ra
        speedSprite.setTextDatum(MC_DATUM);
        speedSprite.setTextFont(SPEED_VALUE_FONT);
        speedSprite.setTextSize(SPEED_VALUE_TEXT_SIZE);
        speedSprite.setTextColor(speedColor);
        speedSprite.drawString(speedText, hudState.speedValueW / 2, hudState.speedValueH / 2);

        // Sprite kirajzolása a képernyőre
        speedSprite.pushSprite(hudState.speedValueX, hudState.speedValueY);
        std::strncpy(hudState.lastSpeedText, speedText, sizeof(hudState.lastSpeedText) - 1);
        hudState.lastSpeedText[sizeof(hudState.lastSpeedText) - 1] = '\0';
        hudState.lastSpeedColor = speedColor;
        return;
    }

    // Háttér kirajzolása a sebesség szám területére
    for (int16_t y = 0; y < hudState.speedValueH; y++) {
        const int16_t globalY = hudState.speedValueY + y;
        const uint16_t bg = tft.color565(0, 10 + (globalY * 22) / tft.height(), 18 + (globalY * 34) / tft.height());
        tft.drawFastHLine(hudState.speedValueX, globalY, hudState.speedValueW, bg);
    }

    // Függőleges vonalak kirajzolása a sebesség szám területen
    for (int16_t globalX = 0; globalX < tft.width(); globalX += 20) {
        if (globalX >= hudState.speedValueX && globalX < hudState.speedValueX + hudState.speedValueW) {
            tft.drawFastVLine(globalX, hudState.speedValueY, hudState.speedValueH, tft.color565(0, 18, 26));
        }
    }

    // Sebesség szöveg kirajzolása
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(SPEED_VALUE_FONT);
    tft.setTextSize(SPEED_VALUE_TEXT_SIZE);

    tft.setTextColor(speedColor);
    tft.drawString(speedText, hudState.speedValueX + (hudState.speedValueW / 2), hudState.speedValueY + (hudState.speedValueH / 2));

    std::strncpy(hudState.lastSpeedText, speedText, sizeof(hudState.lastSpeedText) - 1);
    hudState.lastSpeedText[sizeof(hudState.lastSpeedText) - 1] = '\0';
    hudState.lastSpeedColor = speedColor;
}
