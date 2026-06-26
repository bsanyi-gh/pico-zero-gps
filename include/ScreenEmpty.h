#pragma once

#include "UIButton.h"
#include "UIScreen.h"

/**
 * @brief Üres képernyő osztály - hibakereséshez és teszteléshez
 */
class ScreenEmpty : public UIScreen {

  public:
    /**
     * @brief Konstruktor
     * @param tft TFT display referencia
     */
    ScreenEmpty() : UIScreen(SCREEN_NAME_EMPTY) {
        DEBUG("ScreenEmpty: Constructor called\n");
        layoutComponents();
    }
    virtual ~ScreenEmpty() = default;

    /**
     * @brief Loop hívás felülírása
     * animációs vagy egyéb saját logika végrehajtására
     * @note Ez a metódus nem hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
     */
    virtual void handleOwnLoop() override {}

    /**
     * @brief Kirajzolja a képernyő saját tartalmát
     */
    virtual void drawContent() override {
        // Szöveg középre igazítása
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_COLOR_BACKGROUND);

        tft.setFreeFont();
        tft.setTextSize(3);

        // Képernyő cím kirajzolása
        tft.drawString(SCREEN_NAME_EMPTY, ::tft.width() / 2, ::tft.height() / 2 - 20);

        // Információs szöveg
        tft.setTextSize(1);
        tft.drawString("ScreenEmpty  for debugging", ::tft.width() / 2, ::tft.height() / 2 + 20);
    }

  private:
    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents() {}
};
