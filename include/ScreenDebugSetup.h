#pragma once

#include "UIScreen.h"

/**
 * Debug képernyő beállítása
 */
class ScreenDebugSetup : public UIScreen {
  public:
    ScreenDebugSetup() : UIScreen(SCREEN_NAME_DEBUG_SETUP) { layoutComponents(); }
    void layoutComponents();
    void drawContent() override;
};
