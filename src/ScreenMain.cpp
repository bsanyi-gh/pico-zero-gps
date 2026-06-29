#include <Arduino.h>
#include <cmath>
#include <cstring>

#include "LinearMeter.h"
#include "ScreenMain.h"
#include "defines.h"

extern TraffipaxManager traffipaxManager;

/**
 * @brief ScreenMain konstruktor
 */
ScreenMain::ScreenMain() : UIScreen(SCREEN_NAME_MAIN), speedSprite(&tft), sensorBarSprite(&tft), SENSOR_BAR_X_RIGHT(tft.width() - SENSOR_BAR_W) {

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
    std::strcpy(hudState.lastTrackMetaText, "");
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

    const float rawTargetSpeed = (c1_sharedGpsData.speedValid) ? c1_sharedGpsData.speedKmph : 0.0f;
    float targetSpeed = (rawTargetSpeed < 0.0f) ? 0.0f : rawTargetSpeed;

    // Álló helyzetben ne billegjen: 0-ról csak 5 km/h felett induljon el a kijelzés.
    if (c1_sharedGpsData.speedValid && hudState.lastSpeedValue <= 0.01f && targetSpeed < 5.0f) {
        targetSpeed = 0.0f;
    }

    if (!Utils::timeHasPassed(hudState.lastRedrawMs, HUD_UPDATE_INTERVAL_MS)) {
        return;
    }

    hudState.lastRedrawMs = millis();

    const bool forceUpdate = forceRedraw;
    forceRedraw = false;
    hudState.lastSpeedValue = targetSpeed;

    const float speedKmph = (targetSpeed < 0.0f) ? 0.0f : targetSpeed;
    if (c1_sharedGpsData.speedValid && speedKmph > maxSpeedSinceBoot) {
        maxSpeedSinceBoot = speedKmph;
    }

    const float voltageValue = config.data.externalVoltageMode ? c1_sharedSensorData.vBus : c1_sharedSensorData.vSys;
    const float temperatureValue = config.data.externalTemperatureMode ? c1_sharedSensorData.externalTemperature : c1_sharedSensorData.coreTemperature;

    const bool voltageNeedsUpdate = forceUpdate || config.data.externalVoltageMode != hudState.lastVoltageMode || std::fabs(voltageValue - hudState.lastVoltageValue) > 0.02f;
    const bool temperatureNeedsUpdate = forceUpdate || config.data.externalTemperatureMode != hudState.lastTemperatureMode || std::fabs(temperatureValue - hudState.lastTemperatureValue) > 0.05f;
    const bool anyBarNeedsUpdate = voltageNeedsUpdate || temperatureNeedsUpdate;

    // Műholdak száma + fix adatok
    char satText[24];
    snprintf(satText, sizeof(satText), "%u / %u", c1_sharedGpsData.satelliteCount, c1_sharedGpsData.satelliteCountForUI);

    // Műholdak minősége és fix mód
    char trackMetaText[64];
    snprintf(trackMetaText, sizeof(trackMetaText), "Q:%s|M:%s", GpsManager::qualityToString(c1_sharedGpsData.fixQuality).c_str(), GpsManager::modeToString(c1_sharedGpsData.fixMode).c_str());
    const uint16_t satColor = c1_sharedGpsData.locationValid ? TFT_GREEN : TFT_ORANGE;
    const bool satNeedsUpdate = std::strcmp(satText, hudState.lastSatText) != 0 || satColor != hudState.lastSatColor;
    const bool trackMetaNeedsUpdate = std::strcmp(trackMetaText, hudState.lastTrackMetaText) != 0;
    if (satNeedsUpdate || trackMetaNeedsUpdate) {
        const String quality = "Q: " + GpsManager::qualityToString(c1_sharedGpsData.fixQuality);
        const String mode = "M: " + GpsManager::modeToString(c1_sharedGpsData.fixMode);

        drawTrackPanelValue(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, satText, quality.c_str(), mode.c_str(), satColor, satNeedsUpdate, trackMetaNeedsUpdate);

        if (satNeedsUpdate) {
            std::strncpy(hudState.lastSatText, satText, sizeof(hudState.lastSatText) - 1);
            hudState.lastSatText[sizeof(hudState.lastSatText) - 1] = '\0';
            hudState.lastSatColor = satColor;
        }

        if (trackMetaNeedsUpdate) {
            std::strncpy(hudState.lastTrackMetaText, trackMetaText, sizeof(hudState.lastTrackMetaText) - 1);
            hudState.lastTrackMetaText[sizeof(hudState.lastTrackMetaText) - 1] = '\0';
        }
    }

    // Idő
    char timeText[24];
    if (c1_sharedGpsData.timeValid) {
        snprintf(timeText, sizeof(timeText), "%02u:%02u", c1_sharedGpsData.hour, c1_sharedGpsData.minute);
    } else {
        snprintf(timeText, sizeof(timeText), "--:--");
    }
    const uint16_t timeColor = c1_sharedGpsData.timeValid ? TFT_CYAN : TFT_DARKGREY;
    if (std::strcmp(timeText, hudState.lastTimeText) != 0 || timeColor != hudState.lastTimeColor) {
        drawHudPanelValue(TIME_X, TIME_Y, TIME_W, TIME_H, timeText, timeColor);
        std::strncpy(hudState.lastTimeText, timeText, sizeof(hudState.lastTimeText) - 1);
        hudState.lastTimeText[sizeof(hudState.lastTimeText) - 1] = '\0';
        hudState.lastTimeColor = timeColor;
    }

    // Jobb felső panel: magasság
    char altText[24];
    if (c1_sharedGpsData.altitudeValid) {
        snprintf(altText, sizeof(altText), "%d", static_cast<int>(std::lroundf(c1_sharedGpsData.altitudeM)));
    } else {
        snprintf(altText, sizeof(altText), "--");
    }
    const uint16_t altColor = c1_sharedGpsData.altitudeValid ? TFT_CYAN : TFT_DARKGREY;
    if (std::strcmp(altText, hudState.lastAltText) != 0 || altColor != hudState.lastAltColor) {
        drawAltitudePanelValue(PREC_X, PREC_Y, PREC_W, PREC_H, altText, altColor);
        std::strncpy(hudState.lastAltText, altText, sizeof(hudState.lastAltText) - 1);
        hudState.lastAltText[sizeof(hudState.lastAltText) - 1] = '\0';
        hudState.lastAltColor = altColor;
    }

    // Sebesség widget kirajzolása ugyanazzal az 500 ms HUD ciklussal.
    drawSpeedWidget(speedKmph, c1_sharedGpsData.speedValid, forceUpdate);

    // Függőleges bar-ok frissítése, ha szükséges
    if (anyBarNeedsUpdate) {

        // Sprite legyártása, ha még nem létezik
        ensureSensorBarSpriteReady();

        // Ha a sprite létrejött, kirajzoljuk a bar-okat
        if (hudState.sensorBarSpriteReady) {

            //  Vertical Line bar - Battery (sprite-os)
            verticalLinearMeter(&sensorBarSprite,                                                        //
                                SENSOR_BAR_H,                                                            //
                                SENSOR_BAR_W,                                                            //
                                config.data.externalVoltageMode ? "Vbus [V]" : "Vsys [V]",               // Feszmérő mód: true = VBus, false = VSys
                                voltageValue,                                                            // value
                                config.data.externalVoltageMode ? VBUS_BARMETER_MIN : VSYS_BARMETER_MIN, // minVal
                                config.data.externalVoltageMode ? VBUS_BARMETER_MAX : VSYS_BARMETER_MAX, // maxVal
                                SENSOR_BAR_X,                                                            // x
                                SENSOR_BAR_Y_BOTTOM,                                                     // y: sprite alsó éle
                                25,                                                                      // bar-w
                                10,                                                                      // bar-h
                                1,                                                                       // gap
                                8,                                                                       // n
                                RED2GREEN);                                                              // color

            // Vertical Line bar - Temperature
            verticalLinearMeter(&sensorBarSprite,                                            //
                                SENSOR_BAR_H,                                                //
                                SENSOR_BAR_W,                                                //
                                config.data.externalTemperatureMode ? "Ext [C]" : "CPU [C]", // category
                                temperatureValue,                                            // value
                                TEMP_BARMETER_MIN,                                           // minVal
                                TEMP_BARMETER_MAX,                                           // maxVal
                                SENSOR_BAR_X_RIGHT,                                          // x: sprite szélesség beszámítva
                                SENSOR_BAR_Y_BOTTOM,                                         // y: sprite alsó éle
                                25,                                                          // bar-w
                                10,                                                          // bar-h
                                1,                                                           // gap
                                8,                                                           // n
                                BLUE2RED,                                                    // color
                                true);                                                       // bal oldalt legyenek az értékek
        }

        hudState.lastVoltageValue = voltageValue;
        hudState.lastVoltageMode = config.data.externalVoltageMode;
        hudState.lastTemperatureValue = temperatureValue;
        hudState.lastTemperatureMode = config.data.externalTemperatureMode;
    }

    // Alsó információs sor: fix oszlopokban balra igazított HDOP és MAX érték.
    char hdopValueText[16];
    const bool hdopValid = (c1_sharedGpsData.hdop > 0.0f && c1_sharedGpsData.hdop < 99.0f);
    if (hdopValid) {
        snprintf(hdopValueText, sizeof(hdopValueText), "%.1f", c1_sharedGpsData.hdop);
    } else {
        snprintf(hdopValueText, sizeof(hdopValueText), "--");
    }
    const int maxSpeedDisplay = static_cast<int>(std::lroundf(maxSpeedSinceBoot));

    char bottomText[128];
    snprintf(bottomText, sizeof(bottomText), "H:%s|MX:%03d", hdopValueText, maxSpeedDisplay);

    // Alsó információs sor frissítése csak akkor, ha változott
    if (std::strcmp(bottomText, hudState.lastBottomText) != 0) {
        const uint16_t infoBg = tft.color565(6, 14, 22);
        tft.fillRect(INFO_X + 2, INFO_Y + 2, INFO_W - 4, INFO_H - 4, infoBg);

        tft.setFreeFont();
        tft.setTextSize(1);
        tft.setTextColor(tft.color565(170, 220, 255), infoBg);
        tft.setTextDatum(ML_DATUM);

        // Középre igazított sor Y koordinátája
        const int16_t rowY = INFO_Y + (INFO_H / 2);

        // HDOP
        char hdopColText[24];
        const int16_t hdopColLeft = INFO_X + 8;
        snprintf(hdopColText, sizeof(hdopColText), "HDOP: %s", hdopValueText);
        tft.drawString(hdopColText, hdopColLeft, rowY);

        // MAX sebesség
        char maxColText[24];
        const int16_t maxColLeft = INFO_X + 80;
        snprintf(maxColText, sizeof(maxColText), "Max speed: %d", maxSpeedDisplay);
        tft.drawString(maxColText, maxColLeft, rowY);

        // Alsó információs sor szövegének frissítése
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

    // Sat panel érintésre Satellite képernyőre váltunk.
    if (event.x >= TRACK_X && event.x < TRACK_X + TRACK_W && event.y >= TRACK_Y && event.y < TRACK_Y + TRACK_H) {
        if (getScreenManager()) {
            getScreenManager()->switchToScreen(SCREEN_NAME_SATS);
        }
        if (config.data.beeperEnabled) {
            Utils::beepTick();
        }
        return true;
    }

    // Bal oldali függőleges bar: feszültségmérés mód váltás
    if (event.x >= SENSOR_BAR_X && event.x < SENSOR_BAR_X + SENSOR_BAR_W && event.y >= SENSOR_BAR_Y_TOP && event.y < SENSOR_BAR_Y_BOTTOM) {
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
    if (event.x >= SENSOR_BAR_X_RIGHT && event.x < SENSOR_BAR_X_RIGHT + SENSOR_BAR_W && event.y >= SENSOR_BAR_Y_TOP && event.y < SENSOR_BAR_Y_BOTTOM) {
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
 * @brief Kirajzolja a képernyő saját tartalmát (futurisztikus HUD)
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
    drawHudPanel(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, "Sat", "--", TFT_DARKGREY);
    drawHudPanel(TIME_X, TIME_Y, TIME_W, TIME_H, "Local Time", "--:--", TFT_DARKGREY);
    drawHudPanel(PREC_X, PREC_Y, PREC_W, PREC_H, "Altitude", "-- m", TFT_DARKGREY);
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
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(valueColor, panelBg);
    tft.drawString(value, x + 6, y + 20);
    tft.setFreeFont();
}

/**
 * @brief Magasság panel érték kirajzolása kis 'm' mértékegységgel
 * @param x A panel bal felső sarkának X koordinátája
 * @param y A panel bal felső sarkának Y koordinátája
 * @param w A panel szélessége
 * @param h A panel magassága
 * @param value A kirajzolni kívánt érték szövege (pl. "123 m")
 * @param valueColor Az érték szövegének színe
 *
 */
void ScreenMain::drawAltitudePanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *value, uint16_t valueColor) {
    const uint16_t panelBg = tft.color565(8, 16, 26);

    // A törlés legyen beljebb, hogy biztosan ne érintse a keretet/lekerekített sarkot.
    const int16_t valueAreaX = x + 4;
    const int16_t valueAreaY = y + 18;
    const int16_t valueAreaW = w - 8;
    const int16_t valueAreaH = h - 24;
    tft.fillRect(valueAreaX, valueAreaY, valueAreaW, valueAreaH, panelBg);

    // Nagy számérték
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(valueColor, panelBg);
    tft.drawString(value, x + 6, y + 20);

    // Kisebb mértékegység (m)
    const int16_t valueW = tft.textWidth(value);
    tft.setFreeFont();
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, panelBg);
    tft.drawString("m", x + 6 + valueW + 4, y + 26);

    tft.setFreeFont();
}

/**
 * @brief Műholdak száma, minőség és mód értékek kirajzolása a "Sat" panelen
 *
 * @param x A panel bal felső sarkának X koordinátája
 * @param y A panel bal felső sarkának Y koordinátája
 * @param w A panel szélessége
 * @param h A panel magassága
 * @param satValue A műholdak száma szöveg
 * @param qualityValue A minőség szöveg
 * @param modeValue A mód szöveg
 * @param satColor A műholdak száma szövegének színe
 * @param updateSat true, ha a műholdak száma szöveget frissíteni kell
 * @param updateMeta true, ha a minőség és mód szövegeket frissíteni kell
 */
void ScreenMain::drawTrackPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *satValue, const char *qualityValue, const char *modeValue, uint16_t satColor, bool updateSat, bool updateMeta) {
    const uint16_t panelBg = tft.color565(8, 16, 26);
    constexpr int16_t SAT_VALUE_PAD_X = 10;

    // A SAT érték törlési szélességét a tényleges font alapján fixáljuk "88 / 88" mintára,
    // így nem tud belelógni a jobb oldali Q/M zónába.
    static int16_t satValueAreaW = 0;
    if (satValueAreaW <= 0) {
        tft.setFreeFont(&FreeSansBold9pt7b);
        satValueAreaW = tft.textWidth("88 / 88") + SAT_VALUE_PAD_X;
    }

    // SAT (bal oldal) terület, csak ha változott.
    if (updateSat) {
        int16_t satFillW = satValueAreaW;
        const int16_t maxSafeSatFillW = (w / 2) - 4;
        if (satFillW > maxSafeSatFillW) {
            satFillW = maxSafeSatFillW;
        }
        if (satFillW < 8) {
            satFillW = 8;
        }

        tft.fillRect(x + 2, y + 17, satFillW, h - 19, panelBg);
        tft.setFreeFont(&FreeSansBold9pt7b);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.setTextColor(satColor, panelBg);
        tft.drawString(satValue, x + 6, y + 20);
    }

    // Q/M (jobb oldal) terület: teljes korábbi szöveg törlése, majd jobbra igazított kiírás.
    if (updateMeta) {
        const int16_t metaLeft = x + (w / 2);
        const int16_t metaWidth = (x + w - 3) - metaLeft;
        tft.fillRect(metaLeft, y + 3, metaWidth, h - 6, panelBg); // Töröljük a teljes jobb oldali területet, hogy a Q és M szövegek ne fedjék egymást.

        tft.setFreeFont();
        tft.setTextColor(tft.color565(170, 220, 255), panelBg);
        tft.setTextDatum(TR_DATUM); // jobbra igazítás
        tft.drawString(modeValue, x + w - 6, y + 5);
        tft.drawString(qualityValue, x + w - 6, y + 15);
    }

    tft.setFreeFont();
}

/**
 * @brief Sensor bar sprite előkészítése, ha még nincs létrehozva
 */
void ScreenMain::ensureSensorBarSpriteReady() {

    // Ha a sprite már létezik, ellenőrizzük, hogy a mérete megfelelő-e
    if (hudState.sensorBarSpriteReady) {
        return;
    }

    // Sprite legyártása, ha még nem létezik
    sensorBarSprite.setColorDepth(16); // A Sprite háttere vegye fel a képernyő hátterének színét, így a sprite átlátszó lesz a háttér előtt.
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

    drawHudPanel(TRACK_X, TRACK_Y, TRACK_W, TRACK_H, "Sat", "--", TFT_DARKGREY);
    drawHudPanel(TIME_X, TIME_Y, TIME_W, TIME_H, "Local Time", "--:--", TFT_DARKGREY);
    drawHudPanel(PREC_X, PREC_Y, PREC_W, PREC_H, "Altitude", "-- m", TFT_DARKGREY);

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

/**
 * @brief Sebesség widget szövegének és méretének frissítése a beállított font alapján
 * @note Ezt a függvényt akkor kell meghívni, amikor a sebesség widget szövegének fontja vagy mérete megváltozik, hogy a widget megfelelően illeszkedjen a kijelzőn.
 */
void ScreenMain::updateSpeedValueLayoutForFont() {
    constexpr int16_t HORIZONTAL_INSET = 3;
    constexpr int16_t PAD_X = 10;
    constexpr int16_t PAD_Y = 8;
    constexpr int16_t RESERVED_FOR_UNIT = 28;
    constexpr int16_t MIN_W = 96;
    constexpr int16_t MIN_H = 54;
    constexpr int16_t SPEED_INNER_W = SPEED_W - (HORIZONTAL_INSET * 2);

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
    desiredW = static_cast<int16_t>(clampf(desiredW, MIN_W, SPEED_INNER_W));
    desiredH = static_cast<int16_t>(clampf(desiredH, MIN_H, maxH));

    hudState.speedValueW = desiredW;
    hudState.speedValueH = desiredH;
    hudState.speedValueX = SPEED_X + HORIZONTAL_INSET + (SPEED_INNER_W - desiredW) / 2;
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

    // Sprite legyártása, ha még nem létezik
    speedSprite.setColorDepth(16); // A Sprite háttere vegye fel a képernyő hátterének színét, így a sprite átlátszó lesz a háttér előtt.
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
