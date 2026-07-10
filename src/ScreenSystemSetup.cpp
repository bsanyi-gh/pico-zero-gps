#include "ScreenSystemSetup.h"
#include "Config.h"
#include "Utils.h"
#include "ValueChangeDialog.h"
#include "defines.h"

void ScreenSystemSetup::layoutComponents() {

    // Függőlegesen egymás alá
    int btnW = 160;
    int btnH = 45;
    int btnX = (tft.width() - btnW) / 2;
    int btnY = 60;
    int btnGap = 10;

    // Beeper gomb
    int row = 0;
    auto beeperButton = std::make_shared<UIButton>( //
        10,                                         // id
        Rect(btnX, btnY + row * (btnH + btnGap), btnW, btnH),
        "Beeper",                                                                             // label
        UIButton::ButtonType::Toggleable,                                                     // type
        ::config.data.beeperEnabled ? UIButton::ButtonState::On : UIButton::ButtonState::Off, // Kezdeti állapot
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::On || event.state == UIButton::EventButtonState::Off) {
                ::config.data.beeperEnabled = (event.state == UIButton::EventButtonState::On);
            }
        }); //
    beeperButton->setUseMiniFont(false);
    addChild(beeperButton);

    // Demo mód
    row++;
    auto demoButton = std::make_shared<UIButton>( //
        11,                                       // id
        Rect(btnX, btnY + row * (btnH + btnGap), btnW, btnH),
        "Demo Mode",                                                       // label
        UIButton::ButtonType::Toggleable,                                  // type
        demoMode ? UIButton::ButtonState::On : UIButton::ButtonState::Off, // Kezdeti állapot
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::On || event.state == UIButton::EventButtonState::Off) {
                demoMode = (event.state == UIButton::EventButtonState::On);
            }
        }); //

    demoButton->setUseMiniFont(false);
    addChild(demoButton);

    // Screen Saver gomb
    row++;
    auto screenSaverButton = std::make_shared<UIButton>(      //
        12,                                                   //
        Rect(btnX, btnY + row * (btnH + btnGap), btnW, btnH), //
        "Screen Saver",                                       //
        UIButton::ButtonType::Pushable,                       //
        UIButton::ButtonState::Off,                           //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                static int screenSaverTimeout = static_cast<int>(config.data.screenSaverTimeout);

                auto dialog = std::make_shared<ValueChangeDialog>(
                    this, "Screen Saver Timeout", "0-60 minutes, 0->off", //
                    &screenSaverTimeout, 0, 60, 1,
                    [this](const std::variant<int, float, bool> &newValue) {
                        if (std::holds_alternative<int>(newValue)) {
                            int screenSaverTimeout = std::get<int>(newValue);
                            screenSaverTimeout = constrain(screenSaverTimeout, 0, 60);
                            config.data.screenSaverTimeout = static_cast<uint8_t>(screenSaverTimeout);
                        }
                    },
                    [this](UIDialogBase *dialog, UIDialogBase::DialogResult result) {
                        if (result == UIDialogBase::DialogResult::Accepted) {
                        } else if (result == UIDialogBase::DialogResult::Rejected) {
                            screenSaverTimeout = static_cast<int>(config.data.screenSaverTimeout);
                        }
                    });

                showDialog(dialog);
            }
        });
    screenSaverButton->setUseMiniFont(false);
    addChild(screenSaverButton);

    // Vissza gomb
    addChild(std::make_shared<UIButton>( //                                                                                                                                  //
        1,                               //
        Rect(tft.width() - UIButton::DEFAULT_BUTTON_WIDTH, tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT, UIButton::DEFAULT_BUTTON_WIDTH, UIButton::DEFAULT_BUTTON_HEIGHT), //
        "Back", UIButton::ButtonType::Pushable, [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->goBack();
            }
        }));
}

void ScreenSystemSetup::drawContent() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("System Settings", tft.width() / 2, 20);
    tft.setFreeFont();
}
