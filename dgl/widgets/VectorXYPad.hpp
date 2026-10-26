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

#ifndef DGL_VECTOR_XY_PAD_HPP_INCLUDED
#define DGL_VECTOR_XY_PAD_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector 2D XY modulation pad.
   Controls two continuous parameters simultaneously (X and Y axes), with crosshair reticles,
   spring-return ballistics, motion trails, and semantic accessibility (A11y).
 */
class VectorXYPad : public NanoSubWidget,
                    public AccessibleElement
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorXYPadDragStarted(VectorXYPad* pad) { (void)pad; }
        virtual void vectorXYPadDragFinished(VectorXYPad* pad) { (void)pad; }
        virtual void vectorXYPadValueChanged(VectorXYPad* pad, float x, float y) { (void)pad; (void)x; (void)y; }
    };

    explicit VectorXYPad(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fX(0.5f),
          fY(0.5f),
          fMinX(0.0f),
          fMaxX(1.0f),
          fMinY(0.0f),
          fMaxY(1.0f),
          fDefaultX(0.5f),
          fDefaultY(0.5f),
          fBipolarX(false),
          fBipolarY(false),
          fSpringReturn(false),
          fDragging(false),
          fShowCrosshairs(true),
          fShowGrid(true),
          fShowTrails(true),
          fHandleRadius(9.0f),
          fCornerRadius(4.0f),
          fBgColor(Color(22, 24, 28)),
          fGridColor(Color(40, 43, 50)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(65, 68, 75)),
          fHandleColor(Color(52, 152, 219)),
          fHandleBorderColor(Color(230, 240, 255)),
          fHandleGlowColor(Color(52, 152, 219, 80)),
          fCrosshairColor(Color(52, 152, 219, 100)),
          fTrailColor(Color(52, 152, 219, 40)),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    explicit VectorXYPad(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fX(0.5f),
          fY(0.5f),
          fMinX(0.0f),
          fMaxX(1.0f),
          fMinY(0.0f),
          fMaxY(1.0f),
          fDefaultX(0.5f),
          fDefaultY(0.5f),
          fBipolarX(false),
          fBipolarY(false),
          fSpringReturn(false),
          fDragging(false),
          fShowCrosshairs(true),
          fShowGrid(true),
          fShowTrails(true),
          fHandleRadius(9.0f),
          fCornerRadius(4.0f),
          fBgColor(Color(22, 24, 28)),
          fGridColor(Color(40, 43, 50)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(65, 68, 75)),
          fHandleColor(Color(52, 152, 219)),
          fHandleBorderColor(Color(230, 240, 255)),
          fHandleGlowColor(Color(52, 152, 219, 80)),
          fCrosshairColor(Color(52, 152, 219, 100)),
          fTrailColor(Color(52, 152, 219, 40)),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    explicit VectorXYPad(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fX(0.5f),
          fY(0.5f),
          fMinX(0.0f),
          fMaxX(1.0f),
          fMinY(0.0f),
          fMaxY(1.0f),
          fDefaultX(0.5f),
          fDefaultY(0.5f),
          fBipolarX(false),
          fBipolarY(false),
          fSpringReturn(false),
          fDragging(false),
          fShowCrosshairs(true),
          fShowGrid(true),
          fShowTrails(true),
          fHandleRadius(9.0f),
          fCornerRadius(4.0f),
          fBgColor(Color(22, 24, 28)),
          fGridColor(Color(40, 43, 50)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(65, 68, 75)),
          fHandleColor(Color(52, 152, 219)),
          fHandleBorderColor(Color(230, 240, 255)),
          fHandleGlowColor(Color(52, 152, 219, 80)),
          fCrosshairColor(Color(52, 152, 219, 100)),
          fTrailColor(Color(52, 152, 219, 40)),
          fUserCallback(nullptr)
    {
        initAccessibility();
    }

    ~VectorXYPad() override {}

    // ----------------------------------------------------------------------------------------------------------------
    // Values and Ranges

    float getX() const noexcept { return fX; }
    float getY() const noexcept { return fY; }

    float getNormalizedX() const noexcept
    {
        if (d_isEqual(fMaxX, fMinX)) return 0.0f;
        return std::max(0.0f, std::min(1.0f, (fX - fMinX) / (fMaxX - fMinX)));
    }

    float getNormalizedY() const noexcept
    {
        if (d_isEqual(fMaxY, fMinY)) return 0.0f;
        return std::max(0.0f, std::min(1.0f, (fY - fMinY) / (fMaxY - fMinY)));
    }

    void setValues(const float x, const float y, const bool sendCallback = false)
    {
        const float clampedX = std::max(fMinX, std::min(fMaxX, x));
        const float clampedY = std::max(fMinY, std::min(fMaxY, y));

        if (d_isEqual(fX, clampedX) && d_isEqual(fY, clampedY))
            return;

        recordTrailPoint(getNormalizedX(), getNormalizedY());

        fX = clampedX;
        fY = clampedY;

        updateAccessibility();
        repaint();

        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorXYPadValueChanged(this, fX, fY);
    }

    void setNormalizedValues(const float normX, const float normY, const bool sendCallback = false)
    {
        const float x = fMinX + std::max(0.0f, std::min(1.0f, normX)) * (fMaxX - fMinX);
        const float y = fMinY + std::max(0.0f, std::min(1.0f, normY)) * (fMaxY - fMinY);
        setValues(x, y, sendCallback);
    }

    void setRangeX(const float minX, const float maxX)
    {
        if (minX < maxX)
        {
            fMinX = minX;
            fMaxX = maxX;
            fX = std::max(fMinX, std::min(fMaxX, fX));
            updateAccessibility();
            repaint();
        }
    }

    void setRangeY(const float minY, const float maxY)
    {
        if (minY < maxY)
        {
            fMinY = minY;
            fMaxY = maxY;
            fY = std::max(fMinY, std::min(fMaxY, fY));
            updateAccessibility();
            repaint();
        }
    }

    void setDefaultValues(const float defX, const float defY) noexcept
    {
        fDefaultX = std::max(fMinX, std::min(fMaxX, defX));
        fDefaultY = std::max(fMinY, std::min(fMaxY, defY));
    }

    void setBipolar(const bool bipolarX, const bool bipolarY) noexcept
    {
        fBipolarX = bipolarX;
        fBipolarY = bipolarY;
        repaint();
    }

    void setSpringReturn(const bool spring) noexcept
    {
        fSpringReturn = spring;
    }

    bool isSpringReturn() const noexcept
    {
        return fSpringReturn;
    }

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    bool performAccessibleAction(const AccessibleAction action) override
    {
        const float stepX = (fMaxX - fMinX) * 0.05f;
        const float stepY = (fMaxY - fMinY) * 0.05f;
        if (action == kAccessibleActionIncrement)
        {
            setValues(fX + stepX, fY + stepY, true);
            return true;
        }
        if (action == kAccessibleActionDecrement)
        {
            setValues(fX - stepX, fY - stepY, true);
            return true;
        }
        return false;
    }

    void setShowCrosshairs(const bool show) noexcept
    {
        fShowCrosshairs = show;
        repaint();
    }

    void setShowGrid(const bool show) noexcept
    {
        fShowGrid = show;
        repaint();
    }

    void setShowTrails(const bool show) noexcept
    {
        fShowTrails = show;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Styling & Appearance

    void setHandleColor(const Color& color) noexcept
    {
        fHandleColor = color;
        fHandleGlowColor = Color(color.red, color.green, color.blue, 80);
        fCrosshairColor = Color(color.red, color.green, color.blue, 100);
        fTrailColor = Color(color.red, color.green, color.blue, 40);
        repaint();
    }

    void setBackgroundColor(const Color& color) noexcept { fBgColor = color; repaint(); }
    void setBorderColor(const Color& color) noexcept { fBorderColor = color; repaint(); }
    void setHandleRadius(const float radius) noexcept { fHandleRadius = radius; repaint(); }
    void setCornerRadius(const float radius) noexcept { fCornerRadius = radius; repaint(); }
    void setCallback(Callback* const cb) noexcept { fUserCallback = cb; }

    // ----------------------------------------------------------------------------------------------------------------
    // Event Handling

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != kMouseButtonLeft)
            return false;

        if (ev.press)
        {
            fDragging = true;
            updateValuesFromMouse(ev.pos.getX(), ev.pos.getY());

            if (fUserCallback != nullptr)
                fUserCallback->vectorXYPadDragStarted(this);

            return true;
        }

        if (fDragging)
        {
            fDragging = false;

            if (fSpringReturn)
                setValues(fDefaultX, fDefaultY, true);

            if (fUserCallback != nullptr)
                fUserCallback->vectorXYPadDragFinished(this);

            return true;
        }

        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (!fDragging)
            return false;

        updateValuesFromMouse(ev.pos.getX(), ev.pos.getY());
        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // NanoVG Rendering

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 4.0f || h <= 4.0f)
            return;

        // 1. Background
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        fillColor(fBgColor);
        fill();

        // 2. Grid Lines (4x4 divisions)
        if (fShowGrid)
        {
            strokeColor(fGridColor);
            strokeWidth(0.8f);

            for (int i = 1; i < 4; ++i)
            {
                const float gx = (w - 2.0f) * (static_cast<float>(i) * 0.25f) + 1.0f;
                beginPath();
                moveTo(gx, 1.0f);
                lineTo(gx, h - 1.0f);
                stroke();

                const float gy = (h - 2.0f) * (static_cast<float>(i) * 0.25f) + 1.0f;
                beginPath();
                moveTo(1.0f, gy);
                lineTo(w - 1.0f, gy);
                stroke();
            }
        }

        // 3. Center Bipolar Axes
        if (fBipolarX || fBipolarY)
        {
            strokeColor(fAxisColor);
            strokeWidth(1.0f);

            if (fBipolarX)
            {
                const float midX = w * 0.5f;
                beginPath();
                moveTo(midX, 1.0f);
                lineTo(midX, h - 1.0f);
                stroke();
            }

            if (fBipolarY)
            {
                const float midY = h * 0.5f;
                beginPath();
                moveTo(1.0f, midY);
                lineTo(w - 1.0f, midY);
                stroke();
            }
        }

        const float pad = fHandleRadius + 2.0f;
        const float usableW = w - 2.0f * pad;
        const float usableH = h - 2.0f * pad;

        const float normX = getNormalizedX();
        // Invert Y so up is 1.0 and down is 0.0
        const float normY = 1.0f - getNormalizedY();

        const float hx = pad + normX * usableW;
        const float hy = pad + normY * usableH;

        // 4. Motion Trails
        if (fShowTrails && !fTrailPoints.empty())
        {
            for (size_t i = 0; i < fTrailPoints.size(); ++i)
            {
                const float trailNormX = fTrailPoints[i].first;
                const float trailNormY = 1.0f - fTrailPoints[i].second;
                const float tx = pad + trailNormX * usableW;
                const float ty = pad + trailNormY * usableH;

                const float alpha = static_cast<float>(i + 1) / static_cast<float>(fTrailPoints.size() + 1);
                const float trailRadius = fHandleRadius * (0.3f + 0.5f * alpha);

                beginPath();
                circle(tx, ty, trailRadius);
                fillColor(Color(fTrailColor.red, fTrailColor.green, fTrailColor.blue, static_cast<uint8_t>(fTrailColor.alpha * alpha)));
                fill();
            }
        }

        // 5. Crosshair Lines
        if (fShowCrosshairs)
        {
            strokeColor(fCrosshairColor);
            strokeWidth(1.0f);

            // Horizontal crosshair
            beginPath();
            moveTo(1.0f, hy);
            lineTo(w - 1.0f, hy);
            stroke();

            // Vertical crosshair
            beginPath();
            moveTo(hx, 1.0f);
            lineTo(hx, h - 1.0f);
            stroke();
        }

        // 6. Handle Puck Glow & Body
        beginPath();
        circle(hx, hy, fHandleRadius * 1.6f);
        fillColor(fHandleGlowColor);
        fill();

        beginPath();
        circle(hx, hy, fHandleRadius);
        fillColor(fHandleColor);
        fill();

        beginPath();
        circle(hx, hy, fHandleRadius);
        strokeColor(fHandleBorderColor);
        strokeWidth(1.8f);
        stroke();

        // Center Pip
        beginPath();
        circle(hx, hy, 2.0f);
        fillColor(Color(255, 255, 255));
        fill();

        // 7. Border Frame
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        strokeColor(fBorderColor);
        strokeWidth(1.2f);
        stroke();
    }

private:
    float fX;
    float fY;
    float fMinX;
    float fMaxX;
    float fMinY;
    float fMaxY;
    float fDefaultX;
    float fDefaultY;
    bool  fBipolarX;
    bool  fBipolarY;
    bool  fSpringReturn;
    bool  fDragging;

    bool  fShowCrosshairs;
    bool  fShowGrid;
    bool  fShowTrails;

    float fHandleRadius;
    float fCornerRadius;

    Color fBgColor;
    Color fGridColor;
    Color fAxisColor;
    Color fBorderColor;
    Color fHandleColor;
    Color fHandleBorderColor;
    Color fHandleGlowColor;
    Color fCrosshairColor;
    Color fTrailColor;

    Callback* fUserCallback;

    std::vector<std::pair<float, float> > fTrailPoints;

    void recordTrailPoint(const float nx, const float ny)
    {
        if (!fShowTrails)
            return;

        if (fTrailPoints.size() >= 6)
            fTrailPoints.erase(fTrailPoints.begin());

        fTrailPoints.push_back(std::make_pair(nx, ny));
    }

    void updateValuesFromMouse(const double mx, const double my)
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        const float pad = fHandleRadius + 2.0f;
        const float usableW = w - 2.0f * pad;
        const float usableH = h - 2.0f * pad;

        if (usableW <= 1.0f || usableH <= 1.0f)
            return;

        const float normX = std::max(0.0f, std::min(1.0f, (static_cast<float>(mx) - pad) / usableW));
        // Up is 1.0, Down is 0.0
        const float normY = 1.0f - std::max(0.0f, std::min(1.0f, (static_cast<float>(my) - pad) / usableH));

        setNormalizedValues(normX, normY, true);
    }

    void initAccessibility()
    {
        setAccessibleRole(kAccessibleRoleIndicator);
        setAccessibleLabel("XY Modulation Pad");
        setAccessibleRange(fMinX, fMaxX);
        updateAccessibility();
    }

    void updateAccessibility()
    {
        char valBuf[64];
        std::snprintf(valBuf, sizeof(valBuf), "X: %.2f, Y: %.2f", static_cast<double>(fX), static_cast<double>(fY));
        setAccessibleValue(fX, valBuf);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorXYPad)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_XY_PAD_HPP_INCLUDED
