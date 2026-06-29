#include "TraffipaxAlertController.h"

#include <cmath>

#include "pins.h"

void TraffipaxAlertController::stopSiren() {
    noTone(PIN_BUZZER);
    sirenState.active = false;
    sirenState.step = 0;
    sirenState.nextStepTime = 0;
}

void TraffipaxAlertController::startSiren(unsigned long currentTime, bool beeperEnabled) {
    if (!beeperEnabled) {
        return;
    }

    sirenState.active = true;
    sirenState.step = 0;
    sirenState.nextStepTime = currentTime + 50;
    tone(PIN_BUZZER, 600);
}

void TraffipaxAlertController::updateSiren(unsigned long currentTime) {
    if (!sirenState.active) {
        return;
    }

    if (currentTime < sirenState.nextStepTime) {
        return;
    }

    if (sirenState.step == 0) {
        noTone(PIN_BUZZER);
        tone(PIN_BUZZER, 1800);
        sirenState.step = 1;
        sirenState.nextStepTime = currentTime + 50;
    } else {
        stopSiren();
    }
}

void TraffipaxAlertController::drawAlert(TFT_eSPI &tft, const TraffipaxManager::TraffipaxRecord *traffipax, double distance, AlertState state) {
    if (traffipax == nullptr) {
        return;
    }

    uint16_t backgroundColor = TFT_RED;
    uint16_t textColor = TFT_WHITE;
    if (state == AlertState::DEPARTING) {
        backgroundColor = TFT_ORANGE;
        textColor = TFT_BLACK;
    }

    tft.fillRect(0, TRAFFI_ALERT_Y, tft.width(), TRAFFI_ALERT_H, backgroundColor);
    tft.setFreeFont();
    tft.setTextColor(textColor, backgroundColor);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.drawString(traffipax->city, 8, 8);

    tft.setTextSize(1);
    tft.drawString(traffipax->street_or_km, 8, 28);

    char distanceText[16];
    snprintf(distanceText, sizeof(distanceText), "%dm", static_cast<int>(std::lround(distance)));
    tft.setTextDatum(MR_DATUM);
    tft.setFreeFont(&FreeSerifBold24pt7b);
    tft.setTextSize(1);
    tft.setTextPadding(tft.textWidth("8888m"));
    tft.drawString(distanceText, tft.width() - 8, TRAFFI_ALERT_H / 2);
    tft.setFreeFont();
}

TraffipaxAlertController::AlertState TraffipaxAlertController::calculateState(AlertState currentState, double currentDistance, double lastDistance, uint16_t alarmDistanceM) {
    const bool isApproaching = currentDistance < (lastDistance - DISTANCE_EPSILON_M);
    const bool isDeparting = currentDistance > (lastDistance + DISTANCE_EPSILON_M);
    const bool isStoppedNear = !isApproaching && !isDeparting && currentDistance <= alarmDistanceM;

    if (currentState == AlertState::INACTIVE || isApproaching) {
        return AlertState::APPROACHING;
    }
    if (isDeparting) {
        return AlertState::DEPARTING;
    }
    if (isStoppedNear) {
        return AlertState::NEARBY_STOPPED;
    }

    return currentState;
}

void TraffipaxAlertController::reset() {
    stopSiren();
    alertState = AlertRuntimeState{};
    outOfRangeStart = 0;
}

TraffipaxAlertController::UpdateResult TraffipaxAlertController::update(double currentLat, double currentLon, bool positionValid, const ConfigSnapshot &cfg, unsigned long currentTime, TFT_eSPI &tft,
                                                                        TraffipaxManager &traffipaxManager) {
    UpdateResult result;

    if (!positionValid || !cfg.gpsAlarmEnabled) {
        stopSiren();
        if (alertState.currentState != AlertState::INACTIVE) {
            alertState.currentState = AlertState::INACTIVE;
            alertState.activeTraffipax = nullptr;
            alertState.currentDistance = 0.0;
            result.baseAreaNeedsRestore = true;
            result.hudNeedsRepaint = true;
        }
        outOfRangeStart = 0;
        return result;
    }

    double minDistance = 999999.0;
    const TraffipaxManager::TraffipaxRecord *closestTraffipax = traffipaxManager.getClosestTraffipax(currentLat, currentLon, minDistance);

    if (closestTraffipax == nullptr) {
        if (alertState.currentState != AlertState::INACTIVE) {
            alertState.currentState = AlertState::INACTIVE;
            alertState.activeTraffipax = nullptr;
            result.baseAreaNeedsRestore = true;
            result.hudNeedsRepaint = true;
        }
        outOfRangeStart = 0;
        return result;
    }

    if (minDistance > cfg.alarmDistanceM) {
        if (outOfRangeStart == 0) {
            outOfRangeStart = currentTime;
        }

        if (currentTime - outOfRangeStart > OUT_OF_RANGE_CLEAR_MS) {
            if (alertState.currentState != AlertState::INACTIVE) {
                alertState.currentState = AlertState::INACTIVE;
                alertState.activeTraffipax = nullptr;
                result.baseAreaNeedsRestore = true;
                result.hudNeedsRepaint = true;
            }
            outOfRangeStart = 0;
            return result;
        }

        if (alertState.currentState != AlertState::INACTIVE && alertState.activeTraffipax) {
            alertState.currentDistance = minDistance;
            drawAlert(tft, alertState.activeTraffipax, minDistance, alertState.currentState);
        }
        return result;
    }

    outOfRangeStart = 0;

    const AlertState newState = calculateState(alertState.currentState, minDistance, alertState.lastDistance, cfg.alarmDistanceM);
    if (newState != alertState.currentState) {
        alertState.currentState = newState;
        alertState.activeTraffipax = closestTraffipax;
        result.hudNeedsRepaint = true;
        if (newState != AlertState::APPROACHING) {
            stopSiren();
        }
    }

    alertState.currentDistance = minDistance;
    drawAlert(tft, closestTraffipax, minDistance, alertState.currentState);

    if (cfg.gpsSirenEnabled && alertState.currentState == AlertState::APPROACHING) {
        if (currentTime - alertState.lastSirenTime >= SIREN_INTERVAL_MS) {
            startSiren(currentTime, cfg.beeperEnabled);
            alertState.lastSirenTime = currentTime;
        }
    }

    updateSiren(currentTime);
    alertState.lastDistance = minDistance;

    return result;
}
