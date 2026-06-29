#pragma once

#include "UIScreen.h"

/**
 * @file ScreenMain.h
 * @brief Test képernyő osztály, amely használja a ButtonsGroupManager-t
 */

class ScreenScreenSaver : public UIScreen {

  public:
    /**
     * @brief ScreenScreenSaver konstruktor
     * @param tft TFT display referencia
     */
    ScreenScreenSaver();

    /**
     * @brief ScreenScreenSaver destruktor
     */
    virtual ~ScreenScreenSaver();

    /**
     * @brief Loop hívás felülírása
     * animációs vagy egyéb saját logika végrehajtására
     * @note Ez a metódus nem hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
     */
    virtual void handleOwnLoop() override;

    /**
     * @brief Érintés esemény kezelése
     * @param event Érintés esemény adatok
     * @return true ha kezelte az eseményt (mindig), false egyébként
     * @details Bármilyen érintés ébreszti a képernyővédőt és visszatér az előző képernyőre
     */
    virtual bool handleTouch(const TouchEvent &event) override;

    /**
     * @brief Képernyő aktiválása
     *
     * Meghívódik amikor a képernyő aktívvá válik (pl. visszatérés Info/Setup képernyőről)
     * Reseteli a statikus változókat hogy kényszerítse az újrarajzolást
     */
    virtual void activate() override;

    /**
     * @brief Kirajzolja a képernyő saját tartalmát
     */
    virtual void drawContent() override;

  private:
    /// Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
    static constexpr uint8_t SCREENSAVER_TFT_FONT = 7;
    int32_t textW;
    int32_t textH;

    int32_t lastX = 0, lastY = 0;

    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents();
};
