#pragma once

#include "ButtonsGroupManager.h"
#include "GpsManager.h"
#include "MessageDialog.h"
#include "SensorUtils.h"
#include "TraffipaxManager.h"
#include "UIScreen.h"
#include "ValueChangeDialog.h"
#include <TFT_eSPI.h>

class ScreenMain : public UIScreen, public ButtonsGroupManager<ScreenMain> {

  public:
    ScreenMain();
    virtual ~ScreenMain();

    /**
     * @brief Képernyő aktiválása
     *
     * Meghívódik amikor a képernyő aktívvá válik (pl. visszatérés Info/Setup képernyőről)
     * Reseteli a statikus változókat hogy kényszerítse az újrarajzolást
     */
    virtual void activate() override;

    /**
     * @brief Loop hívás felülírása animációs vagy egyéb saját logika végrehajtására
     * @note Ez a metódus nem hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
     */
    virtual void handleOwnLoop() override;

    /**
     * @brief Kirajzolja a képernyő saját tartalmát
     */
    virtual void drawContent() override;

    /**
     * @brief Touch esemény kezelése
     * @param event A touch esemény, amely tartalmazza a koordinátákat és a lenyomás állapotát.
     * @return true, ha a touch eseményt kezelte a képernyő, false egyébként
     */
    virtual bool handleTouch(const TouchEvent &event) override;

    /**
     * @brief Callback függvény, amit a Config hív meg változás esetén
     */
    void onConfigChanged();

  private:
    static constexpr float HUD_MAX_SPEED_KMPH = 180.0f;
    static constexpr uint32_t HUD_UPDATE_INTERVAL_MS = 120;

    // Sebesség widget pozíció és méret
    static constexpr int16_t TRACK_X = 8;
    static constexpr int16_t TRACK_Y = 6;
    static constexpr int16_t TRACK_W = 98;
    static constexpr int16_t TRACK_H = 40;

    // Idő widget pozíció és méret
    static constexpr int16_t TIME_X = 111;
    static constexpr int16_t TIME_Y = 6;
    static constexpr int16_t TIME_W = 98;
    static constexpr int16_t TIME_H = 40;

    // PREC widget pozíció és méret
    static constexpr int16_t PREC_X = 214;
    static constexpr int16_t PREC_Y = 6;
    static constexpr int16_t PREC_W = 98;
    static constexpr int16_t PREC_H = 40;

    // Sebesség widget pozíció és méret
    static constexpr int16_t SPEED_X = 54;
    static constexpr int16_t SPEED_Y = 52;
    static constexpr int16_t SPEED_W = 212;
    static constexpr int16_t SPEED_H = 118;

    // Függőleges sensor barok pozíció és méret
    static constexpr int16_t SENSOR_BAR_X = 0;
    static constexpr int16_t SENSOR_BAR_Y = 52;
    static constexpr int16_t SENSOR_BAR_W = 52;
    static constexpr int16_t SENSOR_BAR_H = 118;
    static constexpr int16_t SENSOR_BAR_RIGHT_X = 320 - SENSOR_BAR_W;

    // Alsó információs sor
    static constexpr int16_t INFO_X = 18;
    static constexpr int16_t INFO_Y = 175;
    static constexpr int16_t INFO_W = 284;
    static constexpr int16_t INFO_H = 26;

    // Traffipax alert sáv
    static constexpr int16_t TRAFFI_ALERT_Y = 0;
    static constexpr int16_t TRAFFI_ALERT_H = 48;
    static constexpr uint32_t TRAFFI_ALERT_OUT_OF_RANGE_CLEAR_MS = 3000;
    static constexpr uint32_t TRAFFI_ALERT_SIREN_INTERVAL_MS = 10000;
    static constexpr double TRAFFI_ALERT_DISTANCE_EPSILON_M = 10.0;

    struct ScreenMainHudState {
        bool initialized = false;
        bool staticPainted = false;
        bool speedSpriteReady = false;
        bool sensorBarSpriteReady = false;
        float smoothSpeed = 0.0f;
        float lastDrawnSpeed = -1000.0f;
        uint32_t lastRedrawMs = 0;

        float lastVoltageValue = -1000.0f;
        bool lastVoltageMode = false;

        float lastTemperatureValue = -1000.0f;
        bool lastTemperatureMode = false;

        char lastSatText[24] = "";
        uint16_t lastSatColor = 0;

        char lastTimeText[24] = "";
        uint16_t lastTimeColor = 0;

        char lastHdopText[24] = "";
        uint16_t lastHdopColor = 0;

        char lastBottomText[128] = "";
    };

    struct TraffipaxAlertState {
        enum State {
            INACTIVE,
            APPROACHING,
            NEARBY_STOPPED,
            DEPARTING,
        };

        State currentState = INACTIVE;
        const TraffipaxManager::TraffipaxRecord *activeTraffipax = nullptr;
        double currentDistance = 0.0;
        double lastDistance = 999999.0;
        unsigned long lastSirenTime = 0;
    };

    struct TraffipaxSirenState {
        bool active = false;
        uint8_t step = 0;
        unsigned long nextStepTime = 0;
        unsigned long nextRepeatTime = 0;
    };

    ScreenMainHudState hudState;
    TFT_eSprite speedSprite;
    TFT_eSprite sensorBarSprite;
    TraffipaxAlertState traffipaxAlert;
    TraffipaxSirenState traffipaxSiren;
    unsigned long traffiOutOfRangeStart = 0;

    /**
     * @brief Kényszerített újrarajzolás flag - amikor visszatérünk más képernyőről
     */
    bool forceRedraw = false;

    // Config callback id a leiratkozáshoz
    size_t configCallbackId;

    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents();

    void stopTraffipaxSiren();
    void startTraffipaxSiren(unsigned long currentTime);
    void updateTraffipaxSiren(unsigned long currentTime);
    void drawTraffipaxBaseArea();
    void clearTraffipaxAlert();
    void displayTraffipaxAlert(const TraffipaxManager::TraffipaxRecord *traffipax, double distance, TraffipaxAlertState::State state);
    void processTraffipaxAlert(double currentLat, double currentLon, bool positionValid, bool &forceRedrawFlag);

    static float clampf(float v, float lo, float hi);
    static uint16_t arcColorForRatio(float ratio);
    static uint16_t meterColorForRatio(float ratio, bool temperatureBar);

    void drawHudPanel(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, const char *value, uint16_t valueColor);
    void drawHudPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *value, uint16_t valueColor);
    void drawSensorBarSprite(float value, float minVal, float maxVal, const char *title, const char *unit, bool temperatureBar);
    void ensureSensorBarSpriteReady();

    template <typename Canvas> void drawSpeedArc(Canvas &canvas, float speedKmph, int16_t centerX, int16_t centerY, int16_t rOuter, int16_t rInner);
    void drawStaticHudBackground();
    void ensureSpeedSpriteReady();
    void drawSpeedWidget(float speedKmph, bool speedValid);
};
