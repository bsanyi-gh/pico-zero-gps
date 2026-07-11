#pragma once

#include <algorithm> // For std::max
#include <functional>

#include "Config.h"
#include "UIComponent.h"
#include "utils.h"

/**
 * @brief UI Button komponens
 *
 * Egyszerű gomb komponens, amely kezeli a lenyomást és a megjelenítést.
 */
class UIButton : public UIComponent {

  public:
    // Alapértelmezett gomb méretek
    static constexpr uint16_t DEFAULT_BUTTON_WIDTH = 48;       // Alapértelmezett gomb szélesség
    static constexpr uint16_t DEFAULT_BUTTON_HEIGHT = 24;      // Alapértelmezett gomb magasság
    static constexpr uint16_t HORIZONTAL_TEXT_PADDING = 2 * 8; // 8px padding a szöveg mindkét oldalán
    static constexpr uint16_t BUTTON_TOUCH_MARGIN = 6;         // Gomb érintési érzékenység margója

    // Gomb típusok
    enum class ButtonType {
        Pushable,  // Egyszerű nyomógomb
        Toggleable // Váltógomb (on/off)
    };

    // Gomb esemény állapotok
    enum class EventButtonState {
        Off = 0,
        On,
        Disabled,      // Állapot riportáláshoz
        CurrentActive, // Jelenleg aktív mód jelzése
        Clicked,       // Lenyomás történt (Pushable gomboknál)
        LongPressed    // Hosszú nyomás
    };

    // Gomb esemény struktúra
    struct ButtonEvent {
        uint8_t id;
        const char *label;
        EventButtonState state;
        ButtonEvent(uint8_t id, const char *label, EventButtonState state) : id(id), label(label), state(state) {}
    };

    // Gomb logikai állapotai (a Disabled állapotot az ősosztály kezeli)
    enum class ButtonState { Off, On, CurrentActive, Disabled };

  private:
    static constexpr uint8_t CORNER_RADIUS = 5;
    static constexpr uint32_t LONG_PRESS_THRESHOLD = 1000; // ms

    uint8_t buttonId;
    const char *label;
    ButtonType buttonType = ButtonType::Pushable;
    ButtonState currentState = ButtonState::Off;
    bool autoSizeToText = false;
    bool useMiniFont = true; // Alapértelmezett a kisebb font használata, hogy a gombok kompaktabbak legyenek
    uint32_t pressStartTime = 0;
    bool longPressThresholdMet = false; // Jelzi, ha a hosszú lenyomás küszöbét elértük

    ButtonColorScheme currentButtonScheme; // Gomb-specifikus színséma
    std::function<void(const ButtonEvent &)> eventCallback;
    std::function<void()> clickCallback; // Backward compatibility

    // Gomb állapot színek
    struct StateColors {
        uint16_t background;
        uint16_t border;
        uint16_t text;
        uint16_t led;
    };

    /**
     * @brief Lekéri a gomb aktuális állapotának megfelelő színeket
     * @return StateColors struktúra, amely tartalmazza a háttér, keret, szöveg és LED színeket
     */
    StateColors getStateColors() const {
        StateColors resultColors;

        if (isDisabled()) {
            resultColors.background = this->currentButtonScheme.disabledBackground;
            resultColors.border = this->currentButtonScheme.disabledBorder;
            resultColors.text = this->currentButtonScheme.disabledForeground;
            resultColors.led = this->currentButtonScheme.ledOffColor;
        } else if (this->pressed) {
            resultColors.background = this->currentButtonScheme.pressedBackground;
            resultColors.border = this->currentButtonScheme.pressedBorder;
            resultColors.text = this->currentButtonScheme.pressedForeground;

            if (buttonType == ButtonType::Toggleable) {
                resultColors.led = (currentState == ButtonState::On) ? this->currentButtonScheme.ledOnColor : this->currentButtonScheme.ledOffColor;
            } else { // Pushable
                resultColors.led = TFT_BLACK;
            }
        } else {
            if (currentState == ButtonState::On) {
                resultColors.background = this->currentButtonScheme.activeBackground;
                resultColors.border = this->currentButtonScheme.ledOnColor;
                resultColors.text = this->currentButtonScheme.activeForeground;
                resultColors.led = this->currentButtonScheme.ledOnColor;
            } else if (currentState == ButtonState::CurrentActive) {
                resultColors.background = this->currentButtonScheme.activeBackground;
                resultColors.border = TFT_BLUE;
                resultColors.text = this->currentButtonScheme.activeForeground;
                resultColors.led = TFT_BLACK;
            } else { // ButtonState::Off
                resultColors.background = this->currentButtonScheme.background;
                resultColors.border = this->currentButtonScheme.border;
                resultColors.text = this->currentButtonScheme.foreground;
                resultColors.led = (buttonType == ButtonType::Toggleable) ? this->currentButtonScheme.ledOffColor : TFT_BLACK;
            }
        }
        return resultColors;
    }

    /**
     * @brief Sötétíti a megadott színt a megadott mértékben
     * @param color A szín, amelyet sötétíteni szeretnénk (16 bites RGB565 formátumban)
     * @param amount A sötétítés mértéke (0-255), ahol 0 nem változtatja a színt, 255 teljesen fekete lesz
     * @return A sötétített szín (16 bites RGB565 formátumban)
     * @note A sötétítés lineárisan csökkenti a vörös, zöld és kék komponenseket a megadott mértékben.
     */
    uint16_t darkenColor(uint16_t color, uint8_t amount) const {
        uint8_t r = (color & 0xF800) >> 11;
        uint8_t g = (color & 0x07E0) >> 5;
        uint8_t b = (color & 0x001F);
        uint8_t darkenAmount = amount > 0 ? (amount >> 3) : 0;
        r = (r > darkenAmount) ? r - darkenAmount : 0;
        g = (g > darkenAmount) ? g - darkenAmount : 0;
        b = (b > darkenAmount) ? b - darkenAmount : 0;
        return (r << 11) | (g << 5) | b;
    }

    /**
     * @brief Rajzolja a lenyomott effektust a gombon
     * @param baseColorForEffect Az alap szín, amelyből a lenyomott effektust számítjuk
     * @note A lenyomott effektus sötétített színekből álló lépcsős hatás.
     */
    void drawPressedEffect(uint16_t baseColorForEffect) {
        const uint8_t steps = 6;
        uint8_t stepWidth = bounds.width / steps;
        uint8_t stepHeight = bounds.height / steps;
        for (uint8_t i = 0; i < steps; i++) {
            uint16_t fadedColor = darkenColor(baseColorForEffect, i * 30);
            tft.fillRoundRect(bounds.x + i * stepWidth / 2, bounds.y + i * stepHeight / 2, bounds.width - i * stepWidth, bounds.height - i * stepHeight, CORNER_RADIUS, fadedColor);
        }
    }

    /**
     * @brief Frissíti a gomb szélességét a szöveghez igazítva, ha az autoSizeToText engedélyezve van
     * @note Ha a gomb szélessége 0, és az autoSizeToText engedélyezve van, a gomb szélessége a szöveg szélességéhez igazodik, figyelembe véve a vízszintes paddinget.
     */
    void updateWidthToFitText() {
        if (!autoSizeToText || label == nullptr) {
            return;
        }

        uint8_t prevDatum = tft.getTextDatum();
        uint8_t currentTftTextSize = tft.textsize;

        tft.setTextSize(1);
        if (useMiniFont) {
            tft.setFreeFont();
        } else {
            tft.setFreeFont(&FreeSans9pt7b);
        }

        uint16_t textW = (strlen(label) > 0) ? tft.textWidth(label) : 0;
        uint16_t newWidth = textW + HORIZONTAL_TEXT_PADDING;

        newWidth = std::max(newWidth, static_cast<uint16_t>(bounds.height > 0 ? bounds.height : DEFAULT_BUTTON_HEIGHT));
        newWidth = std::max(newWidth, static_cast<uint16_t>(DEFAULT_BUTTON_WIDTH / 2));

        if (bounds.width != newWidth) {
            bounds.width = newWidth;
            markForRedraw();
        }

        tft.setTextSize(currentTftTextSize);
        tft.setTextDatum(prevDatum);
    }

  private:
    /**
     * @brief Privát konstruktor a UIButton osztályhoz, amelyet a publikus konstruktorok hívnak.
     * @param id A gomb azonosítója
     * @param bounds A gomb határai (Rect)
     * @param label A gombon megjelenítendő szöveg
     * @param type A gomb típusa (ButtonType)
     * @param initialState A gomb kezdeti állapota (ButtonState)
     * @param initiallyDisabled A gomb kezdeti tiltott állapota
     * @param callback A gomb eseménykezelője
     * @param scheme A gomb színpalettája
     * @param autoSize A gomb automatikus méretezésének engedélyezése
     */
    UIButton(uint8_t id, const Rect &bounds, const char *label, ButtonType type, ButtonState initialState, bool initiallyDisabled, std::function<void(const ButtonEvent &)> callback, const ButtonColorScheme &scheme,
             bool autoSize)
        : UIComponent(Rect(bounds.x, bounds.y, (bounds.width == 0 && !autoSize ? DEFAULT_BUTTON_WIDTH : bounds.width), (bounds.height == 0 ? DEFAULT_BUTTON_HEIGHT : bounds.height)), scheme), buttonId(id), label(label),
          buttonType(type), currentState(initialState), currentButtonScheme(scheme), eventCallback(callback), autoSizeToText(autoSize) {

        if (initiallyDisabled) {
            this->disabled = true; // Közvetlen beállítás a redraw elkerülése érdekében
        }

        if (buttonType == ButtonType::Pushable && currentState == ButtonState::On) {
            currentState = ButtonState::Off;
        }

        if (autoSizeToText) {
            updateWidthToFitText();
        }
    }

  public:
    /**
     * @brief Kiszámolja a gomb szélességét a szöveghez igazítva, figyelembe véve a paddinget és az alapértelmezett méreteket
     * @param text A gombon megjelenítendő szöveg
     * @param btnUseMiniFont A mini betűtípus használata
     * @param currentButtonHeight A gomb aktuális magassága
     * @return A számított gomb szélesség
     */
    static uint16_t calculateWidthForText(const char *text, bool btnUseMiniFont, uint16_t currentButtonHeight) {
        if (text == nullptr)
            text = "";
        uint8_t prevDatum = ::tft.getTextDatum();
        uint8_t currentTftTextSize = ::tft.textsize;
        ::tft.setTextSize(1);
        if (btnUseMiniFont) {
            ::tft.setFreeFont();
        } else {
            ::tft.setFreeFont(&FreeSans9pt7b);
        }
        uint16_t textW = strlen(text) > 0 ? ::tft.textWidth(text) : 0;
        uint16_t calculatedWidth = textW + HORIZONTAL_TEXT_PADDING;
        uint16_t minHeight = (currentButtonHeight > 0) ? currentButtonHeight : DEFAULT_BUTTON_HEIGHT;
        calculatedWidth = std::max(calculatedWidth, minHeight);
        calculatedWidth = std::max(calculatedWidth, static_cast<uint16_t>(DEFAULT_BUTTON_WIDTH / 2));
        ::tft.setTextSize(currentTftTextSize);
        ::tft.setTextDatum(prevDatum);
        return calculatedWidth;
    }

    /**
     * @brief Elsődleges gomb konstruktor.
     * @param initiallyDisabled Opcionális: A gomb letiltott állapottal induljon-e.
     * @param id A gomb azonosítója
     * @param bounds A gomb határai (Rect)
     * @param label A gombon megjelenítendő szöveg
     * @param type A gomb típusa (ButtonType)
     * @param initialState A gomb kezdeti állapota (ButtonState)
     * @param callback A gomb eseménykezelője
     * @param scheme A gomb színpalettája
     * @param autoSizeToText A gomb automatikus méretezésének engedélyezése
     * @param initiallyDisabled A gomb kezdeti tiltott állapota
     */
    UIButton(uint8_t id, const Rect &bounds, const char *label, ButtonType type = ButtonType::Pushable, ButtonState initialState = ButtonState::Off, std::function<void(const ButtonEvent &)> callback = nullptr,
             const ButtonColorScheme &scheme = UIColorPalette::createDefaultButtonScheme(), bool autoSizeToText = false, bool initiallyDisabled = false)
        : UIButton(id, bounds, label, type, initialState, initiallyDisabled, callback, scheme, autoSizeToText) {}

    /**
     * @brief Konstruktor a callback és típus kötelező megadásával.
     * @param id A gomb azonosítója
     * @param bounds A gomb határai (Rect)
     * @param label A gombon megjelenítendő szöveg
     * @param type A gomb típusa (ButtonType)
     * @param callback A gomb eseménykezelője
     * @param scheme A gomb színpalettája
     * @param autoSizeToText A gomb automatikus méretezésének engedélyezése
     */
    UIButton(uint8_t id, const Rect &bounds, const char *label, ButtonType type, std::function<void(const ButtonEvent &)> callback, const ButtonColorScheme &scheme = UIColorPalette::createDefaultButtonScheme(),
             bool autoSizeToText = false)
        : UIButton(id, bounds, label, type, ButtonState::Off, false, callback, scheme, autoSizeToText) {}

    /**
     * @brief Egyszerűsített konstruktor "Pushable" gombokhoz.
     * @param id A gomb azonosítója
     * @param bounds A gomb határai (Rect)
     * @param label A gombon megjelenítendő szöveg
     * @param callback A gomb eseménykezelője
     * @param autoSizeToText A gomb automatikus méretezésének engedélyezése
     */
    UIButton(uint8_t id, const Rect &bounds, const char *label, std::function<void(const ButtonEvent &)> callback = nullptr, bool autoSizeToText = false)
        : UIButton(id, bounds, label, ButtonType::Pushable, ButtonState::Off, false, callback, UIColorPalette::createDefaultButtonScheme(), autoSizeToText) {}

    // ================================
    // Getters/Setters
    // ================================

    uint8_t getId() const { return buttonId; }
    void setId(uint8_t id) { buttonId = id; }

    ButtonType getButtonType() const { return buttonType; }
    void setButtonType(ButtonType type) {
        if (buttonType != type) {
            buttonType = type;
            markForRedraw();
        }
    }

    ButtonState getButtonState() const { return currentState; }

    /**
     * @brief Gomb logikai állapotának beállítása. A letiltást a setEnabled() kezeli.
     * @param newState A gomb új állapota (ButtonState)
     */
    void setButtonState(ButtonState newState) {
        if (buttonType == ButtonType::Pushable && newState == ButtonState::On) {
            newState = ButtonState::Off;
        }

        if (currentState != newState) {
            currentState = newState;
            markForRedraw();
        }
    }

    /**
     * @brief Gomb engedélyezése vagy letiltása.
     * @param enable true, ha a gomb engedélyezett, false, ha letiltott
     */
    void setEnabled(bool enable) {
        // Az ősosztály setDisabled metódusa kezeli a 'disabled' flag-et és a redraw-t.
        UIComponent::setDisabled(!enable);
    }

    /**
     * @brief Gomb automatikus méretezésének engedélyezése vagy letiltása.
     * @param enable true, ha a gomb automatikusan méreteződik a szöveghez, false, ha nem
     */
    void setAutoSizeToText(bool enable) {
        if (autoSizeToText != enable) {
            autoSizeToText = enable;
            if (autoSizeToText) {
                updateWidthToFitText();
            }
            markForRedraw();
        }
    }

    /**
     * @brief Lekéri, hogy a gomb automatikusan méreteződik-e a szöveghez.
     * @return true, ha a gomb automatikusan méreteződik, false, ha nem
     */
    bool getAutoSizeToText() const { return autoSizeToText; }

    void setLabel(const char *newLabel) {
        label = newLabel;
        if (autoSizeToText) {
            updateWidthToFitText();
        } else {
            markForRedraw();
        }
    }

    const char *getLabel() const { return label; }

    /**
     * @brief Beállítja, hogy a gomb mini betűtípust használjon-e.
     * @param mini true, ha a gomb mini betűtípust használ, false, ha nem
     */
    void setUseMiniFont(bool mini) {
        if (useMiniFont != mini) {
            useMiniFont = mini;
            if (autoSizeToText) { // A méret is változhat
                updateWidthToFitText();
            } else {
                markForRedraw();
            }
        }
    }

    /**
     * @brief Lekéri, hogy a gomb mini betűtípust használ-e.
     * @return true, ha a gomb mini betűtípust használ, false, ha nem
     */
    bool isUseMiniFont() const { return useMiniFont; }

    void setEventCallback(std::function<void(const ButtonEvent &)> callback) { eventCallback = callback; }

    void setClickCallback(std::function<void()> callback) { clickCallback = callback; }

    /**
     * @brief Beállítja a gombot alapértelmezett választógombként, amely letiltott állapotban van, és a színsémája a választógombokhoz van optimalizálva.
     * @note Ez a metódus a gombot letiltott állapotba helyezi, és a színsémát a választógombokhoz optimalizálja. A gomb nem lesz interaktív, de vizuálisan kiemelkedik.
     */
    void setAsDefaultChoiceButton() {
        setEnabled(false);
        ColorScheme baseScheme = UIColorPalette::createDefaultChoiceButtonScheme();
        ButtonColorScheme btnScheme(baseScheme, baseScheme.activeBorder, TFT_DARKGREY);
        setButtonColorScheme(btnScheme);
    }

    /**
     * @brief Rajzolja a gombot a képernyőre.
     * @note A rajzolás a gomb aktuális állapotától függően történik, beleértve a lenyomott, engedélyezett, letiltott és váltógomb állapotokat.
     */
    virtual void draw() override {
        if (!needsRedraw)
            return;

        StateColors currentDrawColors = getStateColors();

        if (this->pressed) {
            drawPressedEffect(currentDrawColors.background);
        } else {
            tft.fillRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, CORNER_RADIUS, currentDrawColors.background);
        }

        tft.drawRoundRect(bounds.x, bounds.y, bounds.width, bounds.height, CORNER_RADIUS, currentDrawColors.border);

        // Mini-fontos Toggleable ON gombnál extra belső keret, hogy 2px vastag legyen a zöld kiemelés.
        if (!isDisabled() && buttonType == ButtonType::Toggleable && useMiniFont && currentState == ButtonState::On && bounds.width > 2 && bounds.height > 2) {
            uint8_t innerRadius = (CORNER_RADIUS > 0) ? static_cast<uint8_t>(CORNER_RADIUS - 1) : 0;
            tft.drawRoundRect(bounds.x + 1, bounds.y + 1, bounds.width - 2, bounds.height - 2, innerRadius, this->currentButtonScheme.ledOnColor);
        }

        if (label != nullptr) {
            tft.setTextSize(1);
            if (useMiniFont) {
                tft.setFreeFont();
            } else {
                tft.setFreeFont(&FreeSans9pt7b);
            }

            tft.setTextColor(currentDrawColors.text);
            tft.setTextDatum(MC_DATUM);

            bool willHaveLed = (buttonType == ButtonType::Toggleable && !useMiniFont && currentDrawColors.led != TFT_BLACK);
            int16_t textY = bounds.centerY();
            if (useMiniFont)
                textY += 1;

            if (willHaveLed) {
                constexpr uint8_t LED_HEIGHT = 5;
                constexpr uint8_t LED_GAP = 3;
                int16_t ledTopY = bounds.y + bounds.height - LED_HEIGHT - 3;
                int16_t desiredTextBottomY = ledTopY - LED_GAP;
                int16_t textHeight = tft.fontHeight(); // Robusztusabb magasság lekérdezés
                int16_t adjustedTextY = desiredTextBottomY - (textHeight / 2);

                if (adjustedTextY < textY) {
                    textY = adjustedTextY;
                }
            }

            tft.drawString(label, bounds.centerX(), textY);
        }

        if (buttonType == ButtonType::Toggleable && !useMiniFont && currentDrawColors.led != TFT_BLACK) {
            constexpr uint8_t LED_HEIGHT = 5;
            constexpr uint8_t LED_MARGIN = 10;
            tft.fillRect(bounds.x + LED_MARGIN, bounds.y + bounds.height - LED_HEIGHT - 3, bounds.width - 2 * LED_MARGIN, LED_HEIGHT, currentDrawColors.led);
        }

        needsRedraw = false;
    }

    /**
     * @brief Beállítja a gomb színsémáját.
     * @param newScheme A gomb új színsémája (ButtonColorScheme)
     */
    void setButtonColorScheme(const ButtonColorScheme &newScheme) {
        this->currentButtonScheme = newScheme;
        UIComponent::setColorScheme(newScheme);
        markForRedraw();
    }

  protected:
    /**
     * @brief Érintés esemény kezelése, amikor a gombot lenyomják.
     * @param event Az érintés esemény adatai (TouchEvent)
     */
    virtual void onTouchDown(const TouchEvent &event) override {
        UIComponent::onTouchDown(event);
        if (isDisabled())
            return;

        longPressThresholdMet = false;
        pressStartTime = millis();
    }

    /**
     * @brief Érintés esemény kezelése, amikor a gombot felengedik.
     * @param event Az érintés esemény adatai (TouchEvent)
     */
    virtual void onTouchUp(const TouchEvent &event) override {
        UIComponent::onTouchUp(event);
        // A logika átkerült az onClick-be, itt már csak a flag-ek resetelése történik,
        // amit az onTouchCancel és az onClick is elvégez.
    }

    /**
     * @brief Gomb kattintás esemény kezelése. Eldönti, hogy rövid vagy hosszú lenyomás történt-e.
     *
     * @param event Az érintés esemény adatai (TouchEvent)
     */
    virtual bool onClick(const TouchEvent &event) override {
        UIComponent::onClick(event);
        if (isDisabled())
            return false;

        bool wasLongPress = longPressThresholdMet;

        // Flag-ek azonnali törlése a következő interakcióhoz.
        pressStartTime = 0;
        longPressThresholdMet = false;

        if (wasLongPress) {
            if (eventCallback) {
                DEBUG("UIButton: Long press event fired for button %d (%s)\n", buttonId, label);
                eventCallback(ButtonEvent(buttonId, label, EventButtonState::LongPressed));
            }
        } else {
            // Normál (rövid) kattintás
            if (buttonType == ButtonType::Toggleable) {
                currentState = (currentState == ButtonState::Off || currentState == ButtonState::CurrentActive) ? ButtonState::On : ButtonState::Off;
                if (eventCallback) {
                    eventCallback(ButtonEvent(buttonId, label, (currentState == ButtonState::On) ? EventButtonState::On : EventButtonState::Off));
                }
            } else { // Pushable
                if (eventCallback) {
                    eventCallback(ButtonEvent(buttonId, label, EventButtonState::Clicked));
                }
            }

            if (clickCallback) { // Visszafelé kompatibilitás
                clickCallback();
            }
        }

        markForRedraw();

        if (config.data.beeperEnabled) {
            Utils::beepTick();
        }

        return true; // Kezeltük a kattintást
    }

    /**
     * @brief Gomb lenyomás megszakítása (pl. ujj lecsúszik).
     * @param event Az érintés esemény adatai (TouchEvent)
     */
    virtual void onTouchCancel(const TouchEvent &event) override {
        UIComponent::onTouchCancel(event);
        if (isDisabled())
            return;

        // Lenyomás megszakadt, töröljük a flag-eket.
        pressStartTime = 0;
        longPressThresholdMet = false;
    }

  public:
    virtual bool allowsVisualPressedFeedback() const override { return true; }

    /**
     * @brief Gomb állapotának frissítése a hosszú lenyomás küszöb ellenőrzésével.
     * @note Ha a gomb le van nyomva, és a hosszú lenyomás küszöbét elértük, a gomb vizuális visszajelzést ad a hosszú lenyomásról.
     */
    virtual void loop() override {
        UIComponent::loop();
        if (isDisabled() || !this->pressed)
            return;

        if (!longPressThresholdMet && pressStartTime > 0) {
            if (millis() - pressStartTime >= LONG_PRESS_THRESHOLD) {
                longPressThresholdMet = true;
                // Vizuális visszajelzés arról, hogy a hosszú lenyomás "élesítve" van.
                markForRedraw();
            }
        }
    }

    /**
     * @brief Lekéri a gomb érintési margóját.
     * @return A gomb érintési margója (BUTTON_TOUCH_MARGIN)
     */
    virtual int16_t getTouchMargin() const override { return BUTTON_TOUCH_MARGIN; }
};
