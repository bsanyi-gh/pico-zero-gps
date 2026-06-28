#include "ScreenInfo.h"
#include "Config.h"
#include "Utils.h"
#include "defines.h"

#include "TraffipaxManager.h"
extern TraffipaxManager traffipaxManager;

#include "GpsManager.h"

#include "SensorUtils.h"
extern SensorUtils sensorUtils;

/**
 * @brief ScreenInfo konstruktor
 */
ScreenInfo::ScreenInfo() : UIScreen(SCREEN_NAME_INFO) {

    // Padding kiszámítása
    tft.setFreeFont();
    tft.setTextSize(1);
    textPadding = tft.textWidth("8888-88-88");
    bootTextPadding = tft.textWidth("88 mins, 88 sec, 888 msec");

    lastUpdated = 0;

    // Komponensek elrendezése
    layoutComponents();
}

/**
 * UI komponensek elhelyezése
 */
void ScreenInfo::layoutComponents() {

    // A Sats gomb jobb a 'Back' gomb előtt
    addChild(std::make_shared<UIButton>(                           //
        1,                                                         //
        Rect(tft.width() - 2 * (UIButton::DEFAULT_BUTTON_WIDTH)-4, // x gap 4 pixel
             tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT,       // y
             UIButton::DEFAULT_BUTTON_WIDTH,                       // width
             UIButton::DEFAULT_BUTTON_HEIGHT),                     // height
        "Sats",                                                    //
        UIButton::ButtonType::Pushable,
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->switchToScreen(SCREEN_NAME_SATS);
            }
        }) //
    );

    // 'Back' gomb a jobb alsó sarokban
    addChild(std::make_shared<UIButton>(                     //                                                                                                                                  //
        2,                                                   //
        Rect(tft.width() - UIButton::DEFAULT_BUTTON_WIDTH,   // x
             tft.height() - UIButton::DEFAULT_BUTTON_HEIGHT, // y
             UIButton::DEFAULT_BUTTON_WIDTH,                 // width
             UIButton::DEFAULT_BUTTON_HEIGHT),               // height
        "Back",                                              //
        UIButton::ButtonType::Pushable,                      //
        [this](const UIButton::ButtonEvent &event) {
            if (event.state == UIButton::EventButtonState::Clicked) {
                getScreenManager()->goBack();
            }
        }) //
    );
}

/**
 * Kirajzolja a képernyő saját STATIKUS tartalmát
 */
void ScreenInfo::drawContent() {
    // Háttér törlése
    tft.fillScreen(TFT_BLACK);

    // Címsor
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Info Screen", tft.width() / 2, 12);

    // Program neve és verziója
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    snprintf(valueBuffer, sizeof(valueBuffer), "GPS Speedometer %s", PROGRAM_VERSION);
    tft.drawString(valueBuffer, tft.width() / 2, 30);
    // Author
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(PROGRAM_AUTHOR, tft.width() / 2, 50);

    // Táblázat prompt
    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextPadding(0);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

    constexpr uint8_t lineHeight = 10;

    // 1. oszlop - Prompt
    uint16_t tableX = 50;
    uint16_t tableY = 70;

    tft.drawString("CPU", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("TFT", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Temp mod", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("GPS mod", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Compiled", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("DB size", tableX, tableY);

    // 2. oszlop - Prompt
    tableX = 250;
    tableY = 70;
    tft.drawString("CPU Clock", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Total Heap", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Free Heap", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Used Heap", tableX, tableY);
    tableY += lineHeight;

    // Táblázat Értékek
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);

    // 1. oszlop értékek
    tableX = 60;
    tableY = 70;
    tft.drawString("RP Pico Zero", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("ILI9341", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("DS18B20", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("uBlox Neo-6M/8M", tableX, tableY);
    tableY += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%s %s", __DATE__, __TIME__);
    tft.drawString(valueBuffer, tableX, tableY);
    tableY += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%d", ::traffipaxManager.count());
    tft.drawString(valueBuffer, tableX, tableY);

    // 2. oszlop értékek
    tableX = 260;
    tableY = 70;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.1f MHz", rp2040.f_cpu() / 1000000.0f);
    tft.drawString(valueBuffer, tableX, tableY);
    tableY += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.1f kB", rp2040.getTotalHeap() / 1024.0f);
    tft.drawString(valueBuffer, tableX, tableY);
    tableY += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.1f kB", rp2040.getFreeHeap() / 1024.0f);
    tft.drawString(valueBuffer, tableX, tableY);
    tableY += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.1f kB", rp2040.getUsedHeap() / 1024.0f);
    tft.drawString(valueBuffer, tableX, tableY);
    tableY += lineHeight;

    //-------------------------------------------------------------------------------------------

    // 2. tábla 2 oszlop prompt
    // Azért ilyen sorrendben, mert az MR_DATUM törli az előtte lévő teljes tartalmat, így az 1. tábla promptokat is
    tableX = 250;
    tableY = 135;
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);

    tft.drawString("Accumulator", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Battery", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Temp External", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Temp CPU Core", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Boot time", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Mode", tableX, tableY);
    tableY += lineHeight;

    // 2. tábla 1 oszlop prompt
    tableX = 50;
    tableY = 135;

    tft.drawString("Sats", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Sats DB", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Lat", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Lng", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Alt", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Quality", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Mode", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("Speed", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("HDop", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("GPS Date", tableX, tableY);
    tableY += lineHeight;
    tft.drawString("GPS Time", tableX, tableY);
    tableY += lineHeight;
}

/**
 * Kezeli a képernyő saját ciklusát (dinamikus frissítés)
 */
void ScreenInfo::handleOwnLoop() {

    // 5 másodperces frissítés
    if (!Utils::timeHasPassed(lastUpdated, 5000)) {
        return;
    }
    lastUpdated = millis();

    tft.setFreeFont();
    tft.setTextSize(1);
    tft.setTextPadding(textPadding);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);

    // 2. oszlop
    uint16_t x = 260;
    uint16_t y = 135;
    constexpr uint8_t lineHeight = 10;

    snprintf(valueBuffer, sizeof(valueBuffer), "%.2fV", sensorUtils.readVBusExternal());
    // dtostrf(sensorUtils.readVBusExternal(), 0, 2, valueBuffer);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.2fV", sensorUtils.readVSysExternal());
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.2fC", sensorUtils.readExternalTemperature());
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.2fC", sensorUtils.readCoreTemperature());
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;

    // Nagyobb padding
    tft.setTextPadding(bootTextPadding);
    if (c1_sharedGpsData.gpsBootTime == 0) {
        snprintf(valueBuffer, sizeof(valueBuffer), "--:--");
    } else {
        Utils::secToMinSecString(c1_sharedGpsData.gpsBootTime, valueBuffer, sizeof(valueBuffer));
    }
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;

    // Padding vissza
    tft.setTextPadding(textPadding);

    // Demo/Normal mód jelzés
    tft.drawString("Normal", x, y);
    y += lineHeight;

    // 1. oszlop
    x = 60;
    y = 135;

    snprintf(valueBuffer, sizeof(valueBuffer), "%u", c1_sharedGpsData.satelliteCount);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%u", c1_sharedGpsData.satelliteCountForUI);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.6f", c1_sharedGpsData.lat);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.6f", c1_sharedGpsData.lng);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.1f m", c1_sharedGpsData.altitudeM);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    tft.drawString(GpsManager::qualityToString(c1_sharedGpsData.fixQuality), x, y);
    y += lineHeight;
    tft.drawString(GpsManager::modeToString(c1_sharedGpsData.fixMode), x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.1f km/h", c1_sharedGpsData.speedKmph);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
    snprintf(valueBuffer, sizeof(valueBuffer), "%.2f", c1_sharedGpsData.hdop);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;

    snprintf(valueBuffer, sizeof(valueBuffer), "%04d-%02d-%02d", c1_sharedGpsData.year, c1_sharedGpsData.month, c1_sharedGpsData.day);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;

    snprintf(valueBuffer, sizeof(valueBuffer), "%02d:%02d:%02d", c1_sharedGpsData.hour, c1_sharedGpsData.minute, c1_sharedGpsData.second);
    tft.drawString(valueBuffer, x, y);
    y += lineHeight;
}