#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "TraffipaxManager.h"

/**
 * @brief A Traffipax riasztás vezérléséért felelős osztály
 */
class TraffipaxAlertController {
  public:
    /**
     * @brief A TraffipaxAlertController riasztás állapotának enumerációja
     */
    enum class AlertState {
        INACTIVE,
        APPROACHING,
        NEARBY_STOPPED,
        DEPARTING,
    };

    /**
     * @brief A TraffipaxAlertController konfigurációs pillanatképe
     */
    struct ConfigSnapshot {
        bool gpsAlarmEnabled = false;
        bool gpsSirenEnabled = false;
        bool beeperEnabled = false;
        uint16_t alarmDistanceM = 0;
    };

    /**
     * @brief A TraffipaxAlertController frissítése a GPS koordináták és a konfiguráció alapján
     */
    struct UpdateResult {
        bool baseAreaNeedsRestore = false;
        bool hudNeedsRepaint = false;
        bool alertActive = false;
    };

    static constexpr int16_t ALERT_DRAW_HEIGHT = 44;

    TraffipaxAlertController() = default;

    void reset();

    UpdateResult update(double currentLat, double currentLon, bool positionValid, const ConfigSnapshot &cfg, unsigned long currentTime, TFT_eSPI &tft, TraffipaxManager &traffipaxManager);

  private:
    /**
     * @brief A Traffipax riasztás állapotának futásidejű adatai
     */
    struct AlertRuntimeState {
        AlertState currentState = AlertState::INACTIVE;
        const TraffipaxManager::TraffipaxRecord *activeTraffipax = nullptr;
        double currentDistance = 0.0;
        double lastDistance = 999999.0;
        unsigned long lastStateChangeTime = 0;
        unsigned long lastSirenTime = 0;
    };

    /**
     * @brief A Traffipax riasztás sziréna futásidejű adatai
     */
    struct SirenRuntimeState {
        bool active = false;
        uint8_t step = 0;
        unsigned long nextStepTime = 0;
    };

    static constexpr int16_t TRAFFI_ALERT_Y = 0;
    static constexpr int16_t TRAFFI_ALERT_H = ALERT_DRAW_HEIGHT;
    static constexpr uint32_t OUT_OF_RANGE_CLEAR_MS = 3000;
    static constexpr uint32_t STATE_CHANGE_HOLD_MS = 1200;
    static constexpr uint32_t SIREN_INTERVAL_MS = 10000;
    static constexpr double DISTANCE_EPSILON_M = 10.0;
    static constexpr double SWITCH_TO_DEPART_DELTA_M = 18.0;
    static constexpr double SWITCH_TO_APPROACH_DELTA_M = 12.0;

    AlertRuntimeState alertState;
    SirenRuntimeState sirenState;
    unsigned long outOfRangeStart = 0;

    static void drawAlert(TFT_eSPI &tft, const TraffipaxManager::TraffipaxRecord *traffipax, double distance, AlertState state);
    static AlertState calculateState(AlertState currentState, double currentDistance, double lastDistance, uint16_t alarmDistanceM);

    void stopSiren();
    void startSiren(unsigned long currentTime, bool beeperEnabled);
    void updateSiren(unsigned long currentTime);
};
