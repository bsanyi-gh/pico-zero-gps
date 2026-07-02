#include "ScreenSetup.h"
#include "Config.h"
#include "Utils.h"
#include "defines.h"

/**
 * UI komponensek elhelyezése
 */
void ScreenSetup::layoutComponents() {

    int btnW = 100;
    int btnH = 50;
    int buttonXGap = 20;

    // 1. sor
    int buttonY = 80;

    // TFT beállítások gomb
    auto tftButton = std::make_shared<UIButton>(                                          //
        10, Rect(buttonXGap, buttonY, btnW, btnH), "TFT", UIButton::ButtonType::Pushable, //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->switchToScreen(SCREEN_NAME_TFT_SETUP);
            }
        });
    tftButton->setUseMiniFont(false);
    addChild(tftButton);

    // System beállítások button
    auto systemButton = std::make_shared<UIButton>( //
        11, Rect((tft.width() - btnW) - buttonXGap, buttonY, btnW, btnH), "System", UIButton::ButtonType::Pushable, [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->switchToScreen(SCREEN_NAME_SYSTEM_SETUP);
            }
        });
    systemButton->setUseMiniFont(false);
    addChild(systemButton);

    // 2. sor
    buttonY = 150;

    // GPS beállítások gomb
    auto gpsButton = std::make_shared<UIButton>( //
        12,                                      //
        Rect(buttonXGap, buttonY, btnW, btnH),   //
        "GPS Alarm",                             //
        UIButton::ButtonType::Pushable,          //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->switchToScreen(SCREEN_NAME_GPS_SETUP);
            }
        }); //
    gpsButton->setUseMiniFont(false);
    addChild(gpsButton);

    // Debug beállítások gomb
    auto debugButton = std::make_shared<UIButton>(                    //
        13,                                                           //
        Rect((tft.width() - btnW) - buttonXGap, buttonY, btnW, btnH), //
        "Debug",                                                      //
        UIButton::ButtonType::Pushable, [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->switchToScreen(SCREEN_NAME_DEBUG_SETUP);
            }
        }); //
    debugButton->setUseMiniFont(false);
    addChild(debugButton);

    // Back gomb jobb alsó sarokban
    addChild(std::make_shared<UIButton>(                                                                                                                                     //
        1,                                                                                                                                                                   //
        Rect(tft.width() - UIButton::DEFAULT_BUTTON_WIDTH, tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT, UIButton::DEFAULT_BUTTON_WIDTH, UIButton::DEFAULT_BUTTON_HEIGHT), //
        "Back",                                                                                                                                                              //
        UIButton::ButtonType::Pushable,                                                                                                                                      //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {

                // Itt mentjük a menthetőt
                config.checkSave();
                getScreenManager()->goBack();
            }
        }) //
    );
}

/**
 * Kirajzolja a képernyő saját tartalmát
 */
void ScreenSetup::drawContent() {
    // Háttér törlése
    tft.fillScreen(TFT_BLACK);

    // Címsor
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Setup Screen", tft.width() / 2, 30);
}
