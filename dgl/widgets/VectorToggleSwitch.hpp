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

#ifndef DGL_VECTOR_TOGGLE_SWITCH_HPP_INCLUDED
#define DGL_VECTOR_TOGGLE_SWITCH_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <algorithm>
#include <cmath>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector toggle / slide switch.
   Supports horizontal and vertical orientations, sliding thumb, active status LED glow,
   and semantic accessibility (A11y).
 */
class VectorToggleSwitch : public NanoSubWidget,
                           public AccessibleElement
{
public:
    enum Orientation {
        Horizontal,
        Vertical
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorToggleSwitchChanged(VectorToggleSwitch* sw, bool checked) { (void)sw; (void)checked; }
        virtual void vectorToggleSwitchContextMenu(VectorToggleSwitch* sw, int32_t x, int32_t y) { (void)sw; (void)x; (void)y; }
    };

    typedef void (*ContextMenuCallback)(Widget* widget, int32_t x, int32_t y, void* userData);

    void setContextMenuCallback(ContextMenuCallback cb, void* userData = nullptr) noexcept
    {
        fContextMenuCb = cb;
        fContextMenuUserData = userData;
    }

    explicit VectorToggleSwitch(Widget* const parentWidget, const Orientation orientation = Horizontal)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fChecked(false),
          fTrackColorOff(Color(32, 34, 38)),
          fTrackColorOn(Color(40, 110, 190)),
          fThumbColor(Color(240, 242, 246)),
          fThumbBorderColor(Color(65, 68, 75)),
          fBorderColor(Color(55, 58, 66)),
          fGlowColor(Color(52, 152, 219, 80)),
          fPadding(3.0f),
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    explicit VectorToggleSwitch(NanoSubWidget* const parentWidget, const Orientation orientation = Horizontal)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fChecked(false),
          fTrackColorOff(Color(32, 34, 38)),
          fTrackColorOn(Color(40, 110, 190)),
          fThumbColor(Color(240, 242, 246)),
          fThumbBorderColor(Color(65, 68, 75)),
          fBorderColor(Color(55, 58, 66)),
          fGlowColor(Color(52, 152, 219, 80)),
          fPadding(3.0f),
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    explicit VectorToggleSwitch(NanoTopLevelWidget* const parentWidget, const Orientation orientation = Horizontal)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fChecked(false),
          fTrackColorOff(Color(32, 34, 38)),
          fTrackColorOn(Color(40, 110, 190)),
          fThumbColor(Color(240, 242, 246)),
          fThumbBorderColor(Color(65, 68, 75)),
          fBorderColor(Color(55, 58, 66)),
          fGlowColor(Color(52, 152, 219, 80)),
          fPadding(3.0f),
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    ~VectorToggleSwitch() override {}

    // ----------------------------------------------------------------------------------------------------------------
    // State API

    bool isChecked() const noexcept
    {
        return fChecked;
    }

    void setChecked(const bool checked, const bool sendCallback = false)
    {
        if (fChecked == checked)
            return;

        fChecked = checked;
        updateAccessibility();
        repaint();

        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorToggleSwitchChanged(this, fChecked);
    }

    void toggle(const bool sendCallback = false)
    {
        setChecked(!fChecked, sendCallback);
    }

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    bool performAccessibleAction(const AccessibleAction action) override
    {
        if (action == kAccessibleActionPress || action == kAccessibleActionIncrement || action == kAccessibleActionDecrement)
        {
            toggle(true);
            return true;
        }
        return false;
    }

    Orientation getOrientation() const noexcept
    {
        return fOrientation;
    }

    void setOrientation(const Orientation orientation) noexcept
    {
        if (fOrientation == orientation)
            return;

        fOrientation = orientation;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Styling

    void setTrackColors(const Color& offColor, const Color& onColor) noexcept
    {
        fTrackColorOff = offColor;
        fTrackColorOn = onColor;
        repaint();
    }

    void setThumbColor(const Color& color) noexcept
    {
        fThumbColor = color;
        repaint();
    }

    void setBorderColor(const Color& color) noexcept
    {
        fBorderColor = color;
        repaint();
    }

    void setCallback(Callback* const cb) noexcept
    {
        fUserCallback = cb;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Event Handling

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button == kMouseButtonRight && ev.press && contains(ev.pos))
        {
            if (fUserCallback != nullptr)
                fUserCallback->vectorToggleSwitchContextMenu(this, ev.pos.getX(), ev.pos.getY());
            if (fContextMenuCb != nullptr)
                fContextMenuCb(this, ev.pos.getX(), ev.pos.getY(), fContextMenuUserData);
            return true;
        }

        if (ev.button != kMouseButtonLeft || !ev.press)
            return false;

        toggle(true);
        return true;
    }

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (!ev.press)
            return false;

        if (ev.key == kKeySpace || ev.key == kKeyEnter || ev.key == kKeyPadEnter)
        {
            toggle(true);
            return true;
        }

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // NanoVG Rendering

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 4.0f || h <= 4.0f)
            return;

        const float trackRadius = (fOrientation == Horizontal) ? (h * 0.5f) : (w * 0.5f);

        // 1. Track Background Pill
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, trackRadius);
        fillColor(fChecked ? fTrackColorOn : fTrackColorOff);
        fill();

        // 2. Track Border
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, trackRadius);
        strokeColor(fBorderColor);
        strokeWidth(1.2f);
        stroke();

        // 3. Sliding Thumb Puck
        float thumbX = 0.0f;
        float thumbY = 0.0f;
        float thumbR = 0.0f;

        if (fOrientation == Horizontal)
        {
            thumbR = (h - 2.0f * fPadding) * 0.5f;
            thumbY = h * 0.5f;
            const float minX = fPadding + thumbR;
            const float maxX = w - fPadding - thumbR;
            thumbX = fChecked ? maxX : minX;
        }
        else
        {
            thumbR = (w - 2.0f * fPadding) * 0.5f;
            thumbX = w * 0.5f;
            const float minY = fPadding + thumbR;
            const float maxY = h - fPadding - thumbR;
            // For vertical switch: top is On, bottom is Off
            thumbY = fChecked ? minY : maxY;
        }

        if (thumbR <= 1.0f)
            return;

        // Glow if checked
        if (fChecked)
        {
            beginPath();
            circle(thumbX, thumbY, thumbR * 1.5f);
            fillColor(fGlowColor);
            fill();
        }

        // Thumb Body
        beginPath();
        circle(thumbX, thumbY, thumbR);
        fillColor(fThumbColor);
        fill();

        // Thumb Border
        beginPath();
        circle(thumbX, thumbY, thumbR);
        strokeColor(fThumbBorderColor);
        strokeWidth(1.2f);
        stroke();
    }

private:
    Orientation fOrientation;
    bool  fChecked;

    Color fTrackColorOff;
    Color fTrackColorOn;
    Color fThumbColor;
    Color fThumbBorderColor;
    Color fBorderColor;
    Color fGlowColor;
    float fPadding;

    Callback* fUserCallback;
    ContextMenuCallback fContextMenuCb;
    void* fContextMenuUserData;

    void initAccessibility()
    {
        setAccessibleRole(kAccessibleRoleCheckBox);
        setAccessibleLabel("Toggle Switch");
        setAccessibleRange(0.0f, 1.0f, 1.0f);
        updateAccessibility();
    }

    void updateAccessibility()
    {
        setAccessibleValue(fChecked ? 1.0f : 0.0f, fChecked ? "On" : "Off");
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorToggleSwitch)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_TOGGLE_SWITCH_HPP_INCLUDED
