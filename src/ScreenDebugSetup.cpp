#include "ScreenDebugSetup.h"
#include "Config.h"
#include "Utils.h"
#include "defines.h"

#include "GpsManager.h"
extern GpsManager *gpsManager;

/**
 * UI komponensek elhelyezése
 */
void ScreenDebugSetup::layoutComponents() {

    // Függőlegesen egymás alá
    int btnW = 160;
    int btnH = 45;
    int btnX = (tft.width() - btnW) / 2;
    int btnY = 60;
    int btnGap = 10;

    // LED gomb
    int row = 0;

    auto ledButton = std::make_shared<UIButton>(                                                              //
        10,                                                                                                   // id
        Rect(btnX, btnY + row * (btnH + btnGap), btnW, btnH),                                                 // rect
        "Pico-Zero LED",                                                                                      // label
        UIButton::ButtonType::Toggleable,                                                                     // type
        config.data.debugGpsSerialOnInternalFastLed ? UIButton::ButtonState::On : UIButton::ButtonState::Off, // Kezdeti állapot
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::On || event.state == UIButton::EventButtonState::Off) {
                config.data.debugGpsSerialOnInternalFastLed = event.state == UIButton::EventButtonState::On;
                config.checkSave();
            }
        });
    ledButton->setUseMiniFont(false);
    addChild(ledButton);

    // GPS On Serial
    row++;

    auto gpsOnSerialButton = std::make_shared<UIButton>(                                         //
        11,                                                                                      // id
        Rect(btnX, btnY + row * (btnH + btnGap), btnW, btnH),                                    // rect
        "GPS On Serial",                                                                         // label
        UIButton::ButtonType::Toggleable,                                                        // type
        config.data.debugGpsSerialData ? UIButton::ButtonState::On : UIButton::ButtonState::Off, // Kezdeti állapot
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::On || event.state == UIButton::EventButtonState::Off) {
                config.data.debugGpsSerialData = event.state == UIButton::EventButtonState::On;
                config.checkSave();
            }
        });
#if not defined(__DEBUG)
    // Ha nincs Serial, akkor nincs értelme a GPS debug logolásnak
    gpsOnSerialButton->setDisabled(true);
#endif
    gpsOnSerialButton->setUseMiniFont(false);
    addChild(gpsOnSerialButton);

    // SatDB on Serial
    row++;
    auto satDbOnSerialButton = std::make_shared<UIButton>(                                               //
        12,                                                                                              // id
        Rect(btnX, btnY + row * (btnH + btnGap), btnW, btnH),                                            // rect
        "SatDB On Serial",                                                                               // label
        UIButton::ButtonType::Toggleable,                                                                // type
        config.data.debugGpsSatellitesDatabase ? UIButton::ButtonState::On : UIButton::ButtonState::Off, // Kezdeti állapot
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::On || event.state == UIButton::EventButtonState::Off) {
                config.data.debugGpsSatellitesDatabase = event.state == UIButton::EventButtonState::On;
                config.checkSave();
            }
        });
    satDbOnSerialButton->setUseMiniFont(false);
#if not defined(__DEBUG)
    // Ha nincs Serial, akkor nincs értelme a SatDB debug logolásnak
    satDbOnSerialButton->setDisabled(true);
#endif
    addChild(satDbOnSerialButton);

    // Back gomb jobb alsó sarokban
    addChild(std::make_shared<UIButton>(                                                                                                                                     //
        1,                                                                                                                                                                   //
        Rect(tft.width() - UIButton::DEFAULT_BUTTON_WIDTH, tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT, UIButton::DEFAULT_BUTTON_WIDTH, UIButton::DEFAULT_BUTTON_HEIGHT), //
        "Back",                                                                                                                                                              //
        UIButton::ButtonType::Pushable,                                                                                                                                      //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->goBack();
            }
        }) //
    );
}

/**
 *
 */
void ScreenDebugSetup::drawContent() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("Debug Settings", tft.width() / 2, 20);
    tft.setFreeFont();
}
