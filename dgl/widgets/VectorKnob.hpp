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

#ifndef DGL_VECTOR_KNOB_HPP_INCLUDED
#define DGL_VECTOR_KNOB_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../EventHandlers.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <cmath>
#include <cstdio>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector rotary knob.
 */
class VectorKnob : public NanoSubWidget,
                   public KnobEventHandler,
                   public AccessibleElement
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorKnobDragStarted(VectorKnob* knob) {}
        virtual void vectorKnobDragFinished(VectorKnob* knob) {}
        virtual void vectorKnobValueChanged(VectorKnob* knob, float value) {}
        virtual void vectorKnobDoubleClicked(VectorKnob* knob) {}
        virtual void vectorKnobContextMenu(VectorKnob* knob, int32_t x, int32_t y) { (void)knob; (void)x; (void)y; }
    };

    typedef void (*ContextMenuCallback)(Widget* widget, int32_t x, int32_t y, void* userData);

    void setContextMenuCallback(ContextMenuCallback cb, void* userData = nullptr) noexcept
    {
        fContextMenuCb = cb;
        fContextMenuUserData = userData;
    }

    explicit VectorKnob(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          KnobEventHandler(this),
          fTrackColor(Color(42, 44, 48)),
          fActiveColor(Color(52, 152, 219)),
          fDialColor(Color(26, 27, 30)),
          fDialBorderColor(Color(60, 62, 68)),
          fPointerColor(Color(240, 242, 245)),
          fLabelColor(Color(180, 185, 192)),
          fTrackWidth(4.5f),
          fDialRadiusRatio(0.70f),
          fBipolar(false),
          fShowValue(false),
          fStartAngle(0.75f * static_cast<float>(M_PI)),  // 135 deg (-135 deg from top)
          fEndAngle(2.25f * static_cast<float>(M_PI)),    // 405 deg (+135 deg from top)
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        KnobEventHandler::setCallback(&fInternalCallback);
        initAccessibility();
    }

    explicit VectorKnob(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          KnobEventHandler(this),
          fTrackColor(Color(42, 44, 48)),
          fActiveColor(Color(52, 152, 219)),
          fDialColor(Color(26, 27, 30)),
          fDialBorderColor(Color(60, 62, 68)),
          fPointerColor(Color(240, 242, 245)),
          fLabelColor(Color(180, 185, 192)),
          fTrackWidth(4.5f),
          fDialRadiusRatio(0.70f),
          fBipolar(false),
          fShowValue(false),
          fStartAngle(0.75f * static_cast<float>(M_PI)),
          fEndAngle(2.25f * static_cast<float>(M_PI)),
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        KnobEventHandler::setCallback(&fInternalCallback);
        initAccessibility();
    }

    explicit VectorKnob(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          KnobEventHandler(this),
          fTrackColor(Color(42, 44, 48)),
          fActiveColor(Color(52, 152, 219)),
          fDialColor(Color(26, 27, 30)),
          fDialBorderColor(Color(60, 62, 68)),
          fPointerColor(Color(240, 242, 245)),
          fLabelColor(Color(180, 185, 192)),
          fTrackWidth(4.5f),
          fDialRadiusRatio(0.70f),
          fBipolar(false),
          fShowValue(false),
          fStartAngle(0.75f * static_cast<float>(M_PI)),
          fEndAngle(2.25f * static_cast<float>(M_PI)),
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        KnobEventHandler::setCallback(&fInternalCallback);
        initAccessibility();
    }

    ~VectorKnob() override {}

    // ----------------------------------------------------------------------------------------------------------------
    // Styling & Appearance

    void setTrackColor(const Color& color) noexcept
    {
        if (fTrackColor == color) return;
        fTrackColor = color;
        repaint();
    }

    void setActiveColor(const Color& color) noexcept
    {
        if (fActiveColor == color) return;
        fActiveColor = color;
        repaint();
    }

    void setDialColor(const Color& color) noexcept
    {
        if (fDialColor == color) return;
        fDialColor = color;
        repaint();
    }

    void setDialBorderColor(const Color& color) noexcept
    {
        if (fDialBorderColor == color) return;
        fDialBorderColor = color;
        repaint();
    }

    void setPointerColor(const Color& color) noexcept
    {
        if (fPointerColor == color) return;
        fPointerColor = color;
        repaint();
    }

    void setLabelColor(const Color& color) noexcept
    {
        if (fLabelColor == color) return;
        fLabelColor = color;
        repaint();
    }

    void setTrackWidth(const float width) noexcept
    {
        fTrackWidth = width;
        repaint();
    }

    void setDialRadiusRatio(const float ratio) noexcept
    {
        fDialRadiusRatio = std::max(0.1f, std::min(0.95f, ratio));
        repaint();
    }

    void setBipolar(const bool bipolar) noexcept
    {
        if (fBipolar == bipolar) return;
        fBipolar = bipolar;
        repaint();
    }

    void setLabel(const char* const label) noexcept
    {
        fLabel = label != nullptr ? label : "";
        repaint();
    }

    void setShowValue(const bool show) noexcept
    {
        if (fShowValue == show) return;
        fShowValue = show;
        repaint();
    }

    void setUnit(const char* const unit) noexcept
    {
        fUnit = unit != nullptr ? unit : "";
        repaint();
    }

    void setAngles(const float startRads, const float endRads) noexcept
    {
        fStartAngle = startRads;
        fEndAngle = endRads;
        repaint();
    }

    void setCallback(Callback* const callback) noexcept
    {
        fUserCallback = callback;
    }

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    bool performAccessibleAction(const AccessibleAction action) override
    {
        const float range = getMaximum() - getMinimum();
        const float step = (range > 0.0f) ? (range * 0.05f) : 0.01f;

        if (action == kAccessibleActionIncrement)
            return setValue(getValue() + step, true);

        if (action == kAccessibleActionDecrement)
            return setValue(getValue() - step, true);

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Value override

    bool setValue(const float value, const bool sendCallback = false) noexcept override
    {
        const bool changed = KnobEventHandler::setValue(value, sendCallback);
        if (changed)
        {
            setAccessibleValue(getValue());
            repaint();
        }
        return changed;
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // NanoDisplay

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 1.0f || h <= 1.0f)
            return;

        const bool hasText = fShowValue || fLabel.isNotEmpty();
        const float cx = w * 0.5f;
        const float cy = hasText ? (h * 0.44f) : (h * 0.5f);
        const float radius = (std::min(w, h) * 0.5f) - (fTrackWidth * 1.5f);

        if (radius <= 2.0f)
            return;

        // 1. Background Track Arc
        beginPath();
        arc(cx, cy, radius, fStartAngle, fEndAngle, NanoVG::CW);
        strokeColor(fTrackColor);
        strokeWidth(fTrackWidth);
        lineCap(NanoVG::ROUND);
        stroke();

        // 2. Active Arc
        const float norm = getNormalizedValue();
        const float sweep = fEndAngle - fStartAngle;
        const float currentAngle = fStartAngle + norm * sweep;

        if (fBipolar)
        {
            const float midAngle = fStartAngle + sweep * 0.5f;
            if (std::abs(currentAngle - midAngle) > 0.005f)
            {
                beginPath();
                if (currentAngle >= midAngle)
                    arc(cx, cy, radius, midAngle, currentAngle, NanoVG::CW);
                else
                    arc(cx, cy, radius, currentAngle, midAngle, NanoVG::CW);

                strokeColor(fActiveColor);
                strokeWidth(fTrackWidth);
                lineCap(NanoVG::ROUND);
                stroke();
            }
        }
        else
        {
            if (norm > 0.005f)
            {
                beginPath();
                arc(cx, cy, radius, fStartAngle, currentAngle, NanoVG::CW);
                strokeColor(fActiveColor);
                strokeWidth(fTrackWidth);
                lineCap(NanoVG::ROUND);
                stroke();
            }
        }

        // 3. Dial Cap Body
        const float dialRadius = radius * fDialRadiusRatio;
        beginPath();
        circle(cx, cy, dialRadius);
        fillColor(fDialColor);
        fill();

        beginPath();
        circle(cx, cy, dialRadius);
        strokeColor(fDialBorderColor);
        strokeWidth(1.2f);
        stroke();

        // 4. Pointer Indicator
        const float sinA = std::sin(currentAngle);
        const float cosA = std::cos(currentAngle);
        const float ptrInner = dialRadius * 0.35f;
        const float ptrOuter = dialRadius * 0.82f;

        beginPath();
        moveTo(cx + cosA * ptrInner, cy + sinA * ptrInner);
        lineTo(cx + cosA * ptrOuter, cy + sinA * ptrOuter);
        strokeColor(fPointerColor);
        strokeWidth(2.5f);
        lineCap(NanoVG::ROUND);
        stroke();

        // 5. Optional Label / Value Text
        if (hasText)
        {
            fontSize(11.0f);
            textAlign(NanoVG::Align(NanoVG::ALIGN_CENTER | NanoVG::ALIGN_TOP));
            fillColor(fLabelColor);

            char textBuf[64];
            if (fShowValue)
            {
                if (isInteger())
                    std::snprintf(textBuf, sizeof(textBuf), "%d%s", static_cast<int>(getValue()), fUnit.buffer());
                else
                    std::snprintf(textBuf, sizeof(textBuf), "%.2f%s", getValue(), fUnit.buffer());
            }
            else
            {
                std::snprintf(textBuf, sizeof(textBuf), "%s", fLabel.buffer());
            }

            text(cx, cy + radius + 4.0f, textBuf, nullptr);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Events

public:
    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button == kMouseButtonRight && ev.press && contains(ev.pos))
        {
            if (fUserCallback != nullptr)
                fUserCallback->vectorKnobContextMenu(this, ev.pos.getX(), ev.pos.getY());
            if (fContextMenuCb != nullptr)
                fContextMenuCb(this, ev.pos.getX(), ev.pos.getY(), fContextMenuUserData);
            return true;
        }

        const bool handled = KnobEventHandler::mouseEvent(ev);
        if (handled)
            repaint();
        return handled;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        const bool handled = KnobEventHandler::motionEvent(ev);
        if (handled)
            repaint();
        return handled;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const bool handled = KnobEventHandler::scrollEvent(ev);
        if (handled)
            repaint();
        return handled;
    }

private:
    Color fTrackColor;
    Color fActiveColor;
    Color fDialColor;
    Color fDialBorderColor;
    Color fPointerColor;
    Color fLabelColor;
    float fTrackWidth;
    float fDialRadiusRatio;
    bool fBipolar;
    bool fShowValue;
    float fStartAngle;
    float fEndAngle;
    String fLabel;
    String fUnit;
    Callback* fUserCallback;
    ContextMenuCallback fContextMenuCb;
    void* fContextMenuUserData;

    struct InternalCallback : KnobEventHandler::Callback {
        VectorKnob* self;
        InternalCallback(VectorKnob* const s) : self(s) {}

        void knobDragStarted(SubWidget*) override
        {
            if (self->fUserCallback != nullptr)
                self->fUserCallback->vectorKnobDragStarted(self);
        }

        void knobDragFinished(SubWidget*) override
        {
            if (self->fUserCallback != nullptr)
                self->fUserCallback->vectorKnobDragFinished(self);
        }

        void knobValueChanged(SubWidget*, const float value) override
        {
            self->setAccessibleValue(value);
            self->repaint();
            if (self->fUserCallback != nullptr)
                self->fUserCallback->vectorKnobValueChanged(self, value);
        }

        void knobDoubleClicked(SubWidget*) override
        {
            if (self->fUserCallback != nullptr)
                self->fUserCallback->vectorKnobDoubleClicked(self);
        }
    } fInternalCallback{this};

    void initAccessibility()
    {
        setAccessibleRole(kAccessibleRoleDial);
        setAccessibleRange(getMinimum(), getMaximum());
        setAccessibleValue(getValue());
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorKnob)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_KNOB_HPP_INCLUDED
