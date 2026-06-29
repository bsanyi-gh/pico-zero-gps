#pragma once
#include <TFT_eSPI.h>

#include "ButtonsGroupManager.h"
#include "GpsManager.h"
#include "MessageDialog.h"
#include "SensorUtils.h"
#include "TraffipaxAlertController.h"
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
    static constexpr uint32_t HUD_UPDATE_INTERVAL_MS = 500;

    // Felső HUD panelek pozíciója és mérete
    static constexpr int16_t TOP_PANEL_Y = 6;
    static constexpr int16_t TOP_PANEL_H = 40;
    static constexpr int16_t TOP_PANEL_LEFT_MARGIN = 0;
    static constexpr int16_t TOP_PANEL_RIGHT_MARGIN = 0;
    static constexpr int16_t TOP_PANEL_GAP = 5;
    static constexpr int16_t TIME_W = 78;
    static constexpr int16_t PREC_W = 78;
    int16_t PREC_X = tft.width() - TOP_PANEL_RIGHT_MARGIN - PREC_W;
    const int16_t TIME_X = PREC_X - TOP_PANEL_GAP - TIME_W;
    static constexpr int16_t TRACK_X = TOP_PANEL_LEFT_MARGIN;
    const int16_t TRACK_W = TIME_X - TOP_PANEL_GAP - TRACK_X;

    // Sat/Track panel
    static constexpr int16_t TRACK_Y = TOP_PANEL_Y;
    static constexpr int16_t TRACK_H = TOP_PANEL_H;

    // Idő panel
    static constexpr int16_t TIME_Y = 6;
    static constexpr int16_t TIME_H = TOP_PANEL_H;

    // ALT panel
    static constexpr int16_t PREC_Y = 6;
    static constexpr int16_t PREC_H = TOP_PANEL_H;

    // Sebesség widget pozíció és méret
    static constexpr int16_t SPEED_X = 54;
    static constexpr int16_t SPEED_Y = 52;
    static constexpr int16_t SPEED_W = 212;
    static constexpr int16_t SPEED_H = 118;
    static constexpr int16_t SPEED_UNIT_BASELINE_Y = SPEED_Y + 96;
    static constexpr uint8_t SPEED_VALUE_FONT = 7;      // A sebesség értékének betűtípusa (7-es GFXFF font, 7 szegmenses font, csak számok)
    static constexpr uint8_t SPEED_VALUE_TEXT_SIZE = 2; // A sebesség értékének betűmérete (2-es GFXFF font)

    // Függőleges sensor bar-ok pozíció és méret
    static constexpr int16_t SENSOR_BAR_X = 0;
    static constexpr int16_t SENSOR_BAR_Y_BOTTOM = 170;
    static constexpr int16_t SENSOR_BAR_W = 67;
    static constexpr int16_t SENSOR_BAR_H = 118;
    static constexpr int16_t SENSOR_BAR_Y_TOP = SENSOR_BAR_Y_BOTTOM - SENSOR_BAR_H;
    const int16_t SENSOR_BAR_X_RIGHT;

    // Alsó információs sor (a két alsó gomb közötti sáv)
    static constexpr int16_t INFO_X = 56;
    static constexpr int16_t INFO_Y = 216;
    static constexpr int16_t INFO_W = 208;
    static constexpr int16_t INFO_H = 24;

    struct ScreenMainHudState {
        bool initialized = false;
        bool staticPainted = false;
        bool speedSpriteReady = false;
        bool sensorBarSpriteReady = false;
        int16_t speedValueX = SPEED_X + 12;
        int16_t speedValueY = SPEED_Y + 8;
        int16_t speedValueW = SPEED_W - 24;
        int16_t speedValueH = 86;
        float lastSpeedValue = -1000.0f;
        char lastSpeedText[16] = "";
        uint16_t lastSpeedColor = 0;
        uint32_t lastRedrawMs = 0;

        float lastVoltageValue = -1000.0f;
        bool lastVoltageMode = false;

        float lastTemperatureValue = -1000.0f;
        bool lastTemperatureMode = false;

        char lastSatText[24] = "";
        char lastTrackMetaText[64] = "";
        uint16_t lastSatColor = 0;

        char lastTimeText[24] = "";
        uint16_t lastTimeColor = 0;

        char lastAltText[24] = "";
        uint16_t lastAltColor = 0;

        char lastBottomText[128] = "";
    };

    ScreenMainHudState hudState;
    TFT_eSprite speedSprite;
    // Sprite a vertikális bar-oknak
    TFT_eSprite sensorBarSprite;

    TraffipaxAlertController traffipaxAlertController;

    /**
     * @brief Kényszerített újrarajzolás flag - amikor visszatérünk más képernyőről
     */
    bool forceRedraw = false;

    // Bekapcsolás óta mért legnagyobb (kijelzett) sebesség.
    float maxSpeedSinceBoot = 0.0f;

    // Config callback id a leiratkozáshoz
    size_t configCallbackId;

    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents();

    void drawTraffipaxBaseArea();

    static float clampf(float v, float lo, float hi);
    static uint16_t meterColorForRatio(float ratio, bool temperatureBar);

    void drawHudPanel(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, const char *value, uint16_t valueColor);
    void drawHudPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *value, uint16_t valueColor);
    void drawAltitudePanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *value, uint16_t valueColor);
    void drawTrackPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *satValue, const char *qualityValue, const char *modeValue, uint16_t satColor, bool updateSat, bool updateMeta);
    void ensureSensorBarSpriteReady();
    void ensureSpeedSpriteReady();
    void updateSpeedValueLayoutForFont();

    void drawStaticHudBackground();
    void drawSpeedWidget(float speedKmph, bool speedValid, bool forceUpdate);
};
