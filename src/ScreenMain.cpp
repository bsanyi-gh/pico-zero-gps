#include <Arduino.h>
#include <algorithm>
#include <cmath>
#include <cstring>

#include "LinearMeter.h"
#include "ScreenMain.h"
#include "defines.h"

extern TraffipaxManager traffipaxManager;

namespace {

/**
 * @brief A Traffipax riasztás állapotának futásidejű adatai
 */
struct DemoDataSnapshot {
    uint8_t satCount;
    uint8_t satCountForUI;
    uint8_t fixQuality;
    uint8_t fixMode;
    bool locationValid;
    bool speedValid;
    bool altitudeValid;
    bool dateValid;
    bool timeValid;
    float speedKmph;
    float altitudeM;
    float hdop;
    float courseDeg;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

/**
 * @brief Demó mód runtime adatok snapshot struktúra
 * @param nowMs Az aktuális idő milliszekundumban
 */
DemoDataSnapshot buildDemoDataSnapshot(uint32_t nowMs) {
    static uint32_t lastSatMs = 0;
    static uint32_t lastAltMs = 0;
    static uint32_t lastSpeedMs = 0;
    static uint8_t demoSatCount = 8;
    static float demoAltitude = 180.0f;
    static float demoSpeed = 45.0f;
    static float demoCourse = 0.0f;

    if (lastSatMs == 0 || (nowMs - lastSatMs) >= 2000UL) {
        demoSatCount = static_cast<uint8_t>(random(4, 16));
        lastSatMs = nowMs;
    }

    if (lastAltMs == 0 || (nowMs - lastAltMs) >= 3000UL) {
        demoAltitude = static_cast<float>(random(50, 2000));
        lastAltMs = nowMs;
    }

    if (lastSpeedMs == 0 || (nowMs - lastSpeedMs) >= 1500UL) {
        demoSpeed = static_cast<float>(random(0, 350));
        demoCourse += static_cast<float>(random(18, 42));
        while (demoCourse >= 360.0f) {
            demoCourse -= 360.0f;
        }
        lastSpeedMs = nowMs;
    }

    const uint32_t elapsedSec = nowMs / 1000UL;
    const uint32_t base = 17UL * 3600UL + 42UL * 60UL + 30UL + elapsedSec;

    DemoDataSnapshot demo{};
    demo.satCount = demoSatCount;
    demo.satCountForUI = demoSatCount;
    demo.fixQuality = 1;
    demo.fixMode = 3;
    demo.locationValid = true;
    demo.speedValid = true;
    demo.altitudeValid = true;
    demo.dateValid = true;
    demo.timeValid = true;
    demo.speedKmph = demoSpeed;
    demo.altitudeM = demoAltitude;
    demo.hdop = static_cast<float>(random(80, 450)) / 100.0f;
    demo.courseDeg = demoCourse;
    demo.year = 2026;
    demo.month = 7;
    demo.day = 2;
    demo.hour = static_cast<uint8_t>((base / 3600UL) % 24UL);
    demo.minute = static_cast<uint8_t>((base % 3600UL) / 60UL);
    demo.second = static_cast<uint8_t>(base % 60UL);
    return demo;
}

/**
 * @brief Runtime adatok snapshot struktúra
 */
struct RuntimeDataSnapshot {
    uint8_t satCount;
    uint8_t satCountForUI;
    uint8_t fixQuality;
    uint8_t fixMode;
    bool locationValid;
    bool speedValid;
    bool altitudeValid;
    bool dateValid;
    bool timeValid;
    float speedKmph;
    float altitudeM;
    float hdop;
    float courseDeg;
    uint32_t courseAgeMs;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    double lat;
    double lon;
    float voltageValue;
    float temperatureValue;
};

/**
 * @brief Valós idejű runtime adatok összegyűjtése a RuntimeDataSnapshot struktúrába
 * @return RuntimeDataSnapshot A valós idejű adatok snapshot-ja
 *
 */
RuntimeDataSnapshot collectLiveRuntimeData() {
    RuntimeDataSnapshot data{};
    data.satCount = c1_sharedGpsData.satelliteCount;
    data.satCountForUI = c1_sharedGpsData.satelliteCountForUI;
    data.fixQuality = c1_sharedGpsData.fixQuality;
    data.fixMode = c1_sharedGpsData.fixMode;
    data.locationValid = c1_sharedGpsData.locationValid;
    data.speedValid = c1_sharedGpsData.speedValid;
    data.altitudeValid = c1_sharedGpsData.altitudeValid;
    data.dateValid = c1_sharedGpsData.dateValid;
    data.timeValid = c1_sharedGpsData.timeValid;
    data.speedKmph = c1_sharedGpsData.speedKmph;
    data.altitudeM = c1_sharedGpsData.altitudeM;
    data.hdop = c1_sharedGpsData.hdop;
    data.courseDeg = c1_sharedGpsData.courseDeg;
    data.courseAgeMs = c1_sharedGpsData.courseAgeMs;
    data.year = c1_sharedGpsData.year;
    data.month = c1_sharedGpsData.month;
    data.day = c1_sharedGpsData.day;
    data.hour = c1_sharedGpsData.hour;
    data.minute = c1_sharedGpsData.minute;
    data.second = c1_sharedGpsData.second;
    data.lat = c1_sharedGpsData.lat;
    data.lon = c1_sharedGpsData.lng;
    data.voltageValue = config.data.externalVoltageMode ? c1_sharedSensorData.vBus : c1_sharedSensorData.vSys;
    data.temperatureValue = config.data.externalTemperatureMode ? c1_sharedSensorData.externalTemperature : c1_sharedSensorData.coreTemperature;
    return data;
}

/**
 * @brief Demó mód runtime adatok alkalmazása a RuntimeDataSnapshot struktúrára
 *
 * @param nowMs Az aktuális idő milliszekundumban
 * @param data A RuntimeDataSnapshot referencia, amire a demó adatokat alkalmazzuk
 * @param traffiManager A TraffipaxManager referencia, amivel a demó Traffipax koordinátákat kezeljük
 * @param alertController A TraffipaxAlertController referencia, amivel a demó Traffipax riasztás állapotát kezeljük
 *
 */
void applyDemoRuntimeData(uint32_t nowMs, RuntimeDataSnapshot &data, TraffipaxManager &traffiManager, TraffipaxAlertController &alertController) {
    const DemoDataSnapshot demo = buildDemoDataSnapshot(nowMs);

    data.satCount = demo.satCount;
    data.satCountForUI = demo.satCountForUI;
    data.fixQuality = demo.fixQuality;
    data.fixMode = demo.fixMode;
    data.locationValid = demo.locationValid;
    data.speedValid = demo.speedValid;
    data.altitudeValid = demo.altitudeValid;
    data.dateValid = demo.dateValid;
    data.timeValid = demo.timeValid;
    data.speedKmph = demo.speedKmph;
    data.altitudeM = demo.altitudeM;
    data.hdop = demo.hdop;
    data.courseDeg = demo.courseDeg;
    data.courseAgeMs = 0;
    data.year = demo.year;
    data.month = demo.month;
    data.day = demo.day;
    data.hour = demo.hour;
    data.minute = demo.minute;
    data.second = demo.second;

    if (!traffiManager.isDemoActive()) {
        traffiManager.startDemo();
        // Új demo ciklusnál töröljük a korábbi célpont/távolság állapotot.
        alertController.reset();
    }

    // Demó módban kizárólag a demo koordinátát használjuk a traffi logikához.
    data.locationValid = false;
    data.lat = 0.0;
    data.lon = 0.0;
    if (traffiManager.isDemoActive()) {
        double demoLat = 0.0;
        double demoLon = 0.0;
        traffiManager.processDemo();
        if (traffiManager.getDemoCoords(demoLat, demoLon)) {
            data.lat = demoLat;
            data.lon = demoLon;
            data.locationValid = true;
        }
    }

    // Demó szenzor értékek, hogy ne keveredjenek valós Core1 méréssel.
    static uint32_t lastDemoSensorMs = 0;
    static float demoVBus = 12.2f;
    static float demoVsys = 4.1f;
    static float demoExtTemp = 23.0f;
    static float demoCpuTemp = 38.0f;
    if (lastDemoSensorMs == 0 || (nowMs - lastDemoSensorMs) >= 2200UL) {
        demoVBus = 3.5f + static_cast<float>(random(0, 1201)) / 100.0f;
        demoVsys = 2.7f + static_cast<float>(random(0, 231)) / 100.0f;
        demoExtTemp = -15.5f + static_cast<float>(random(0, 811)) / 10.0f;
        demoCpuTemp = 20.0f + static_cast<float>(random(0, 601)) / 10.0f;
        lastDemoSensorMs = nowMs;
    }
    data.voltageValue = config.data.externalVoltageMode ? demoVBus : demoVsys;
    data.temperatureValue = config.data.externalTemperatureMode ? demoExtTemp : demoCpuTemp;
}
} // namespace

/**
 * @brief ScreenMain konstruktor
 */
ScreenMain::ScreenMain() : UIScreen(SCREEN_NAME_MAIN), speedSprite(&tft), sensorBarSprite(&tft), graphSprite(&tft), SENSOR_BAR_X_RIGHT(tft.width() - SENSOR_BAR_W) {

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
 * @brief UI komponensek elhelyezése a képernyőn
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

    this->trendGraphMode = static_cast<GraphMode>(config.data.trendGraphMode);

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
    std::strcpy(hudState.lastDateTimeText, "");
    std::strcpy(hudState.lastAltText, "");
    std::strcpy(hudState.lastBottomText, "");
    hudState.lastSatColor = 0;
    hudState.lastDateTimeColor = 0;
    hudState.lastAltColor = 0;
    hudState.lastAltPanelCompassMode = false;
    hudState.lastCompassNorthDeg = -1000.0f;
    speedDisplayMoving = false;
    compassHasKnownCourse = false;
    compassLastKnownCourseDeg = 0.0f;
    traffipaxAlertController.reset();
    graphDirty = true;

    // Beállítjuk a kényszerített újrarajzolás flag-et
    this->forceRedraw = true;

    // a következő ciklusban kényszerítjük az újrarajzolást
    markForRedraw(true); // a képernyőt és a gyerekeit  újrarajzolásra jelöljük

    // Ős activate() metódus hívása
    UIScreen::activate();
}

/**
 * @brief Kezeli a képernyő saját ciklusát (dinamikus frissítés)
 */
void ScreenMain::handleOwnLoop() {

    uint32_t nowMs = millis();
    static bool lastDemoMode = demoMode;

    RuntimeDataSnapshot data = collectLiveRuntimeData();

    if (lastDemoMode != demoMode) {
        // Módváltáskor teljes HUD cache reset, hogy ne maradjon régi (demo/valós) tartalom a képernyőn.
        hudState.staticPainted = false;
        hudState.lastRedrawMs = 0;
        std::strcpy(hudState.lastSatText, "");
        std::strcpy(hudState.lastTrackMetaText, "");
        std::strcpy(hudState.lastDateTimeText, "");
        std::strcpy(hudState.lastAltText, "");
        std::strcpy(hudState.lastBottomText, "");
        hudState.lastSatColor = 0;
        hudState.lastDateTimeColor = 0;
        hudState.lastAltColor = 0;
        hudState.lastAltPanelCompassMode = false;
        hudState.lastCompassNorthDeg = -1000.0f;
        maxSpeedSinceBoot = 0.0f;
        speedDisplayMoving = false;
        compassHasKnownCourse = false;
        compassLastKnownCourseDeg = 0.0f;
        graphSampleCount = 0;
        graphDirty = true;
        traffipaxAlertController.reset();
        forceRedraw = true;
        markForRedraw(true);

        if (demoMode) {
            traffipaxManager.startDemo();
        }

        lastDemoMode = demoMode;
    }

    if (demoMode) {
        applyDemoRuntimeData(nowMs, data, traffipaxManager, traffipaxAlertController);
    }

    const uint8_t dataSatCount = data.satCount;
    const uint8_t dataSatCountForUI = data.satCountForUI;
    const uint8_t dataFixQuality = data.fixQuality;
    const uint8_t dataFixMode = data.fixMode;
    const bool dataLocationValid = data.locationValid;
    const bool dataSpeedValid = data.speedValid;
    const bool dataAltitudeValid = data.altitudeValid;
    const bool dataDateValid = data.dateValid;
    const bool dataTimeValid = data.timeValid;
    const float dataSpeedKmph = data.speedKmph;
    const float dataAltitudeM = data.altitudeM;
    const float dataHdop = data.hdop;
    const float dataCourseDeg = data.courseDeg;
    const uint32_t dataCourseAgeMs = data.courseAgeMs;
    const uint16_t dataYear = data.year;
    const uint8_t dataMonth = data.month;
    const uint8_t dataDay = data.day;
    const uint8_t dataHour = data.hour;
    const uint8_t dataMinute = data.minute;
    const double dataLat = data.lat;
    const double dataLon = data.lon;
    const float dataVoltageValue = data.voltageValue;
    const float dataTemperatureValue = data.temperatureValue;

    // Traffipax közeledés / távolodás figyelmeztetés frissítése minden ciklusban.
    const TraffipaxAlertController::ConfigSnapshot traffiCfg{config.data.gpsTraffiAlarmEnabled, config.data.gpsTraffiSirenAlarmEnabled, config.data.beeperEnabled, config.data.gpsTraffiAlarmDistance};
    const auto traffiResult = traffipaxAlertController.update(dataLat, dataLon, dataLocationValid, traffiCfg, nowMs, tft, traffipaxManager);
    if (traffiResult.baseAreaNeedsRestore) {
        drawTraffipaxBaseArea();
    }
    if (traffiResult.hudNeedsRepaint) {
        hudState.staticPainted = false;
        hudState.lastRedrawMs = 0;
        forceRedraw = true;
    }

    // A sebesség értékének lekérése a GPS adatokból
    const bool speedValid = dataSpeedValid;
    const float rawTargetSpeed = speedValid ? dataSpeedKmph : 0.0f;
    float targetSpeed = (rawTargetSpeed < 0.0f) ? 0.0f : rawTargetSpeed;

    // Schmitt-trigger hiszterézis a stabil 0->5 km/h átmenethez.
    if (!speedValid) {
        speedDisplayMoving = false;
        targetSpeed = 0.0f;
    } else {
        if (!speedDisplayMoving && targetSpeed >= SPEED_DISPLAY_ON_THRESHOLD_KMPH) {
            speedDisplayMoving = true;
        } else if (speedDisplayMoving && targetSpeed <= SPEED_DISPLAY_OFF_THRESHOLD_KMPH) {
            speedDisplayMoving = false;
        }

        if (!speedDisplayMoving) {
            targetSpeed = 0.0f;
        }
    }

    if (!Utils::timeHasPassed(hudState.lastRedrawMs, HUD_UPDATE_INTERVAL_MS)) {
        return;
    }

    hudState.lastRedrawMs = nowMs;

    const bool forceUpdate = forceRedraw;
    forceRedraw = false;
    hudState.lastSpeedValue = targetSpeed;

    const float speedKmph = (targetSpeed < 0.0f) ? 0.0f : targetSpeed;
    if (speedValid && speedKmph > maxSpeedSinceBoot) {
        maxSpeedSinceBoot = speedKmph;
    }

    // Mintavétel a trend grafikonhoz
    recordGraphSample(speedKmph, speedValid, dataAltitudeM, dataAltitudeValid, hudState.lastRedrawMs);

    if (!traffiResult.alertActive) {
        // Műholdak száma + fix adatok
        char satText[24];
        snprintf(satText, sizeof(satText), "%u / %u", dataSatCount, dataSatCountForUI);

        // Műholdak minősége és fix mód
        char trackMetaText[64];
        snprintf(trackMetaText, sizeof(trackMetaText), "Q:%s|M:%s", GpsManager::qualityToString(dataFixQuality).c_str(), GpsManager::modeToString(dataFixMode).c_str());
        const uint16_t satColor = dataLocationValid ? TFT_GREEN : TFT_DARKGREY;
        const bool satNeedsUpdate = std::strcmp(satText, hudState.lastSatText) != 0 || satColor != hudState.lastSatColor;
        const bool trackMetaNeedsUpdate = std::strcmp(trackMetaText, hudState.lastTrackMetaText) != 0;
        if (satNeedsUpdate || trackMetaNeedsUpdate) {
            const String quality = "Q: " + GpsManager::qualityToString(dataFixQuality);
            const String mode = "M: " + GpsManager::modeToString(dataFixMode);

            drawTrackPanelValue(SAT_X, SAT_Y, SAT_W, SAT_H, satText, quality.c_str(), mode.c_str(), satColor, satNeedsUpdate, trackMetaNeedsUpdate);

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

        // Dátum/Idő
        char dateTimeText[24];
        if (config.data.dateTimeModeDate) {
            if (dataDateValid) {
                snprintf(dateTimeText, sizeof(dateTimeText), "%04u.%02u.%02u", dataYear, dataMonth, dataDay);
            } else {
                snprintf(dateTimeText, sizeof(dateTimeText), "----.--.--");
            }
        } else {
            if (dataTimeValid) {
                snprintf(dateTimeText, sizeof(dateTimeText), "%02u:%02u", dataHour, dataMinute);
            } else {
                snprintf(dateTimeText, sizeof(dateTimeText), "--:--");
            }
        }
        const uint16_t dateTimeColor = config.data.dateTimeModeDate ? (dataDateValid ? TFT_CYAN : TFT_DARKGREY) : (dataTimeValid ? TFT_CYAN : TFT_DARKGREY);
        if (std::strcmp(dateTimeText, hudState.lastDateTimeText) != 0 || dateTimeColor != hudState.lastDateTimeColor) {
            drawHudPanelValue(DATETIME_X, DATETIME_Y, DATETIME_W, DATETIME_H, dateTimeText, dateTimeColor);

            std::strncpy(hudState.lastDateTimeText, dateTimeText, sizeof(hudState.lastDateTimeText) - 1);
            hudState.lastDateTimeText[sizeof(hudState.lastDateTimeText) - 1] = '\0';
            hudState.lastDateTimeColor = dateTimeColor;
        }

        // Jobb felső panel: magasság vagy iránytű
        if (!config.data.altitudeCompassMode) {
            // Magasság mód: a magasság értékének kirajzolása
            char altText[24];
            if (dataAltitudeValid) {
                snprintf(altText, sizeof(altText), "%d", static_cast<int>(std::lroundf(dataAltitudeM)));
            } else {
                snprintf(altText, sizeof(altText), "--");
            }

            const uint16_t altColor = dataAltitudeValid ? TFT_CYAN : TFT_DARKGREY;
            if (forceUpdate || hudState.lastAltPanelCompassMode || std::strcmp(altText, hudState.lastAltText) != 0 || altColor != hudState.lastAltColor) {
                drawAltitudePanelValue(ALT_X, ALT_Y, ALT_W, ALT_H, altText, altColor);
                std::strncpy(hudState.lastAltText, altText, sizeof(hudState.lastAltText) - 1);
                hudState.lastAltText[sizeof(hudState.lastAltText) - 1] = '\0';
                hudState.lastAltColor = altColor;
                hudState.lastAltPanelCompassMode = false;
            }
        } else {
            // Iránytű mód: a haladási irányt mutató nyíl kirajzolása
            const bool hasFreshCourse = (dataCourseAgeMs < GPS_DATA_MAX_AGE);
            float courseDeg = dataCourseDeg;
            if (!std::isfinite(courseDeg)) {
                courseDeg = 0.0f;
            }
            courseDeg = std::fmod(courseDeg, 360.0f);
            if (courseDeg < 0.0f) {
                courseDeg += 360.0f;
            }

            const bool hasUsableCourse = dataLocationValid && dataSpeedValid && (courseDeg >= 0.0f) && (courseDeg <= 360.0f);
            if (hasUsableCourse) {
                compassHasKnownCourse = true;
                compassLastKnownCourseDeg = courseDeg;
            }

            // Északot mutató nyíl a haladási irányhoz képest: 0°=felfelé, 90°=jobbra.
            const float displayCourseDeg = compassHasKnownCourse ? compassLastKnownCourseDeg : 0.0f;
            const float northAngleDeg = (displayCourseDeg == 0.0f) ? 0.0f : (360.0f - displayCourseDeg);
            const uint16_t compassColor = (hasFreshCourse && hasUsableCourse) ? TFT_CYAN : tft.color565(95, 115, 130);

            if (forceUpdate || !hudState.lastAltPanelCompassMode || std::fabs(northAngleDeg - hudState.lastCompassNorthDeg) >= 0.2f || compassColor != hudState.lastAltColor) {
                drawCompassPanelValue(ALT_X, ALT_Y, ALT_W, ALT_H, northAngleDeg, hasFreshCourse && hasUsableCourse, compassColor);
                hudState.lastCompassNorthDeg = northAngleDeg;
                hudState.lastAltColor = compassColor;
                hudState.lastAltPanelCompassMode = true;
            }
        }
    }

    // Sebesség widget kirajzolása ugyanazzal az 500 ms HUD ciklussal.
    drawSpeedWidget(targetSpeed, speedValid, forceUpdate);

    // Trend grafikon kirajzolása a külön sávba.
    drawTrendGraph(forceUpdate);

    // Függőleges bar-ok adatai
    const float voltageValue = dataVoltageValue;
    const float temperatureValue = dataTemperatureValue;

    // Ellenőrizzük, hogy szükséges-e a függőleges bar-ok frissítése
    const bool voltageNeedsUpdate = forceUpdate || config.data.externalVoltageMode != hudState.lastVoltageMode || std::fabs(voltageValue - hudState.lastVoltageValue) > 0.02f;
    const bool temperatureNeedsUpdate = forceUpdate || config.data.externalTemperatureMode != hudState.lastTemperatureMode || std::fabs(temperatureValue - hudState.lastTemperatureValue) > 0.05f;
    const bool anyBarNeedsUpdate = voltageNeedsUpdate || temperatureNeedsUpdate;

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
                                20,                                                                      // bar-w
                                8,                                                                       // bar-h
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
                                20,                                                          // bar-w
                                8,                                                           // bar-h
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

    if (!traffiResult.alertActive) {
        // Alsó információs sor: fix oszlopokban balra igazított HDOP és MAX érték.
        char hdopValueText[16];
        const bool hdopValid = (dataHdop > 0.0f && dataHdop < 99.0f);
        if (hdopValid) {
            snprintf(hdopValueText, sizeof(hdopValueText), "%.1f", dataHdop);
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
    }
}

/**
 * @brief Touch esemény kezelése - hőmérsékleti mód váltás
 *
 * @param event A touch esemény, amely tartalmazza a koordinátákat és a lenyomás állapotát.
 * @return true, ha a touch eseményt kezelte a képernyő, false egyébként
 */
bool ScreenMain::handleTouch(const TouchEvent &event) {

    // Csak lenyomás eseményre reagálunk
    if (!event.pressed) {
        return UIScreen::handleTouch(event); // Továbbítjuk az ősosztálynak
    }

    // Trend grafikon sáv: Off -> Speed -> Altitude -> Off.
    if (event.x >= GRAPH_X && event.x < GRAPH_X + GRAPH_W && event.y >= GRAPH_Y && event.y < GRAPH_Y + GRAPH_H) {
        switch (trendGraphMode) {
            case GraphMode::Off:
                trendGraphMode = GraphMode::Speed;
                break;
            case GraphMode::Speed:
                trendGraphMode = GraphMode::Altitude;
                break;
            case GraphMode::Altitude:
                trendGraphMode = GraphMode::Off;
                break;
        }
        config.data.trendGraphMode = static_cast<uint8_t>(this->trendGraphMode);
        config.checkSave(); // A módosítás mentése a flash-be
        graphDirty = true;
        forceRedraw = true;
        hudState.lastRedrawMs = 0;
        if (config.data.beeperEnabled) {
            Utils::beepTick();
        }
        return true;
    }

    // Sat panel érintésre Satellite képernyőre váltunk.
    if (event.x >= SAT_X && event.x < SAT_X + SAT_W && event.y >= SAT_Y && event.y < SAT_Y + SAT_H) {
        if (getScreenManager()) {
            getScreenManager()->switchToScreen(SCREEN_NAME_SATS);
        }
        if (config.data.beeperEnabled) {
            Utils::beepTick();
        }
        return true;
    }

    // Dátum/Idő panel érintésre módosítjuk a kijelzett értéket.
    if (event.x >= DATETIME_X && event.x < DATETIME_X + DATETIME_W && event.y >= DATETIME_Y && event.y < DATETIME_Y + DATETIME_H) {
        config.data.dateTimeModeDate = !config.data.dateTimeModeDate;
        config.checkSave(); // A módosítás mentése a flash-be
        // A DateTime panel újrarajzolása a következő ciklusban
        drawHudPanel(DATETIME_X, DATETIME_Y, DATETIME_W, DATETIME_H, config.data.dateTimeModeDate ? "Date" : "Time", config.data.dateTimeModeDate ? "--.--.--" : "--:--", TFT_DARKGREY);
        hudState.lastRedrawMs = 0; // A következő ciklusban kényszerítjük az újrarajzolást
        if (config.data.beeperEnabled) {
            Utils::beepTick();
        }
        return true;
    }

    // Jobb felső panel érintésre váltás: magasság <-> iránytű
    if (event.x >= ALT_X && event.x < ALT_X + ALT_W && event.y >= ALT_Y && event.y < ALT_Y + ALT_H) {
        config.data.altitudeCompassMode = !config.data.altitudeCompassMode;
        config.checkSave(); // A módosítás mentése a flash-be

        drawHudPanel(ALT_X, ALT_Y, ALT_W, ALT_H, config.data.altitudeCompassMode ? "Compass" : "Altitude", "--", TFT_DARKGREY);
        forceRedraw = true;
        hudState.lastRedrawMs = 0;
        hudState.lastAltPanelCompassMode = config.data.altitudeCompassMode;
        hudState.lastCompassNorthDeg = -1000.0f;
        compassHasKnownCourse = false;
        compassLastKnownCourseDeg = 0.0f;
        std::strcpy(hudState.lastAltText, "");
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

    // Ha nem volt találat, továbbítjuk az ősosztálynak a touch eseményt
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

    constexpr int16_t TRAFFI_ALERT_H = TraffipaxAlertController::ALERT_DRAW_HEIGHT;
    for (int16_t y = 0; y < TRAFFI_ALERT_H; y++) {
        const uint16_t bg = tft.color565(0, 40 + (y * 60) / tft.height(), 80 + (y * 100) / tft.height());
        tft.drawFastHLine(0, y, tft.width(), bg);
    }

    // A top HUD elemei, amelyek az alert sáv alatt is látszanak, ha nincs riasztás
    drawHudPanel(SAT_X, SAT_Y, SAT_W, SAT_H, "Sat", "--", TFT_DARKGREY);
    drawHudPanel(DATETIME_X, DATETIME_Y, DATETIME_W, DATETIME_H, config.data.dateTimeModeDate ? "Date" : "Time", config.data.dateTimeModeDate ? "--.--.--" : "--:--", TFT_DARKGREY);
    drawHudPanel(ALT_X, ALT_Y, ALT_W, ALT_H, config.data.altitudeCompassMode ? "Compass" : "Altitude", "--", TFT_DARKGREY);
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
    tft.setTextPadding(0);

    // Panel háttér kirajzolása lekerekített sarkokkal
    tft.fillRoundRect(x, y, w, h, 6, panelBg);
    // A keret legyen a panel szélén, ezért a fillRoundRect után rajzoljuk ki.
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
    tft.setTextPadding(0);
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
 *
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
    tft.setTextPadding(0);

    // Szűkebb törlési sáv a jobb oldali keret védelméhez.
    const int16_t valueAreaX = x + 4;
    const int16_t valueAreaY = y + 17;
    const int16_t valueAreaW = w - 12;
    const int16_t valueAreaH = h - 19;

    // A korábbi panel érték törlése
    tft.fillRect(valueAreaX, valueAreaY, valueAreaW, valueAreaH, panelBg);

    // Nagy számérték kirajzolása
    tft.setFreeFont(&FreeSansBold9pt7b);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(valueColor, panelBg);
    tft.drawString(value, x + 6, y + 20);

    // Kisebb mértékegység (m) utána
    const int16_t valueW = tft.textWidth(value);
    tft.setFreeFont();
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, panelBg);
    const int16_t unitX = std::min<int16_t>(x + 6 + valueW + 4, x + w - 12);
    tft.drawString("m", unitX, y + 26);

    tft.setFreeFont();
}

/**
 * @brief Iránytű panel érték kirajzolása északot mutató nyíllal
 *
 * @param x A panel bal felső sarkának X koordinátája
 * @param y A panel bal felső sarkának Y koordinátája
 * @param w A panel szélessége
 * @param h A panel magassága
 * @param northAngleDeg Az északi irány szöge fokban (0°=felfelé)
 * @param hasValidHeading true, ha érvényes az irányadat
 * @param valueColor Az iránytű színe
 */
void ScreenMain::drawCompassPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, float northAngleDeg, bool /*hasValidHeading*/, uint16_t valueColor) {

    const uint16_t panelBg = tft.color565(8, 16, 26);
    tft.setTextPadding(0);

    // Beljebb törlünk/rajzolunk, hogy a lekerekített keret széleit biztosan ne érintsük.
    const int16_t valueAreaX = x + 4;
    const int16_t valueAreaY = y + 18;
    const int16_t valueAreaW = w - 12;
    const int16_t valueAreaH = h - 22;
    tft.fillRect(valueAreaX, valueAreaY, valueAreaW, valueAreaH, panelBg);

    const int16_t cx = valueAreaX + (valueAreaW / 2);
    const int16_t cy = valueAreaY + (valueAreaH / 2);
    const int16_t radius = std::max<int16_t>(5, (std::min<int16_t>(valueAreaW, valueAreaH) / 2) - 2);

    const uint16_t arrowColor = valueColor;
    const float angleRad = northAngleDeg * static_cast<float>(DEG_TO_RAD);
    const float ux = std::sin(angleRad);
    const float uy = -std::cos(angleRad);
    const float px = -uy;
    const float py = ux;

    const int16_t tipX = cx + static_cast<int16_t>(std::lroundf(ux * (radius - 1)));
    const int16_t tipY = cy + static_cast<int16_t>(std::lroundf(uy * (radius - 1)));
    const int16_t tailX = cx - static_cast<int16_t>(std::lroundf(ux * (radius - 1)));
    const int16_t tailY = cy - static_cast<int16_t>(std::lroundf(uy * (radius - 1)));

    // Középre igazított, vastagabb nyélszár a jobb olvashatóságért.
    for (int8_t i = -1; i <= 1; i++) {
        const int16_t ox = static_cast<int16_t>(std::lroundf(px * i));
        const int16_t oy = static_cast<int16_t>(std::lroundf(py * i));
        tft.drawLine(tailX + ox, tailY + oy, tipX + ox, tipY + oy, arrowColor);
    }
    tft.fillCircle(tipX, tipY, 1, arrowColor);

    const int16_t headBack = std::max<int16_t>(3, radius / 2);
    const int16_t headHalfWidth = std::max<int16_t>(3, radius / 2);
    const int16_t leftX = tipX - static_cast<int16_t>(std::lroundf(ux * headBack - px * headHalfWidth));
    const int16_t leftY = tipY - static_cast<int16_t>(std::lroundf(uy * headBack - py * headHalfWidth));
    const int16_t rightX = tipX - static_cast<int16_t>(std::lroundf(ux * headBack + px * headHalfWidth));
    const int16_t rightY = tipY - static_cast<int16_t>(std::lroundf(uy * headBack + py * headHalfWidth));
    tft.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, arrowColor);
}

/**
 * @brief Ellenőrzi, hogy a trend grafikon sprite létrejött-e, és ha nem, létrehozza azt
 */
void ScreenMain::ensureGraphSpriteReady() {
    if (hudState.graphSpriteReady) {
        return;
    }

    // graphSprite.setColorDepth(16);
    hudState.graphSpriteReady = graphSprite.createSprite(GRAPH_W, GRAPH_H) != nullptr;
}

/**
 * @brief Trend grafikon minták rögzítése
 *
 * @param speedKmph A sebesség km/h-ban
 * @param speedValid true, ha a sebesség érvényes, false ha nem
 * @param altitudeM A magasság méterben
 * @param altitudeValid true, ha a magasság érvényes, false ha nem
 * @param nowMs Az aktuális idő milliszekundumban
 *
 */
void ScreenMain::recordGraphSample(float speedKmph, bool speedValid, float altitudeM, bool altitudeValid, uint32_t nowMs) {

    static uint32_t graphLastSampleMs = 0;
    if (graphSampleCount > 0 && !Utils::timeHasPassed(graphLastSampleMs, TREND_GRAPH_SAMPLE_INTERVAL_MS)) {
        return;
    }

    // Minták rögzítése, ha érvényesek, különben GRAPH_SAMPLE_INVALID érték kerül a minták közé.
    const int16_t speedSample = speedValid ? static_cast<int16_t>(std::lroundf(speedKmph)) : GRAPH_SAMPLE_INVALID;
    const int16_t altitudeSample = altitudeValid ? static_cast<int16_t>(std::lroundf(altitudeM)) : GRAPH_SAMPLE_INVALID;

    // Minták tárolása a körkörös bufferben
    if (graphSampleCount < GRAPH_W) {
        speedGraphSamples[graphSampleCount] = speedSample;
        altitudeGraphSamples[graphSampleCount] = altitudeSample;
        graphSampleCount++;
    } else {
        // Körkörös buffer frissítése, ha a buffer megtelt
        std::memmove(speedGraphSamples, speedGraphSamples + 1, sizeof(speedGraphSamples) - sizeof(speedGraphSamples[0]));
        std::memmove(altitudeGraphSamples, altitudeGraphSamples + 1, sizeof(altitudeGraphSamples) - sizeof(altitudeGraphSamples[0]));
        speedGraphSamples[GRAPH_W - 1] = speedSample;
        altitudeGraphSamples[GRAPH_W - 1] = altitudeSample;
    }

    graphLastSampleMs = nowMs;
    graphDirty = true;
}

/**
 * @brief Trend grafikon kirajzolása a kijelzőre
 *
 * @param forceUpdate true, ha kényszeríteni kell az újrarajzolást, false ha csak akkor rajzoljon, ha a grafikon "piszkos" (dirty)
 */
void ScreenMain::drawTrendGraph(bool forceUpdate) {

    // Ha nincs kényszerített frissítés és a grafikon nem "piszkos", akkor nem rajzolunk újra
    if (!forceUpdate && !graphDirty) {
        return;
    }

    ensureGraphSpriteReady();
    if (!hudState.graphSpriteReady) {
        return;
    }

    // Grafikon háttér színének beállítása és kitöltése
    const uint16_t bg = graphSprite.color565(28, 30, 32);
    graphSprite.fillSprite(bg);

    // Grafikon rácsvonalak színei
    const uint16_t gridColor = graphSprite.color565(70, 70, 70);
    // Grafikon alapvonal színe
    const uint16_t baselineColor = graphSprite.color565(120, 120, 120);

    const uint16_t titleColor = graphSprite.color565(150, 210, 240);
    const uint16_t mutedTextColor = graphSprite.color565(90, 130, 160);

    auto bgColorForGlobalY = [&](int16_t globalY) -> uint16_t { return graphSprite.color565(0, 10 + (globalY * 22) / tft.height(), 18 + (globalY * 34) / tft.height()); };

    // Grafikon háttér kirajzolása
    for (int16_t localY = 0; localY < GRAPH_H; localY++) {
        const int16_t globalY = GRAPH_Y + localY;
        graphSprite.drawFastHLine(0, localY, GRAPH_W, bgColorForGlobalY(globalY));
    }

    // Függőleges rácsvonalak kirajzolása 20 pixelenként
    for (int16_t globalX = 0; globalX < GRAPH_W; globalX += 20) {
        graphSprite.drawFastVLine(globalX, 0, GRAPH_H, gridColor);
    }

    // Vízszintes rácsvonalak kirajzolása 12 és 26 pixelenként, valamint az alsó alapvonal
    graphSprite.drawFastHLine(0, 14, GRAPH_W, gridColor);
    graphSprite.drawFastHLine(0, 26, GRAPH_W, gridColor);
    graphSprite.drawFastHLine(0, GRAPH_H - 2, GRAPH_W, baselineColor);

    graphSprite.setFreeFont();
    graphSprite.setTextSize(1);
    graphSprite.setTextDatum(TL_DATUM);

    if (this->trendGraphMode == GraphMode::Off) {
        graphSprite.setTextColor(mutedTextColor, bgColorForGlobalY(GRAPH_Y + 2));
        graphSprite.drawString("Trend: OFF", 4, 2);
        graphSprite.drawString("tap to cycle", 4, 20);
        graphSprite.pushSprite(GRAPH_X, GRAPH_Y);
        graphDirty = false;
        return;
    }

    // A kirajzolni kívánt minták kiválasztása a grafikon módjától függően
    const int16_t *samples = (this->trendGraphMode == GraphMode::Speed) ? speedGraphSamples : altitudeGraphSamples;
    const uint16_t plotColor = (this->trendGraphMode == GraphMode::Speed) ? graphSprite.color565(255, 220, 60) : graphSprite.color565(80, 230, 255);
    const char *title = (this->trendGraphMode == GraphMode::Speed) ? "Speed" : "Altitude";

    int16_t maxValue = 0;
    for (uint16_t i = 0; i < graphSampleCount; i++) {
        if (samples[i] != GRAPH_SAMPLE_INVALID && samples[i] > maxValue) {
            maxValue = samples[i];
        }
    }

    graphSprite.setTextColor(titleColor, bgColorForGlobalY(GRAPH_Y + 2));
    graphSprite.drawString(title, 4, 2);

    graphSprite.setTextDatum(TR_DATUM);
    char maxText[24];
    if (this->trendGraphMode == GraphMode::Speed) {
        snprintf(maxText, sizeof(maxText), "max %d km/h", maxValue);
    } else {
        snprintf(maxText, sizeof(maxText), "max %d m", maxValue);
    }
    graphSprite.drawString(maxText, GRAPH_W - 5, 2);
    graphSprite.setTextDatum(TL_DATUM);

    // Ha nincs érvényes minta, akkor a "collecting samples" üzenet jelenik meg
    if (graphSampleCount == 0 || maxValue <= 0) {
        graphSprite.setTextColor(mutedTextColor, bgColorForGlobalY(GRAPH_Y + 16));
        graphSprite.drawString("collecting " + String(TREND_GRAPH_SAMPLE_INTERVAL_MS / 1000) + " secs samples", 4, 20);
        graphSprite.pushSprite(GRAPH_X, GRAPH_Y);
        graphDirty = false;
        return;
    }

    const int16_t scaleMax = maxValue + std::max<int16_t>(1, maxValue / 10);
    const int16_t chartTop = 12;
    const int16_t chartBottom = GRAPH_H - 3;
    const int16_t chartHeight = chartBottom - chartTop;
    const int16_t startX = GRAPH_W - graphSampleCount;

    int16_t prevX = -1;
    int16_t prevY = -1;
    for (uint16_t i = 0; i < graphSampleCount; i++) {
        const int16_t sample = samples[i];
        if (sample == GRAPH_SAMPLE_INVALID) {
            prevX = -1;
            prevY = -1;
            continue;
        }

        const int16_t x = startX + static_cast<int16_t>(i);
        const int16_t y = chartBottom - static_cast<int16_t>((static_cast<int32_t>(sample) * chartHeight) / scaleMax);

        // Kitöltés színe a görbe alatt, a grafikon módjától függően
        const uint16_t fillColor = (this->trendGraphMode == GraphMode::Speed) ? graphSprite.color565(120, 90, 20) : graphSprite.color565(20, 90, 120);
        //
        if (prevX >= 0) { // Ha van előző pont, akkor a kitöltést és a görbét is rajzoljuk

            // Kitöltés a görbe alatt
            graphSprite.drawLine(prevX, chartBottom, prevX, prevY, fillColor);
            graphSprite.drawLine(x, chartBottom, x, y, fillColor);

            // Görbe
            graphSprite.drawLine(prevX, prevY, x, y, plotColor);

            // Ha a görbe pixelben van, akkor a következő pixel sorban is rajzolunk egy vonalat, hogy vastagabb legyen a görbe
            if (y + 1 < GRAPH_H) {
                graphSprite.drawLine(prevX, prevY + 1, x, y + 1, plotColor);
            }

        } else { // Ha nincs előző pont, akkor csak a kitöltést rajzoljuk

            // Kitöltés a görbe alatt
            graphSprite.drawLine(x, chartBottom, x, y, fillColor);
            // Ha a görbe pixelben van, akkor a következő pixel sorban is rajzolunk egy vonalat, hogy vastagabb legyen a görbe
            graphSprite.drawPixel(x, y, plotColor);

            if (y + 1 < GRAPH_H) {
                graphSprite.drawPixel(x, y + 1, plotColor);
            }
        }

        prevX = x;
        prevY = y;
    }

    graphSprite.pushSprite(GRAPH_X, GRAPH_Y);
    graphDirty = false;
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
    tft.setTextPadding(0);

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
 *
 * @note Ezt a függvényt csak egyszer kell meghívni, amikor a képernyő először aktiválódik,
 * vagy amikor a képernyő teljes újrarajzolása szükséges.
 *
 */
void ScreenMain::drawStaticHudBackground() {

    // Háttér színátmenet kirajzolása
    for (int16_t y = 0; y < tft.height(); y++) {
        const uint16_t bg = tft.color565(0, 10 + (y * 22) / tft.height(), 18 + (y * 34) / tft.height());
        tft.drawFastHLine(0, y, tft.width(), bg);
    }

    // Függőleges rácsvonalak kirajzolása 20 pixelenként
    for (int16_t x = 0; x < tft.width(); x += 20) {
        tft.drawFastVLine(x, 0, tft.height(), tft.color565(0, 18, 26));
    }

    // Alsó információs sor panel
    drawHudPanel(SAT_X, SAT_Y, SAT_W, SAT_H, "Sat", "--", TFT_DARKGREY);
    drawHudPanel(DATETIME_X, DATETIME_Y, DATETIME_W, DATETIME_H, config.data.dateTimeModeDate ? "Date" : "Time", config.data.dateTimeModeDate ? "--.--.--" : "--:--", TFT_DARKGREY);
    drawHudPanel(ALT_X, ALT_Y, ALT_W, ALT_H, config.data.altitudeCompassMode ? "Compass" : "Altitude", "--", TFT_DARKGREY);

    // Sebesség widget szövegének és méretének frissítése a beállított font alapján
    updateSpeedValueLayoutForFont();

    // Statikus sebesség mértékegység felirat
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont();
    tft.setTextSize(2);
    tft.setTextColor(tft.color565(120, 220, 255));
    const int16_t kmhY = hudState.speedValueY + hudState.speedValueH + 12;
    tft.drawString("km/h", SPEED_X + (SPEED_W / 2), kmhY <= (SPEED_Y + SPEED_H - 8) ? kmhY : SPEED_UNIT_BASELINE_Y);

    // Alsó információs sor háttér kirajzolása
    tft.fillRoundRect(INFO_X, INFO_Y, INFO_W, INFO_H, 6, tft.color565(6, 14, 22));
    tft.drawRoundRect(INFO_X, INFO_Y, INFO_W, INFO_H, 6, tft.color565(0, 132, 214));
}

/**
 * @brief Sebesség widget szövegének és méretének frissítése a beállított font alapján
 *
 * @note Ezt a függvényt akkor kell meghívni, amikor a sebesség widget szövegének fontja vagy mérete megváltozik,
 * hogy a widget megfelelően illeszkedjen a kijelzőn.
 */
void ScreenMain::updateSpeedValueLayoutForFont() {
    constexpr int16_t RESERVED_FOR_UNIT = 28;
    constexpr int16_t MIN_W = 96;
    constexpr int16_t MIN_H = 54;
    constexpr int16_t SPEED_INNER_W = SPEED_W;

    // A sprite tényleges fontbeállításával mérünk, így a méret mindig konzisztens.
    speedSprite.setTextFont(SPEED_VALUE_FONT);
    speedSprite.setTextSize(SPEED_VALUE_TEXT_SIZE);
    int16_t textW = speedSprite.textWidth("888");
    int16_t textH = speedSprite.fontHeight();

    if (textW <= 0) {
        textW = MIN_W;
    }
    if (textH <= 0) {
        textH = MIN_H;
    }

    int16_t desiredW = textW;
    int16_t desiredH = textH;

    const int16_t maxH = SPEED_H - RESERVED_FOR_UNIT;
    desiredW = static_cast<int16_t>(std::clamp(desiredW, MIN_W, SPEED_INNER_W));
    desiredH = static_cast<int16_t>(std::clamp(desiredH, MIN_H, maxH));

    hudState.speedValueW = desiredW;
    hudState.speedValueH = desiredH;
    hudState.speedValueX = SPEED_X + (SPEED_INNER_W - desiredW) / 2;
    hudState.speedValueY = SPEED_Y + ((maxH - desiredH) / 2);
}

/**
 * @brief Sebesség widgethez szükséges sprite inicializálása, ha még nem történt meg
 */
void ScreenMain::ensureSpeedSpriteReady() {

    // A sebesség widget szövegének és méretének frissítése a beállított font alapján
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
 *
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

        const uint16_t glowColor = speedSprite.color565(0, 120, 180);
        const int16_t cx = hudState.speedValueW / 2;
        const int16_t cy = hudState.speedValueH / 2;

        // Glow effekt kirajzolása a sebesség szám köré
        const int8_t offsets[][2] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
        speedSprite.setTextColor(glowColor);

        for (auto &o : offsets) {
            speedSprite.drawString(speedText, cx + o[0], cy + o[1]);
        }

        speedSprite.setTextColor(speedColor);
        speedSprite.drawString(speedText, cx, cy);

        // Sprite kirajzolása a képernyőre
        speedSprite.pushSprite(hudState.speedValueX, hudState.speedValueY);

        // A legutóbbi sebesség szöveg és szín frissítése
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

    // Sebesség kirajzolása
    tft.setTextDatum(MC_DATUM);
    tft.setTextFont(SPEED_VALUE_FONT);
    tft.setTextSize(SPEED_VALUE_TEXT_SIZE);

    tft.setTextColor(speedColor);
    tft.drawString(speedText, hudState.speedValueX + (hudState.speedValueW / 2), hudState.speedValueY + (hudState.speedValueH / 2));

    std::strncpy(hudState.lastSpeedText, speedText, sizeof(hudState.lastSpeedText) - 1);
    hudState.lastSpeedText[sizeof(hudState.lastSpeedText) - 1] = '\0';
    hudState.lastSpeedColor = speedColor;
}
