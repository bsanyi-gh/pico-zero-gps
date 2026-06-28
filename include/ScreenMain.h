#pragma once

#include "ButtonsGroupManager.h"
#include "GpsManager.h"
#include "MessageDialog.h"
#include "SensorUtils.h"
#include "TraffipaxManager.h"
#include "UIScreen.h"
#include "ValueChangeDialog.h"

class ScreenMain : public UIScreen, public ButtonsGroupManager<ScreenMain> {

  public:
    ScreenMain();
    virtual ~ScreenMain();

    /**
     * @brief Képernyő aktiválása
     *
     * Meghívódik amikor a képernyő aktívvá válik (pl. visszatérés Info/Setup képernyőről)
     * Reseteli a statikus változókat hogy kényszerítse az újrarajzolást
     */
    virtual void activate() override;

    /**
     * @brief Loop hívás felülírása animációs vagy egyéb saját logika végrehajtására
     * @note Ez a metódus nem hívja meg a gyerek komponensek loop-ját, csak saját logikát tartalmaz.
     */
    virtual void handleOwnLoop() override;

    /**
     * @brief Kirajzolja a képernyő saját tartalmát
     */
    virtual void drawContent() override;

    /**
     * @brief Touch esemény kezelése
     * @param event A touch esemény, amely tartalmazza a koordinátákat és a lenyomás állapotát.
     * @return true, ha a touch eseményt kezelte a képernyő, false egyébként
     */
    virtual bool handleTouch(const TouchEvent &event) override;

    /**
     * @brief Callback függvény, amit a Config hív meg változás esetén
     */
    void onConfigChanged();

  private:
    /**
     * @brief Kényszerített újrarajzolás flag - amikor visszatérünk más képernyőről
     */
    bool forceRedraw = false;

    // Config callback id a leiratkozáshoz
    size_t configCallbackId;

    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents();
};
