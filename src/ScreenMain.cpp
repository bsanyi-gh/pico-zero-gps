#include <Arduino.h>

#include "ScreenMain.h"
#include "defines.h"

/**
 * @brief ScreenMain konstruktor
 */
ScreenMain::ScreenMain() : UIScreen(SCREEN_NAME_MAIN) {

    DEBUG("ScreenMain: Constructor called\n");

    // Feliratkozás a config változásokra
    configCallbackId = config.registerChangeCallback([this]() { this->onConfigChanged(); });

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
 * Kirajzolja a képernyő saját tartalmát (statikus elemek)
 */
void ScreenMain::drawContent() {}

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
void ScreenMain::handleOwnLoop() {}

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
