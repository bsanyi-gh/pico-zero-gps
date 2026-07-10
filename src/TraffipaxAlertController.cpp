#include <TinyGPS++.h>
#include <cmath>

#include "TraffipaxAlertController.h"
#include "pins.h"

namespace {
double distanceToRecordMeters(double lat, double lon, const TraffipaxManager::TraffipaxRecord *rec) {
    if (rec == nullptr) {
        return 999999.0;
    }
    return TinyGPSPlus::distanceBetween(lat, lon, rec->lat, rec->lon);
}
} // namespace

/**
 * @brief Siren leállítása
 */
void TraffipaxAlertController::stopSiren() {
    noTone(PIN_BUZZER);
    sirenState.active = false;
    sirenState.step = 0;
    sirenState.nextStepTime = 0;
}

/**
 * @brief Siren elindítása
 *
 * @param currentTime Az aktuális idő millis() formátumban
 * @param beeperEnabled true, ha a beeper engedélyezve van, false ha nem
 */
void TraffipaxAlertController::startSiren(unsigned long currentTime, bool beeperEnabled) {
    if (!beeperEnabled) {
        return;
    }

    sirenState.active = true;
    sirenState.step = 0;
    sirenState.nextStepTime = currentTime + 120;
    tone(PIN_BUZZER, 600);
}

/**
 * @brief Siren frissítése, a következő lépéshez szükséges idő ellenőrzése
 *
 * @param currentTime Az aktuális idő millis() formátumban
 */
void TraffipaxAlertController::updateSiren(unsigned long currentTime) {
    if (!sirenState.active) {
        return;
    }

    if (currentTime < sirenState.nextStepTime) {
        return;
    }

    if (sirenState.step == 0) {
        noTone(PIN_BUZZER);
        tone(PIN_BUZZER, 900);
        sirenState.step = 1;
        sirenState.nextStepTime = currentTime + 120;
    } else if (sirenState.step == 1) {
        noTone(PIN_BUZZER);
        tone(PIN_BUZZER, 1300);
        sirenState.step = 2;
        sirenState.nextStepTime = currentTime + 120;
    } else if (sirenState.step == 2) {
        noTone(PIN_BUZZER);
        tone(PIN_BUZZER, 1800);
        sirenState.step = 3;
        sirenState.nextStepTime = currentTime + 140;
    } else {
        stopSiren();
    }
}

/**
 * @brief Trafipax riasztás kirajzolása a kijelzőre
 *
 * @param tft A TFT kijelző objektum
 * @param traffipax A legközelebbi trafipax rekordja
 * @param distance A trafipax távolsága méterben
 * @param state A riasztás állapota
 *
 */
void TraffipaxAlertController::drawAlert(TFT_eSPI &tft, const TraffipaxManager::TraffipaxRecord *traffipax, double distance, AlertState state) {
    if (traffipax == nullptr) {
        return;
    }

    static AlertState lastState = AlertState::INACTIVE;
    static const TraffipaxManager::TraffipaxRecord *lastTraffipax = nullptr;
    static int lastDistance = -1;

    uint16_t backgroundColor = TFT_RED;
    uint16_t textColor = TFT_WHITE;
    if (state == AlertState::DEPARTING) {
        backgroundColor = TFT_ORANGE;
        textColor = TFT_BLACK;
    }

    const bool fullRedraw = (state != lastState) || (traffipax != lastTraffipax);
    if (fullRedraw) {
        tft.fillRect(0, TRAFFI_ALERT_Y, tft.width(), TRAFFI_ALERT_H, backgroundColor);
        tft.setFreeFont();
        tft.setTextColor(textColor, backgroundColor);

        // Cím: nagyobb font, balra igazítva.
        tft.setTextDatum(TL_DATUM);
        tft.setFreeFont(&FreeSansBold12pt7b);
        tft.setTextSize(1);
        tft.drawString(traffipax->city, 8, 0);

        tft.setTextDatum(TL_DATUM);
        tft.setFreeFont();
        tft.drawString(traffipax->street_or_km, 8, 22);

        const char *stateLabel = "NEARBY";
        if (state == AlertState::APPROACHING) {
            stateLabel = "APPROACHING";
        } else if (state == AlertState::DEPARTING) {
            stateLabel = "DEPARTING";
        }
        tft.setTextColor(state == AlertState::DEPARTING ? TFT_BLACK : TFT_YELLOW, backgroundColor);
        tft.setTextDatum(BL_DATUM);
        tft.drawString(stateLabel, 8, TRAFFI_ALERT_H - 2);
        tft.setTextDatum(TL_DATUM);

        lastState = state;
        lastTraffipax = traffipax;
        lastDistance = -1;
    }

    char distanceText[16];
    const int distanceInt = static_cast<int>(std::lround(distance));
    if (fullRedraw || distanceInt != lastDistance) {
        snprintf(distanceText, sizeof(distanceText), "%dm", distanceInt);
        tft.setTextDatum(MR_DATUM);
        tft.setFreeFont(&FreeSansBold12pt7b);
        tft.setTextSize(1);
        tft.setTextPadding(tft.textWidth("8888m"));
        tft.setTextColor(textColor, backgroundColor);
        tft.drawString(distanceText, tft.width() - 8, (TRAFFI_ALERT_H / 2) + 2);
        lastDistance = distanceInt;
    }
    tft.setTextPadding(0);
    tft.setFreeFont();
}

/**
 * @brief Riasztás állapotának kiszámítása a távolság és az előző állapot alapján
 *
 * @param currentState Az aktuális riasztás állapota
 * @param currentDistance Az aktuális távolság a trafipaxhoz
 * @param lastDistance Az előző távolság a trafipaxhoz
 * @param alarmDistanceM A riasztási távolság méterben
 * @return A következő riasztás állapota
 *
 */
TraffipaxAlertController::AlertState TraffipaxAlertController::calculateState(AlertState currentState, double currentDistance, double lastDistance, uint16_t alarmDistanceM) {
    const double delta = currentDistance - lastDistance;
    const bool isApproaching = delta <= -DISTANCE_EPSILON_M;
    const bool isDeparting = delta >= DISTANCE_EPSILON_M;
    const bool isNear = currentDistance <= alarmDistanceM;

    if (currentState == AlertState::INACTIVE || isApproaching) {
        return AlertState::APPROACHING;
    }

    // Irányváltás hiszterézissel: a váltáshoz nagyobb távolságdelta kell,
    // mint a sima "azonos irányban maradáshoz". Ettől normál/demó módban sem keveredik.
    if (currentState == AlertState::APPROACHING) {
        if (delta >= SWITCH_TO_DEPART_DELTA_M) {
            return AlertState::DEPARTING;
        }
        return AlertState::APPROACHING;
    }

    if (currentState == AlertState::DEPARTING) {
        if (delta <= -SWITCH_TO_APPROACH_DELTA_M) {
            return AlertState::APPROACHING;
        }
        return AlertState::DEPARTING;
    }

    // Ha nincs egyértelmű távolságtrend, tartsuk az előző irányállapotot,
    // így demó módban sem billeg piros/sárga között.
    if (isNear && (currentState == AlertState::APPROACHING || currentState == AlertState::DEPARTING)) {
        return currentState;
    }
    if (isNear && isDeparting) {
        return AlertState::DEPARTING;
    }
    if (isNear) {
        return AlertState::APPROACHING;
    }

    return currentState;
}

/**
 * @brief Riasztás állapotának visszaállítása az alapértelmezett állapotra
 */
void TraffipaxAlertController::reset() {
    stopSiren();
    alertState = AlertRuntimeState{};
    outOfRangeStart = 0;
}

/**
 * @brief Riasztás frissítése a jelenlegi GPS koordináták és konfiguráció alapján
 *
 * @param currentLat Az aktuális GPS szélességi koordináta
 * @param currentLon Az aktuális GPS hosszúsági koordináta
 * @param positionValid true, ha a GPS pozíció érvényes, false ha nem
 * @param cfg A riasztás konfigurációs pillanatképe
 * @param currentTime Az aktuális idő millis() formátumban
 * @param tft A TFT kijelző objektum
 * @param traffipaxManager A TrafipaxManager objektum a trafipax adatok kezeléséhez
 * @return UpdateResult struktúra, amely jelzi, hogy szükséges-e a HUD újrarajzolása vagy az alapértelmezett terület visszaállítása
 */
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
        result.alertActive = false;
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
        result.alertActive = false;
        return result;
    }

    const TraffipaxManager::TraffipaxRecord *trackedTraffipax = (alertState.activeTraffipax != nullptr) ? alertState.activeTraffipax : closestTraffipax;
    const double trackedDistance = distanceToRecordMeters(currentLat, currentLon, trackedTraffipax);

    if (trackedDistance > cfg.alarmDistanceM) {
        if (outOfRangeStart == 0) {
            outOfRangeStart = currentTime;
        }

        if (currentTime - outOfRangeStart > OUT_OF_RANGE_CLEAR_MS) {
            if (alertState.currentState != AlertState::INACTIVE) {
                alertState.currentState = AlertState::INACTIVE;
                alertState.activeTraffipax = nullptr;
                alertState.lastDistance = 999999.0;
                result.baseAreaNeedsRestore = true;
                result.hudNeedsRepaint = true;
            }
            outOfRangeStart = 0;
            result.alertActive = false;
            return result;
        }

        if (alertState.currentState != AlertState::INACTIVE && alertState.activeTraffipax) {
            alertState.currentDistance = trackedDistance;
            drawAlert(tft, alertState.activeTraffipax, trackedDistance, alertState.currentState);
            result.alertActive = true;
        }
        return result;
    }

    outOfRangeStart = 0;

    if (alertState.currentState == AlertState::INACTIVE || alertState.activeTraffipax == nullptr) {
        alertState.activeTraffipax = closestTraffipax;
    }

    const double activeDistance = distanceToRecordMeters(currentLat, currentLon, alertState.activeTraffipax);

    const AlertState newState = calculateState(alertState.currentState, activeDistance, alertState.lastDistance, cfg.alarmDistanceM);
    if (newState != alertState.currentState) {
        const bool canChangeState = (alertState.currentState == AlertState::INACTIVE) || ((currentTime - alertState.lastStateChangeTime) >= STATE_CHANGE_HOLD_MS);
        if (canChangeState) {
            alertState.currentState = newState;
            alertState.lastStateChangeTime = currentTime;
            if (newState != AlertState::APPROACHING) {
                stopSiren();
            }
        }
    }

    alertState.currentDistance = activeDistance;
    drawAlert(tft, alertState.activeTraffipax, activeDistance, alertState.currentState);
    result.alertActive = true;

    if (cfg.gpsSirenEnabled && alertState.currentState == AlertState::APPROACHING) {
        if (currentTime - alertState.lastSirenTime >= SIREN_INTERVAL_MS) {
            startSiren(currentTime, cfg.beeperEnabled);
            alertState.lastSirenTime = currentTime;
        }
    }

    updateSiren(currentTime);
    alertState.lastDistance = activeDistance;

    return result;
}
