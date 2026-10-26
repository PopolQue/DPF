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

#ifndef DGL_VECTOR_SLIDER_HPP_INCLUDED
#define DGL_VECTOR_SLIDER_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <cmath>
#include <algorithm>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector linear slider.
 */
class VectorSlider : public NanoSubWidget,
                     public AccessibleElement
{
public:
    enum Orientation {
        Vertical,
        Horizontal
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorSliderDragStarted(VectorSlider* slider) {}
        virtual void vectorSliderDragFinished(VectorSlider* slider) {}
        virtual void vectorSliderValueChanged(VectorSlider* slider, float value) {}
        virtual void vectorSliderContextMenu(VectorSlider* slider, int32_t x, int32_t y) { (void)slider; (void)x; (void)y; }
    };

    typedef void (*ContextMenuCallback)(Widget* widget, int32_t x, int32_t y, void* userData);

    void setContextMenuCallback(ContextMenuCallback cb, void* userData = nullptr) noexcept
    {
        fContextMenuCb = cb;
        fContextMenuUserData = userData;
    }

    explicit VectorSlider(Widget* const parentWidget, const Orientation orientation = Vertical)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fValue(0.0f),
          fMin(0.0f),
          fMax(1.0f),
          fDefault(0.0f),
          fStep(0.0f),
          fBipolar(false),
          fDragging(false),
          fTrackColor(Color(36, 38, 42)),
          fActiveColor(Color(52, 152, 219)),
          fThumbColor(Color(230, 233, 238)),
          fThumbBorderColor(Color(50, 52, 58)),
          fTrackThickness(6.0f),
          fThumbWidth(18.0f),
          fThumbHeight(12.0f),
          fCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    explicit VectorSlider(NanoSubWidget* const parentWidget, const Orientation orientation = Vertical)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fValue(0.0f),
          fMin(0.0f),
          fMax(1.0f),
          fDefault(0.0f),
          fStep(0.0f),
          fBipolar(false),
          fDragging(false),
          fTrackColor(Color(36, 38, 42)),
          fActiveColor(Color(52, 152, 219)),
          fThumbColor(Color(230, 233, 238)),
          fThumbBorderColor(Color(50, 52, 58)),
          fTrackThickness(6.0f),
          fThumbWidth(18.0f),
          fThumbHeight(12.0f),
          fCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    explicit VectorSlider(NanoTopLevelWidget* const parentWidget, const Orientation orientation = Vertical)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fValue(0.0f),
          fMin(0.0f),
          fMax(1.0f),
          fDefault(0.0f),
          fStep(0.0f),
          fBipolar(false),
          fDragging(false),
          fTrackColor(Color(36, 38, 42)),
          fActiveColor(Color(52, 152, 219)),
          fThumbColor(Color(230, 233, 238)),
          fThumbBorderColor(Color(50, 52, 58)),
          fTrackThickness(6.0f),
          fThumbWidth(18.0f),
          fThumbHeight(12.0f),
          fCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    ~VectorSlider() override {}

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration

    Orientation getOrientation() const noexcept
    {
        return fOrientation;
    }

    void setOrientation(const Orientation orientation) noexcept
    {
        if (fOrientation == orientation) return;
        fOrientation = orientation;
        repaint();
    }

    float getValue() const noexcept
    {
        return fValue;
    }

    float getNormalizedValue() const noexcept
    {
        if (d_isEqual(fMax, fMin))
            return 0.0f;
        return std::max(0.0f, std::min(1.0f, (fValue - fMin) / (fMax - fMin)));
    }

    bool setValue(float val, const bool sendCallback = false) noexcept
    {
        if (fStep > 0.0f)
            val = fMin + std::round((val - fMin) / fStep) * fStep;

        val = std::max(fMin, std::min(fMax, val));

        if (d_isEqual(fValue, val))
            return false;

        fValue = val;
        setAccessibleValue(fValue);
        repaint();

        if (sendCallback && fCallback != nullptr)
            fCallback->vectorSliderValueChanged(this, fValue);

        return true;
    }

    void setNormalizedValue(const float norm, const bool sendCallback = false) noexcept
    {
        const float clamped = std::max(0.0f, std::min(1.0f, norm));
        setValue(fMin + clamped * (fMax - fMin), sendCallback);
    }

    void setRange(const float min, const float max) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(max > min,);
        fMin = min;
        fMax = max;
        fValue = std::max(fMin, std::min(fMax, fValue));
        repaint();
    }

    void setDefault(const float def) noexcept
    {
        fDefault = std::max(fMin, std::min(fMax, def));
    }

    void setStep(const float step) noexcept
    {
        fStep = std::max(0.0f, step);
    }

    void setBipolar(const bool bipolar) noexcept
    {
        if (fBipolar == bipolar) return;
        fBipolar = bipolar;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Styling

    void setTrackColor(const Color& color) noexcept
    {
        fTrackColor = color;
        repaint();
    }

    void setActiveColor(const Color& color) noexcept
    {
        fActiveColor = color;
        repaint();
    }

    void setThumbColor(const Color& color) noexcept
    {
        fThumbColor = color;
        repaint();
    }

    void setThumbBorderColor(const Color& color) noexcept
    {
        fThumbBorderColor = color;
        repaint();
    }

    void setTrackThickness(const float thickness) noexcept
    {
        fTrackThickness = thickness;
        repaint();
    }

    void setThumbSize(const float width, const float height) noexcept
    {
        fThumbWidth = width;
        fThumbHeight = height;
        repaint();
    }

    void setCallback(Callback* const callback) noexcept
    {
        fCallback = callback;
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

        const float norm = getNormalizedValue();

        if (fOrientation == Vertical)
        {
            const float trackX = (w - fTrackThickness) * 0.5f;
            const float halfThumb = fThumbHeight * 0.5f;
            const float trackTop = halfThumb;
            const float trackBottom = h - halfThumb;
            const float trackHeight = trackBottom - trackTop;

            if (trackHeight <= 1.0f)
                return;

            // 1. Background Track
            beginPath();
            roundedRect(trackX, trackTop, fTrackThickness, trackHeight, fTrackThickness * 0.5f);
            fillColor(fTrackColor);
            fill();

            // Thumb center Y (inverted in vertical: bottom is 0, top is 1)
            const float thumbY = trackBottom - (norm * trackHeight);

            // 2. Active Gauge Bar
            if (fBipolar)
            {
                const float centerY = trackTop + trackHeight * 0.5f;
                if (std::abs(thumbY - centerY) > 1.0f)
                {
                    const float top = std::min(thumbY, centerY);
                    const float barH = std::abs(thumbY - centerY);
                    beginPath();
                    roundedRect(trackX, top, fTrackThickness, barH, fTrackThickness * 0.5f);
                    fillColor(fActiveColor);
                    fill();
                }
            }
            else
            {
                const float barH = trackBottom - thumbY;
                if (barH > 1.0f)
                {
                    beginPath();
                    roundedRect(trackX, thumbY, fTrackThickness, barH, fTrackThickness * 0.5f);
                    fillColor(fActiveColor);
                    fill();
                }
            }

            // 3. Thumb Handle
            const float thumbX = (w - fThumbWidth) * 0.5f;
            const float thumbTop = thumbY - halfThumb;
            beginPath();
            roundedRect(thumbX, thumbTop, fThumbWidth, fThumbHeight, 3.0f);
            fillColor(fThumbColor);
            fill();

            beginPath();
            roundedRect(thumbX, thumbTop, fThumbWidth, fThumbHeight, 3.0f);
            strokeColor(fThumbBorderColor);
            strokeWidth(1.2f);
            stroke();

            // Inner ridge
            beginPath();
            moveTo(thumbX + 4.0f, thumbY);
            lineTo(thumbX + fThumbWidth - 4.0f, thumbY);
            strokeColor(fThumbBorderColor);
            strokeWidth(1.0f);
            stroke();
        }
        else // Horizontal
        {
            const float trackY = (h - fTrackThickness) * 0.5f;
            const float halfThumb = fThumbWidth * 0.5f;
            const float trackLeft = halfThumb;
            const float trackRight = w - halfThumb;
            const float trackWidth = trackRight - trackLeft;

            if (trackWidth <= 1.0f)
                return;

            // 1. Background Track
            beginPath();
            roundedRect(trackLeft, trackY, trackWidth, fTrackThickness, fTrackThickness * 0.5f);
            fillColor(fTrackColor);
            fill();

            // Thumb center X
            const float thumbX = trackLeft + (norm * trackWidth);

            // 2. Active Gauge Bar
            if (fBipolar)
            {
                const float centerX = trackLeft + trackWidth * 0.5f;
                if (std::abs(thumbX - centerX) > 1.0f)
                {
                    const float left = std::min(thumbX, centerX);
                    const float barW = std::abs(thumbX - centerX);
                    beginPath();
                    roundedRect(left, trackY, barW, fTrackThickness, fTrackThickness * 0.5f);
                    fillColor(fActiveColor);
                    fill();
                }
            }
            else
            {
                const float barW = thumbX - trackLeft;
                if (barW > 1.0f)
                {
                    beginPath();
                    roundedRect(trackLeft, trackY, barW, fTrackThickness, fTrackThickness * 0.5f);
                    fillColor(fActiveColor);
                    fill();
                }
            }

            // 3. Thumb Handle
            const float thumbLeft = thumbX - halfThumb;
            const float thumbTop = (h - fThumbHeight) * 0.5f;
            beginPath();
            roundedRect(thumbLeft, thumbTop, fThumbWidth, fThumbHeight, 3.0f);
            fillColor(fThumbColor);
            fill();

            beginPath();
            roundedRect(thumbLeft, thumbTop, fThumbWidth, fThumbHeight, 3.0f);
            strokeColor(fThumbBorderColor);
            strokeWidth(1.2f);
            stroke();

            // Inner ridge
            beginPath();
            moveTo(thumbX, thumbTop + 4.0f);
            lineTo(thumbX, thumbTop + fThumbHeight - 4.0f);
            strokeColor(fThumbBorderColor);
            strokeWidth(1.0f);
            stroke();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Events

public:
    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button == kMouseButtonRight && ev.press && contains(ev.pos))
        {
            if (fCallback != nullptr)
                fCallback->vectorSliderContextMenu(this, ev.pos.getX(), ev.pos.getY());
            if (fContextMenuCb != nullptr)
                fContextMenuCb(this, ev.pos.getX(), ev.pos.getY(), fContextMenuUserData);
            return true;
        }

        if (ev.button != 1)
            return false;

        if (ev.press)
        {
            fDragging = true;
            updateValueFromPosition(ev.pos.getX(), ev.pos.getY());

            if (fCallback != nullptr)
                fCallback->vectorSliderDragStarted(this);

            return true;
        }

        if (fDragging)
        {
            fDragging = false;
            if (fCallback != nullptr)
                fCallback->vectorSliderDragFinished(this);
            return true;
        }

        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (!fDragging)
            return false;

        updateValueFromPosition(ev.pos.getX(), ev.pos.getY());
        return true;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const float delta = (fOrientation == Vertical) ? -ev.delta.getY() : ev.delta.getX();
        const float stepAmount = (fStep > 0.0f) ? fStep : ((fMax - fMin) * 0.02f);
        setValue(fValue + delta * stepAmount, true);
        return true;
    }

private:
    void updateValueFromPosition(const int x, const int y)
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        float norm = 0.0f;

        if (fOrientation == Vertical)
        {
            const float halfThumb = fThumbHeight * 0.5f;
            const float trackTop = halfThumb;
            const float trackBottom = h - halfThumb;
            const float trackHeight = trackBottom - trackTop;

            if (trackHeight > 0.0f)
                norm = 1.0f - ((static_cast<float>(y) - trackTop) / trackHeight);
        }
        else
        {
            const float halfThumb = fThumbWidth * 0.5f;
            const float trackLeft = halfThumb;
            const float trackRight = w - halfThumb;
            const float trackWidth = trackRight - trackLeft;

            if (trackWidth > 0.0f)
                norm = (static_cast<float>(x) - trackLeft) / trackWidth;
        }

        setNormalizedValue(norm, true);
    }

    Orientation fOrientation;
    float fValue;
    float fMin;
    float fMax;
    float fDefault;
    float fStep;
    bool fBipolar;
    bool fDragging;
    Color fTrackColor;
    Color fActiveColor;
    Color fThumbColor;
    Color fThumbBorderColor;
    float fTrackThickness;
    float fThumbWidth;
    float fThumbHeight;
    Callback* fCallback;
    ContextMenuCallback fContextMenuCb;
    void* fContextMenuUserData;

    void initAccessibility()
    {
        setAccessibleRole(kAccessibleRoleSlider);
        setAccessibleRange(fMin, fMax, fStep);
        setAccessibleValue(fValue);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorSlider)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_SLIDER_HPP_INCLUDED
