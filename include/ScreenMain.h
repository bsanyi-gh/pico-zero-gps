#pragma once
#include <TFT_eSPI.h>

#include "ButtonsGroupManager.h"
#include "GpsManager.h"
#include "MessageDialog.h"
#include "SensorUtils.h"
#include "TraffipaxAlertController.h"
#include "UIScreen.h"
#include "ValueChangeDialog.h"

/**
 * @brief A főképernyő osztálya, amely a GPS és szenzor adatok megjelenítéséért felelős.
 *
 */
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
    static constexpr uint32_t GRAPH_SAMPLE_INTERVAL_MS = 10UL * 1000UL; // 10 másodpercenként veszünk mintát a trend grafikonhoz
    static constexpr float SPEED_DISPLAY_ON_THRESHOLD_KMPH = 5.0f;
    static constexpr float SPEED_DISPLAY_OFF_THRESHOLD_KMPH = 2.0f;

    // Felső HUD panelek pozíciója és mérete
    static constexpr int16_t TOP_PANEL_Y = 6;
    static constexpr int16_t TOP_PANEL_H = 40;
    static constexpr int16_t TOP_PANEL_LEFT_MARGIN = 0;
    static constexpr int16_t TOP_PANEL_RIGHT_MARGIN = 0;
    static constexpr int16_t TOP_PANEL_GAP = 5;

    // Satellite panel
    static constexpr int16_t SAT_X = TOP_PANEL_LEFT_MARGIN;
    static constexpr int16_t SAT_Y = TOP_PANEL_Y;
    const int16_t SAT_W = tft.width() - TOP_PANEL_LEFT_MARGIN - TOP_PANEL_RIGHT_MARGIN - DATETIME_W - ALT_W - 2 * TOP_PANEL_GAP; // A SAT panel szélessége a maradék helyet foglalja el a TIME és ALT panelek mellett
    static constexpr int16_t SAT_H = TOP_PANEL_H;

    // Dátum/Idő panel
    const int16_t DATETIME_X = SAT_X + SAT_W + TOP_PANEL_GAP;
    static constexpr int16_t DATETIME_Y = 6;
    static constexpr int16_t DATETIME_W = 110;
    static constexpr int16_t DATETIME_H = TOP_PANEL_H;

    // Altitude panel
    static constexpr int16_t ALT_W = 65;
    const int16_t ALT_X = DATETIME_X + DATETIME_W + TOP_PANEL_GAP;
    static constexpr int16_t ALT_Y = 6;
    static constexpr int16_t ALT_H = TOP_PANEL_H;

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
    static constexpr int16_t SENSOR_BAR_Y_BOTTOM = 175;
    static constexpr int16_t SENSOR_BAR_W = 67;
    static constexpr int16_t SENSOR_BAR_H = 118;
    static constexpr int16_t SENSOR_BAR_Y_TOP = SENSOR_BAR_Y_BOTTOM - SENSOR_BAR_H;
    const int16_t SENSOR_BAR_X_RIGHT;

    // Alsó információs sor (a két alsó gomb közötti sáv)
    static constexpr int16_t INFO_X = 56;
    static constexpr int16_t INFO_Y = 216;
    static constexpr int16_t INFO_W = 208;
    static constexpr int16_t INFO_H = 24;

    // Trend grafikon sáv az infosáv fölött
    static constexpr int16_t GRAPH_X = 0;                   // A grafikon bal széle a képernyő bal széléhez igazodik
    static constexpr int16_t GRAPH_Y = 174;                 // A grafikon teteje az infosáv fölött kezdődik
    static constexpr int16_t GRAPH_W = 320;                 // tft.width() -> A grafikon szélessége a teljes képernyő szélességével megegyezik
    static constexpr int16_t GRAPH_H = 40;                  // A grafikon magassága 50 pixel
    static constexpr int16_t GRAPH_SAMPLE_INVALID = -32768; // Érvénytelen mintajelzés a grafikon mintákhoz

    /**
     * @brief A trend grafikon megjelenítési módját meghatározó enum
     */
    enum class GraphMode : uint8_t {
        Off = 0,
        Speed,
        Altitude,
    };

    /**
     * @brief A HUD állapotát tároló struktúra, amely a főképernyő statikus részeinek kirajzolásához szükséges információkat tartalmazza.
     */
    struct ScreenMainHudState {
        bool initialized = false;
        bool staticPainted = false;
        bool speedSpriteReady = false;
        bool sensorBarSpriteReady = false;
        bool graphSpriteReady = false;
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

        char lastDateTimeText[24] = "";
        uint16_t lastDateTimeColor = 0;

        char lastAltText[24] = "";
        uint16_t lastAltColor = 0;
        bool lastAltPanelCompassMode = false;
        float lastCompassNorthDeg = -1000.0f;

        char lastBottomText[128] = "";
    };

    ScreenMainHudState hudState;
    TFT_eSprite speedSprite;
    // Sprite a vertikális bar-oknak
    TFT_eSprite sensorBarSprite;
    TFT_eSprite graphSprite;

    TraffipaxAlertController traffipaxAlertController;

    /**
     * @brief Kényszerített újrarajzolás flag - amikor visszatérünk más képernyőről
     */
    bool forceRedraw = false;

    // Bekapcsolás óta mért legnagyobb (kijelzett) sebesség.
    float maxSpeedSinceBoot = 0.0f;

    // Sebességkijelzés Schmitt-trigger állapota.
    bool speedDisplayMoving = false;

    // Iránytűhöz az utolsó érvényes irányszög cache-elése.
    bool compassHasKnownCourse = false;
    float compassLastKnownCourseDeg = 0.0f;

    // Trend grafikon minták és állapot
    GraphMode trendGraphMode = GraphMode::Speed;

    uint16_t graphSampleCount = 0;
    bool graphDirty = true;
    int16_t speedGraphSamples[GRAPH_W] = {};    // A sebesség minták a trend grafikonhoz, érvénytelen minták esetén GRAPH_SAMPLE_INVALID érték
    int16_t altitudeGraphSamples[GRAPH_W] = {}; // A magasság minták a trend grafikonhoz, érvénytelen minták esetén GRAPH_SAMPLE_INVALID érték

    // Config callback id a leiratkozáshoz
    size_t configCallbackId;

    /**
     * @brief UI komponensek létrehozása és elhelyezése
     */
    void layoutComponents();

    /**
     * @brief A főképernyő statikus részeinek kirajzolása (HUD panelek, sebesség widget, sensor bar-ok, trend grafikon)
     */
    void drawTraffipaxBaseArea();

    // static uint16_t meterColorForRatio(float ratio, bool temperatureBar);

    void drawHudPanel(int16_t x, int16_t y, int16_t w, int16_t h, const char *title, const char *value, uint16_t valueColor);
    void drawHudPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *value, uint16_t valueColor);
    void drawAltitudePanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *value, uint16_t valueColor);
    void drawCompassPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, float northAngleDeg, bool hasValidHeading, uint16_t valueColor);
    void drawTrackPanelValue(int16_t x, int16_t y, int16_t w, int16_t h, const char *satValue, const char *qualityValue, const char *modeValue, uint16_t satColor, bool updateSat, bool updateMeta);
    void ensureGraphSpriteReady();
    void ensureSensorBarSpriteReady();
    void ensureSpeedSpriteReady();
    void recordGraphSample(float speedKmph, bool speedValid, float altitudeM, bool altitudeValid, uint32_t nowMs);
    void drawTrendGraph(bool forceUpdate);
    void updateSpeedValueLayoutForFont();

    void drawStaticHudBackground();
    void drawSpeedWidget(float speedKmph, bool speedValid, bool forceUpdate);
};
