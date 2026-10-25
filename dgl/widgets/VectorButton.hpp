/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DGL_VECTOR_BUTTON_HPP_INCLUDED
#define DGL_VECTOR_BUTTON_HPP_INCLUDED

#include "../Color.hpp"
#include "../EventHandlers.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <cmath>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector button.
   Supports momentary and toggle modes, hover/active states, and an optional LED status indicator.
 */
class VectorButton : public NanoSubWidget,
                     public ButtonEventHandler
{
public:
    enum Mode {
        Momentary = 0,
        Toggle    = 1
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorButtonClicked(VectorButton* button, int mouseButton) {}
        virtual void vectorButtonToggled(VectorButton* button, bool checked) {}
    };

    explicit VectorButton(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          ButtonEventHandler(this),
          fMode(Momentary),
          fNormalColor(Color(36, 38, 42)),
          fHoverColor(Color(48, 51, 56)),
          fActiveColor(Color(25, 26, 29)),
          fCheckedColor(Color(30, 90, 160)),
          fBorderColor(Color(65, 68, 75)),
          fTextColor(Color(230, 232, 235)),
          fLedColor(Color(52, 199, 89)),
          fCornerRadius(4.0f),
          fBorderWidth(1.2f),
          fFontSize(12.0f),
          fHasLed(false),
          fLedActive(false),
          fUserCallback(nullptr)
    {
        ButtonEventHandler::setCallback(&fInternalCallback);
    }

    explicit VectorButton(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          ButtonEventHandler(this),
          fMode(Momentary),
          fNormalColor(Color(36, 38, 42)),
          fHoverColor(Color(48, 51, 56)),
          fActiveColor(Color(25, 26, 29)),
          fCheckedColor(Color(30, 90, 160)),
          fBorderColor(Color(65, 68, 75)),
          fTextColor(Color(230, 232, 235)),
          fLedColor(Color(52, 199, 89)),
          fCornerRadius(4.0f),
          fBorderWidth(1.2f),
          fFontSize(12.0f),
          fHasLed(false),
          fLedActive(false),
          fUserCallback(nullptr)
    {
        ButtonEventHandler::setCallback(&fInternalCallback);
    }

    explicit VectorButton(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          ButtonEventHandler(this),
          fMode(Momentary),
          fNormalColor(Color(36, 38, 42)),
          fHoverColor(Color(48, 51, 56)),
          fActiveColor(Color(25, 26, 29)),
          fCheckedColor(Color(30, 90, 160)),
          fBorderColor(Color(65, 68, 75)),
          fTextColor(Color(230, 232, 235)),
          fLedColor(Color(52, 199, 89)),
          fCornerRadius(4.0f),
          fBorderWidth(1.2f),
          fFontSize(12.0f),
          fHasLed(false),
          fLedActive(false),
          fUserCallback(nullptr)
    {
        ButtonEventHandler::setCallback(&fInternalCallback);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration

    Mode getMode() const noexcept
    {
        return fMode;
    }

    void setMode(const Mode mode) noexcept
    {
        if (fMode == mode)
            return;

        fMode = mode;
        setCheckable(mode == Toggle);
        repaint();
    }

    const String& getLabel() const noexcept
    {
        return fLabel;
    }

    void setLabel(const char* const label)
    {
        fLabel = label;
        repaint();
    }

    bool isDown() const noexcept
    {
        return (getState() & kButtonStateActive) != 0;
    }

    bool isToggled() const noexcept
    {
        return isChecked();
    }

    void setToggled(const bool toggled, const bool sendCallback = false) noexcept
    {
        setChecked(toggled, sendCallback);
        if (fHasLed)
            fLedActive = toggled;
        repaint();
    }

    bool hasLed() const noexcept
    {
        return fHasLed;
    }

    void setHasLed(const bool hasLed) noexcept
    {
        fHasLed = hasLed;
        repaint();
    }

    bool isLedActive() const noexcept
    {
        return fLedActive;
    }

    void setLedActive(const bool active) noexcept
    {
        fLedActive = active;
        repaint();
    }

    void setLedColor(const Color& color) noexcept
    {
        fLedColor = color;
        repaint();
    }

    void setNormalColor(const Color& color) noexcept
    {
        fNormalColor = color;
        repaint();
    }

    void setHoverColor(const Color& color) noexcept
    {
        fHoverColor = color;
        repaint();
    }

    void setActiveColor(const Color& color) noexcept
    {
        fActiveColor = color;
        repaint();
    }

    void setCheckedColor(const Color& color) noexcept
    {
        fCheckedColor = color;
        repaint();
    }

    void setBorderColor(const Color& color) noexcept
    {
        fBorderColor = color;
        repaint();
    }

    void setTextColor(const Color& color) noexcept
    {
        fTextColor = color;
        repaint();
    }

    void setCornerRadius(const float radius) noexcept
    {
        fCornerRadius = radius;
        repaint();
    }

    void setBorderWidth(const float width) noexcept
    {
        fBorderWidth = width;
        repaint();
    }

    void setFontSize(const float size) noexcept
    {
        fFontSize = size;
        repaint();
    }

    void setCallback(Callback* const cb) noexcept
    {
        fUserCallback = cb;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // NanoVG Rendering

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 1.0f || h <= 1.0f)
            return;

        const State st = getState();
        const bool isHover = (st & kButtonStateHover) != 0;
        const bool isActive = (st & kButtonStateActive) != 0;
        const bool isTog = (fMode == Toggle && isChecked());

        // Determine background color
        Color bgColor;
        if (isActive)
            bgColor = fActiveColor;
        else if (isTog)
            bgColor = fCheckedColor;
        else if (isHover)
            bgColor = fHoverColor;
        else
            bgColor = fNormalColor;

        // 1. Background
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        fillColor(bgColor);
        fill();

        // 2. Border
        Color bColor = fBorderColor;
        if (isActive || isTog)
            bColor = Color(bColor.red + 30, bColor.green + 30, bColor.blue + 30);
        else if (isHover)
            bColor = Color(bColor.red + 20, bColor.green + 20, bColor.blue + 20);

        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        strokeColor(bColor);
        strokeWidth(fBorderWidth);
        stroke();

        float textStartX = 0.0f;
        float textWidth = w;

        // 3. Optional LED Indicator
        if (fHasLed)
        {
            const float ledRadius = std::min(4.0f, h * 0.18f);
            const float ledX = 4.0f + ledRadius * 2.0f;
            const float ledY = h * 0.5f;

            textStartX = ledX + ledRadius + 4.0f;
            textWidth = w - textStartX;

            const bool lit = (fMode == Toggle) ? (isChecked() || fLedActive) : fLedActive;

            // Halo glow when active
            if (lit)
            {
                beginPath();
                circle(ledX, ledY, ledRadius * 1.8f);
                fillColor(Color(fLedColor.red, fLedColor.green, fLedColor.blue, 60));
                fill();
            }

            beginPath();
            circle(ledX, ledY, ledRadius);
            if (lit)
                fillColor(fLedColor);
            else
                fillColor(Color(fLedColor.red / 4, fLedColor.green / 4, fLedColor.blue / 4, 180));
            fill();

            beginPath();
            circle(ledX, ledY, ledRadius);
            strokeColor(Color(0, 0, 0, 100));
            strokeWidth(0.8f);
            stroke();
        }

        // 4. Label text
        if (fLabel.isNotEmpty())
        {
            fontSize(fFontSize);
            fillColor(fTextColor);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

            const float textCenterX = fHasLed ? (textStartX + textWidth * 0.5f - 4.0f) : (w * 0.5f);
            const float textCenterY = (h * 0.5f) + (isActive ? 1.0f : 0.0f);

            text(textCenterX, textCenterY, fLabel.buffer(), nullptr);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Event Forwarding

    bool onMouse(const MouseEvent& ev) override
    {
        const bool handled = ButtonEventHandler::mouseEvent(ev);
        repaint();
        return handled;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        const bool handled = ButtonEventHandler::motionEvent(ev);
        repaint();
        return handled;
    }

protected:
    void stateChanged(State state, State oldState) override
    {
        ButtonEventHandler::stateChanged(state, oldState);
        repaint();
    }

private:
    Mode fMode;
    Color fNormalColor;
    Color fHoverColor;
    Color fActiveColor;
    Color fCheckedColor;
    Color fBorderColor;
    Color fTextColor;
    Color fLedColor;
    float fCornerRadius;
    float fBorderWidth;
    float fFontSize;
    bool fHasLed;
    bool fLedActive;
    String fLabel;
    Callback* fUserCallback;

    struct InternalCallback : ButtonEventHandler::Callback {
        VectorButton* self;
        InternalCallback(VectorButton* const s) : self(s) {}

        void buttonClicked(SubWidget*, const int button) override
        {
            if (self->fMode == Toggle)
            {
                if (self->fHasLed)
                    self->fLedActive = self->isChecked();
                if (self->fUserCallback != nullptr)
                    self->fUserCallback->vectorButtonToggled(self, self->isChecked());
            }

            if (self->fUserCallback != nullptr)
                self->fUserCallback->vectorButtonClicked(self, button);

            self->repaint();
        }
    } fInternalCallback{this};

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorButton)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_BUTTON_HPP_INCLUDED
