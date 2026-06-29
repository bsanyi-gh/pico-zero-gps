#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "TraffipaxManager.h"

class TraffipaxAlertController {
  public:
    enum class AlertState {
        INACTIVE,
        APPROACHING,
        NEARBY_STOPPED,
        DEPARTING,
    };

    struct ConfigSnapshot {
        bool gpsAlarmEnabled = false;
        bool gpsSirenEnabled = false;
        bool beeperEnabled = false;
        uint16_t alarmDistanceM = 0;
    };

    struct UpdateResult {
        bool baseAreaNeedsRestore = false;
        bool hudNeedsRepaint = false;
    };

    TraffipaxAlertController() = default;

    void reset();

    UpdateResult update(double currentLat, double currentLon, bool positionValid, const ConfigSnapshot &cfg, unsigned long currentTime, TFT_eSPI &tft, TraffipaxManager &traffipaxManager);

  private:
    struct AlertRuntimeState {
        AlertState currentState = AlertState::INACTIVE;
        const TraffipaxManager::TraffipaxRecord *activeTraffipax = nullptr;
        double currentDistance = 0.0;
        double lastDistance = 999999.0;
        unsigned long lastSirenTime = 0;
    };

    struct SirenRuntimeState {
        bool active = false;
        uint8_t step = 0;
        unsigned long nextStepTime = 0;
    };

    static constexpr int16_t TRAFFI_ALERT_Y = 0;
    static constexpr int16_t TRAFFI_ALERT_H = 48;
    static constexpr uint32_t OUT_OF_RANGE_CLEAR_MS = 3000;
    static constexpr uint32_t SIREN_INTERVAL_MS = 10000;
    static constexpr double DISTANCE_EPSILON_M = 10.0;

    AlertRuntimeState alertState;
    SirenRuntimeState sirenState;
    unsigned long outOfRangeStart = 0;

    static void drawAlert(TFT_eSPI &tft, const TraffipaxManager::TraffipaxRecord *traffipax, double distance, AlertState state);
    static AlertState calculateState(AlertState currentState, double currentDistance, double lastDistance, uint16_t alarmDistanceM);

    void stopSiren();
    void startSiren(unsigned long currentTime, bool beeperEnabled);
    void updateSiren(unsigned long currentTime);
};
