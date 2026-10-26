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

#ifndef DGL_VECTOR_ENVELOPE_HPP_INCLUDED
#define DGL_VECTOR_ENVELOPE_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector ADSR and multi-stage envelope visualizer/editor.
   Features interactive draggable control points, customizable tension/curvature slopes, pre-shaded
   gradient fills, real-time lock-free playhead telemetry tracking, and semantic accessibility (A11y).
 */
class VectorEnvelope : public NanoSubWidget,
                       public AccessibleElement
{
public:
    enum Stage {
        kStageAttack = 0,
        kStageDecay,
        kStageSustain,
        kStageRelease,
        kStageCount
    };

    enum HandleId {
        kHandleNone = -1,
        kHandleAttack = 0,      // Attack time (X)
        kHandleDecay,           // Decay time (X) and Sustain level (Y)
        kHandleSustainEnd,      // Sustain stage visual end (X) and Sustain level (Y)
        kHandleRelease,         // Release time (X)
        kHandleAttackTension,   // Attack curve tension
        kHandleDecayTension,    // Decay curve tension
        kHandleReleaseTension,  // Release curve tension
        kHandleCount
    };

    struct PlayheadTelemetry {
        int32_t stage;    // 0=A, 1=D, 2=S, 3=R, -1=idle
        float progress;   // 0.0f to 1.0f within stage
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorEnvelopeStageChanged(VectorEnvelope* env, int stage, float time, float level, float tension)
        {
            (void)env; (void)stage; (void)time; (void)level; (void)tension;
        }
        virtual void vectorEnvelopeDragStarted(VectorEnvelope* env, int handleId)
        {
            (void)env; (void)handleId;
        }
        virtual void vectorEnvelopeDragFinished(VectorEnvelope* env, int handleId)
        {
            (void)env; (void)handleId;
        }
        virtual void vectorEnvelopeContextMenu(VectorEnvelope* env, int32_t x, int32_t y)
        {
            (void)env; (void)x; (void)y;
        }
    };

    typedef void (*ContextMenuCallback)(Widget* widget, int32_t x, int32_t y, void* userData);

    explicit VectorEnvelope(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fAttack(0.15f),
          fDecay(0.20f),
          fSustain(0.70f),
          fRelease(0.35f),
          fSustainWidth(0.20f),
          fAttackTension(0.0f),
          fDecayTension(-0.35f),
          fReleaseTension(-0.35f),
          fMaxAttackMs(5000.0f),
          fMaxDecayMs(5000.0f),
          fMaxReleaseMs(10000.0f),
          fActiveHandle(kHandleNone),
          fHoverHandle(kHandleNone),
          fDragging(false),
          fShowGrid(true),
          fShowTensionHandles(true),
          fShowPlayhead(true),
          fShowLabels(true),
          fCornerRadius(4.0f),
          fHandleRadius(6.0f),
          fPlayheadStage(-1),
          fPlayheadProgress(0.0f),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(42, 45, 52)),
          fBorderColor(Color(60, 64, 72)),
          fCurveColor(Color(80, 180, 255)),
          fFillColorTop(Color(52, 152, 219, 130)),
          fFillColorBottom(Color(52, 152, 219, 15)),
          fHandleColor(Color(245, 248, 255)),
          fHandleBorderColor(Color(30, 32, 38)),
          fHandleGlowColor(Color(52, 152, 219, 120)),
          fPlayheadColor(Color(255, 255, 255)),
          fPlayheadGlowColor(Color(52, 152, 219, 160)),
          fTextColor(Color(120, 130, 145)),
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    explicit VectorEnvelope(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fAttack(0.15f),
          fDecay(0.20f),
          fSustain(0.70f),
          fRelease(0.35f),
          fSustainWidth(0.20f),
          fAttackTension(0.0f),
          fDecayTension(-0.35f),
          fReleaseTension(-0.35f),
          fMaxAttackMs(5000.0f),
          fMaxDecayMs(5000.0f),
          fMaxReleaseMs(10000.0f),
          fActiveHandle(kHandleNone),
          fHoverHandle(kHandleNone),
          fDragging(false),
          fShowGrid(true),
          fShowTensionHandles(true),
          fShowPlayhead(true),
          fShowLabels(true),
          fCornerRadius(4.0f),
          fHandleRadius(6.0f),
          fPlayheadStage(-1),
          fPlayheadProgress(0.0f),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(42, 45, 52)),
          fBorderColor(Color(60, 64, 72)),
          fCurveColor(Color(80, 180, 255)),
          fFillColorTop(Color(52, 152, 219, 130)),
          fFillColorBottom(Color(52, 152, 219, 15)),
          fHandleColor(Color(245, 248, 255)),
          fHandleBorderColor(Color(30, 32, 38)),
          fHandleGlowColor(Color(52, 152, 219, 120)),
          fPlayheadColor(Color(255, 255, 255)),
          fPlayheadGlowColor(Color(52, 152, 219, 160)),
          fTextColor(Color(120, 130, 145)),
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    explicit VectorEnvelope(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fAttack(0.15f),
          fDecay(0.20f),
          fSustain(0.70f),
          fRelease(0.35f),
          fSustainWidth(0.20f),
          fAttackTension(0.0f),
          fDecayTension(-0.35f),
          fReleaseTension(-0.35f),
          fMaxAttackMs(5000.0f),
          fMaxDecayMs(5000.0f),
          fMaxReleaseMs(10000.0f),
          fActiveHandle(kHandleNone),
          fHoverHandle(kHandleNone),
          fDragging(false),
          fShowGrid(true),
          fShowTensionHandles(true),
          fShowPlayhead(true),
          fShowLabels(true),
          fCornerRadius(4.0f),
          fHandleRadius(6.0f),
          fPlayheadStage(-1),
          fPlayheadProgress(0.0f),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(42, 45, 52)),
          fBorderColor(Color(60, 64, 72)),
          fCurveColor(Color(80, 180, 255)),
          fFillColorTop(Color(52, 152, 219, 130)),
          fFillColorBottom(Color(52, 152, 219, 15)),
          fHandleColor(Color(245, 248, 255)),
          fHandleBorderColor(Color(30, 32, 38)),
          fHandleGlowColor(Color(52, 152, 219, 120)),
          fPlayheadColor(Color(255, 255, 255)),
          fPlayheadGlowColor(Color(52, 152, 219, 160)),
          fTextColor(Color(120, 130, 145)),
          fUserCallback(nullptr),
          fContextMenuCb(nullptr),
          fContextMenuUserData(nullptr)
    {
        initAccessibility();
    }

    ~VectorEnvelope() override {}

    // ----------------------------------------------------------------------------------------------------------------
    // Envelope Parameters

    float getAttack() const noexcept { return fAttack; }
    float getDecay() const noexcept { return fDecay; }
    float getSustain() const noexcept { return fSustain; }
    float getRelease() const noexcept { return fRelease; }

    void setAttack(const float attack, const bool sendCallback = false) noexcept
    {
        const float clamped = std::max(0.01f, std::min(1.0f, attack));
        if (d_isEqual(fAttack, clamped))
            return;
        fAttack = clamped;
        updateAccessibility();
        repaint();
        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorEnvelopeStageChanged(this, kStageAttack, fAttack, 1.0f, fAttackTension);
    }

    void setDecay(const float decay, const bool sendCallback = false) noexcept
    {
        const float clamped = std::max(0.01f, std::min(1.0f, decay));
        if (d_isEqual(fDecay, clamped))
            return;
        fDecay = clamped;
        updateAccessibility();
        repaint();
        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorEnvelopeStageChanged(this, kStageDecay, fDecay, fSustain, fDecayTension);
    }

    void setSustain(const float sustain, const bool sendCallback = false) noexcept
    {
        const float clamped = std::max(0.0f, std::min(1.0f, sustain));
        if (d_isEqual(fSustain, clamped))
            return;
        fSustain = clamped;
        updateAccessibility();
        repaint();
        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorEnvelopeStageChanged(this, kStageSustain, fSustainWidth, fSustain, 0.0f);
    }

    void setRelease(const float release, const bool sendCallback = false) noexcept
    {
        const float clamped = std::max(0.01f, std::min(1.0f, release));
        if (d_isEqual(fRelease, clamped))
            return;
        fRelease = clamped;
        updateAccessibility();
        repaint();
        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorEnvelopeStageChanged(this, kStageRelease, fRelease, 0.0f, fReleaseTension);
    }

    void setADSR(const float a, const float d, const float s, const float r, const bool sendCallback = false) noexcept
    {
        setAttack(a, false);
        setDecay(d, false);
        setSustain(s, false);
        setRelease(r, false);
        if (sendCallback && fUserCallback != nullptr)
        {
            fUserCallback->vectorEnvelopeStageChanged(this, kStageAttack, fAttack, 1.0f, fAttackTension);
            fUserCallback->vectorEnvelopeStageChanged(this, kStageDecay, fDecay, fSustain, fDecayTension);
            fUserCallback->vectorEnvelopeStageChanged(this, kStageSustain, fSustainWidth, fSustain, 0.0f);
            fUserCallback->vectorEnvelopeStageChanged(this, kStageRelease, fRelease, 0.0f, fReleaseTension);
        }
    }

    // Tension curves [-1.0f to +1.0f]
    float getAttackTension() const noexcept { return fAttackTension; }
    float getDecayTension() const noexcept { return fDecayTension; }
    float getReleaseTension() const noexcept { return fReleaseTension; }

    void setAttackTension(const float tension, const bool sendCallback = false) noexcept
    {
        const float clamped = std::max(-1.0f, std::min(1.0f, tension));
        if (d_isEqual(fAttackTension, clamped))
            return;
        fAttackTension = clamped;
        repaint();
        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorEnvelopeStageChanged(this, kStageAttack, fAttack, 1.0f, fAttackTension);
    }

    void setDecayTension(const float tension, const bool sendCallback = false) noexcept
    {
        const float clamped = std::max(-1.0f, std::min(1.0f, tension));
        if (d_isEqual(fDecayTension, clamped))
            return;
        fDecayTension = clamped;
        repaint();
        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorEnvelopeStageChanged(this, kStageDecay, fDecay, fSustain, fDecayTension);
    }

    void setReleaseTension(const float tension, const bool sendCallback = false) noexcept
    {
        const float clamped = std::max(-1.0f, std::min(1.0f, tension));
        if (d_isEqual(fReleaseTension, clamped))
            return;
        fReleaseTension = clamped;
        repaint();
        if (sendCallback && fUserCallback != nullptr)
            fUserCallback->vectorEnvelopeStageChanged(this, kStageRelease, fRelease, 0.0f, fReleaseTension);
    }

    // Millisecond scaling helpers
    float getAttackMs() const noexcept { return fAttack * fMaxAttackMs; }
    float getDecayMs() const noexcept { return fDecay * fMaxDecayMs; }
    float getReleaseMs() const noexcept { return fRelease * fMaxReleaseMs; }

    void setAttackMs(const float ms, const bool sendCallback = false) noexcept
    {
        setAttack(ms / fMaxAttackMs, sendCallback);
    }

    void setDecayMs(const float ms, const bool sendCallback = false) noexcept
    {
        setDecay(ms / fMaxDecayMs, sendCallback);
    }

    void setReleaseMs(const float ms, const bool sendCallback = false) noexcept
    {
        setRelease(ms / fMaxReleaseMs, sendCallback);
    }

    void setMaxTimesMs(const float maxAttackMs, const float maxDecayMs, const float maxReleaseMs) noexcept
    {
        fMaxAttackMs = std::max(10.0f, maxAttackMs);
        fMaxDecayMs = std::max(10.0f, maxDecayMs);
        fMaxReleaseMs = std::max(10.0f, maxReleaseMs);
        updateAccessibility();
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Real-Time Playhead & Telemetry Tracking

    void setPlayhead(const int stage, const float progress) noexcept
    {
        fPlayheadStage = stage;
        fPlayheadProgress = std::max(0.0f, std::min(1.0f, progress));
        repaint();
    }

    void setPlayheadNormalized(const float pos) noexcept
    {
        if (pos < 0.0f || pos > 1.0f)
        {
            clearPlayhead();
            return;
        }

        const float total = fAttack + fDecay + fSustainWidth + fRelease;
        const float aNorm = fAttack / total;
        const float dNorm = fDecay / total;
        const float sNorm = fSustainWidth / total;

        if (pos < aNorm)
        {
            setPlayhead(kStageAttack, pos / aNorm);
        }
        else if (pos < aNorm + dNorm)
        {
            setPlayhead(kStageDecay, (pos - aNorm) / dNorm);
        }
        else if (pos < aNorm + dNorm + sNorm)
        {
            setPlayhead(kStageSustain, (pos - aNorm - dNorm) / sNorm);
        }
        else
        {
            const float rNorm = fRelease / total;
            setPlayhead(kStageRelease, (pos - aNorm - dNorm - sNorm) / rNorm);
        }
    }

    void clearPlayhead() noexcept
    {
        if (fPlayheadStage != -1)
        {
            fPlayheadStage = -1;
            fPlayheadProgress = 0.0f;
            repaint();
        }
    }

    bool isPlayheadActive() const noexcept
    {
        return fPlayheadStage >= 0 && fPlayheadStage < kStageCount;
    }

    bool processTelemetry(const uint32_t tag, const void* const data, const uint32_t size) noexcept
    {
        (void)tag;
        if (data != nullptr && size == sizeof(PlayheadTelemetry))
        {
            const PlayheadTelemetry* const t = static_cast<const PlayheadTelemetry*>(data);
            setPlayhead(t->stage, t->progress);
            return true;
        }
        return false;
    }

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    bool performAccessibleAction(const AccessibleAction action) override
    {
        if (action == kAccessibleActionIncrement)
        {
            adjustActiveHandle(0.05f, 0.05f);
            return true;
        }
        if (action == kAccessibleActionDecrement)
        {
            adjustActiveHandle(-0.05f, -0.05f);
            return true;
        }
        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Styling & Appearance

    void setBgColor(const Color& color) noexcept { fBgColor = color; repaint(); }
    void setGridColor(const Color& color) noexcept { fGridColor = color; repaint(); }
    void setBorderColor(const Color& color) noexcept { fBorderColor = color; repaint(); }
    void setCurveColor(const Color& color) noexcept { fCurveColor = color; repaint(); }
    void setFillColors(const Color& top, const Color& bottom) noexcept
    {
        fFillColorTop = top;
        fFillColorBottom = bottom;
        repaint();
    }
    void setHandleColor(const Color& color) noexcept { fHandleColor = color; repaint(); }
    void setPlayheadColor(const Color& color) noexcept { fPlayheadColor = color; repaint(); }

    void setShowGrid(const bool show) noexcept { fShowGrid = show; repaint(); }
    void setShowTensionHandles(const bool show) noexcept { fShowTensionHandles = show; repaint(); }
    void setShowPlayhead(const bool show) noexcept { fShowPlayhead = show; repaint(); }
    void setShowLabels(const bool show) noexcept { fShowLabels = show; repaint(); }
    void setCornerRadius(const float radius) noexcept { fCornerRadius = radius; repaint(); }

    // ----------------------------------------------------------------------------------------------------------------
    // Callbacks

    void setCallback(Callback* const cb) noexcept { fUserCallback = cb; }

    void setContextMenuCallback(ContextMenuCallback cb, void* userData = nullptr) noexcept
    {
        fContextMenuCb = cb;
        fContextMenuUserData = userData;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Event Handling

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button == kMouseButtonRight && ev.press && contains(ev.pos))
        {
            if (fUserCallback != nullptr)
                fUserCallback->vectorEnvelopeContextMenu(this, ev.pos.getX(), ev.pos.getY());
            if (fContextMenuCb != nullptr)
                fContextMenuCb(this, ev.pos.getX(), ev.pos.getY(), fContextMenuUserData);
            return true;
        }

        if (ev.button != kMouseButtonLeft)
            return false;

        const float mx = static_cast<float>(ev.pos.getX());
        const float my = static_cast<float>(ev.pos.getY());

        if (ev.press)
        {
            const HandleId handle = hitTestHandle(mx, my);
            if (handle != kHandleNone)
            {
                fActiveHandle = handle;
                fDragging = true;
                fDragStartX = mx;
                fDragStartY = my;
                repaint();

                if (fUserCallback != nullptr)
                    fUserCallback->vectorEnvelopeDragStarted(this, fActiveHandle);

                return true;
            }
        }
        else if (fDragging)
        {
            const HandleId finishedHandle = fActiveHandle;
            fDragging = false;
            fActiveHandle = kHandleNone;
            repaint();

            if (fUserCallback != nullptr)
                fUserCallback->vectorEnvelopeDragFinished(this, finishedHandle);

            return true;
        }

        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        const float mx = static_cast<float>(ev.pos.getX());
        const float my = static_cast<float>(ev.pos.getY());

        if (fDragging && fActiveHandle != kHandleNone)
        {
            updateHandleDrag(mx, my);
            repaint();
            return true;
        }

        // Hover detection
        const HandleId hover = hitTestHandle(mx, my);
        if (hover != fHoverHandle)
        {
            fHoverHandle = hover;
            repaint();
        }

        return false;
    }

    bool onKeyboard(const KeyboardEvent& ev) override
    {
        if (!ev.press)
            return false;

        switch (ev.key)
        {
        case kKeyTab:
            if (ev.mod & kModifierShift)
            {
                fActiveHandle = static_cast<HandleId>((fActiveHandle <= 0) ? (kHandleCount - 1) : (fActiveHandle - 1));
            }
            else
            {
                fActiveHandle = static_cast<HandleId>((fActiveHandle >= kHandleCount - 1) ? 0 : (fActiveHandle + 1));
            }
            repaint();
            return true;
        case kKeyUp:
            adjustActiveHandle(0.0f, 0.05f);
            return true;
        case kKeyDown:
            adjustActiveHandle(0.0f, -0.05f);
            return true;
        case kKeyLeft:
            adjustActiveHandle(-0.05f, 0.0f);
            return true;
        case kKeyRight:
            adjustActiveHandle(0.05f, 0.0f);
            return true;
        default:
            break;
        }

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // NanoVG Rendering

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 10.0f || h <= 10.0f)
            return;

        const float padL = 16.0f;
        const float padR = 16.0f;
        const float padT = 16.0f;
        const float padB = fShowLabels ? 26.0f : 16.0f;
        const float plotW = w - padL - padR;
        const float plotH = h - padT - padB;
        if (plotW <= 0.0f || plotH <= 0.0f)
            return;

        // 1. Background Container
        beginPath();
        roundedRect(0.5f, 0.5f, w - 1.0f, h - 1.0f, fCornerRadius);
        fillColor(fBgColor);
        fill();
        strokeColor(fBorderColor);
        strokeWidth(1.0f);
        stroke();

        // 2. Horizontal Level Grid (25%, 50%, 75%, 100%)
        if (fShowGrid)
        {
            beginPath();
            for (int i = 1; i <= 3; ++i)
            {
                const float gy = padT + plotH * (1.0f - i * 0.25f);
                moveTo(padL, gy);
                lineTo(padL + plotW, gy);
            }
            strokeColor(fGridColor);
            strokeWidth(1.0f);
            stroke();
        }

        // Calculate stage horizontal widths
        const float totalWeight = fAttack + fDecay + fSustainWidth + fRelease;
        const float aW = plotW * (fAttack / totalWeight);
        const float dW = plotW * (fDecay / totalWeight);
        const float sW = plotW * (fSustainWidth / totalWeight);
        const float rW = plotW * (fRelease / totalWeight);

        const float x0 = padL;
        const float y0 = padT + plotH;

        const float x1 = x0 + aW;
        const float y1 = padT; // Peak level = 1.0

        const float x2 = x1 + dW;
        const float y2 = padT + plotH * (1.0f - fSustain); // Sustain level

        const float x3 = x2 + sW;
        const float y3 = y2; // Sustain hold

        const float x4 = x3 + rW;
        const float y4 = padT + plotH; // Zero level

        // Tension Bézier Control Points
        const float cAx = (x0 + x1) * 0.5f;
        const float cAy = y0 + (y1 - y0) * (0.5f + 0.45f * fAttackTension);

        const float cDx = (x1 + x2) * 0.5f;
        const float cDy = y1 + (y2 - y1) * (0.5f - 0.45f * fDecayTension);

        const float cRx = (x3 + x4) * 0.5f;
        const float cRy = y3 + (y4 - y3) * (0.5f - 0.45f * fReleaseTension);

        // 3. Stage Dividers & Labels
        if (fShowGrid)
        {
            beginPath();
            moveTo(x1, padT); lineTo(x1, padT + plotH);
            moveTo(x2, padT); lineTo(x2, padT + plotH);
            moveTo(x3, padT); lineTo(x3, padT + plotH);
            strokeColor(fGridColor);
            strokeWidth(1.0f);
            stroke();
        }

        if (fShowLabels)
        {
            fontSize(11.0f);
            textAlign(ALIGN_CENTER | ALIGN_TOP);
            fillColor(fTextColor);
            text((x0 + x1) * 0.5f, padT + plotH + 4.0f, "A", nullptr);
            text((x1 + x2) * 0.5f, padT + plotH + 4.0f, "D", nullptr);
            text((x2 + x3) * 0.5f, padT + plotH + 4.0f, "S", nullptr);
            text((x3 + x4) * 0.5f, padT + plotH + 4.0f, "R", nullptr);
        }

        // 4. Gradient Fill Under Envelope
        beginPath();
        moveTo(x0, y0);
        quadTo(cAx, cAy, x1, y1);
        quadTo(cDx, cDy, x2, y2);
        lineTo(x3, y3);
        quadTo(cRx, cRy, x4, y4);
        lineTo(x4, padT + plotH);
        lineTo(x0, padT + plotH);
        closePath();

        const Paint envPaint = linearGradient(0.0f, padT, 0.0f, padT + plotH, fFillColorTop, fFillColorBottom);
        fillPaint(envPaint);
        fill();

        // 5. Envelope Contour Line
        beginPath();
        moveTo(x0, y0);
        quadTo(cAx, cAy, x1, y1);
        quadTo(cDx, cDy, x2, y2);
        lineTo(x3, y3);
        quadTo(cRx, cRy, x4, y4);
        strokeColor(fCurveColor);
        strokeWidth(2.5f);
        stroke();

        // 6. Playhead Indicator (if active)
        if (fShowPlayhead && fPlayheadStage >= 0 && fPlayheadStage < kStageCount)
        {
            float px = 0.0f;
            float py = 0.0f;
            const float t = fPlayheadProgress;
            const float it = 1.0f - t;

            switch (fPlayheadStage)
            {
            case kStageAttack:
                px = it * it * x0 + 2.0f * it * t * cAx + t * t * x1;
                py = it * it * y0 + 2.0f * it * t * cAy + t * t * y1;
                break;
            case kStageDecay:
                px = it * it * x1 + 2.0f * it * t * cDx + t * t * x2;
                py = it * it * y1 + 2.0f * it * t * cDy + t * t * y2;
                break;
            case kStageSustain:
                px = x2 + t * (x3 - x2);
                py = y2;
                break;
            case kStageRelease:
                px = it * it * x3 + 2.0f * it * t * cRx + t * t * x4;
                py = it * it * y3 + 2.0f * it * t * cRy + t * t * y4;
                break;
            default:
                break;
            }

            // Vertical neon cursor line
            beginPath();
            moveTo(px, padT);
            lineTo(px, padT + plotH);
            strokeColor(fPlayheadGlowColor);
            strokeWidth(1.5f);
            stroke();

            // Glowing orb at exact playhead position
            beginPath();
            circle(px, py, 10.0f);
            fillColor(fPlayheadGlowColor);
            fill();

            beginPath();
            circle(px, py, 5.0f);
            fillColor(fPlayheadColor);
            fill();
        }

        // 7. Interactive Handles
        renderHandle(x1, y1, kHandleAttack);
        renderHandle(x2, y2, kHandleDecay);
        renderHandle(x3, y3, kHandleSustainEnd);
        renderHandle(x4, y4, kHandleRelease);

        // Tension Handles
        if (fShowTensionHandles)
        {
            const float tAx = 0.25f * x0 + 0.5f * cAx + 0.25f * x1;
            const float tAy = 0.25f * y0 + 0.5f * cAy + 0.25f * y1;
            renderTensionHandle(tAx, tAy, kHandleAttackTension);

            const float tDx = 0.25f * x1 + 0.5f * cDx + 0.25f * x2;
            const float tDy = 0.25f * y1 + 0.5f * cDy + 0.25f * y2;
            renderTensionHandle(tDx, tDy, kHandleDecayTension);

            const float tRx = 0.25f * x3 + 0.5f * cRx + 0.25f * x4;
            const float tRy = 0.25f * y3 + 0.5f * cRy + 0.25f * y4;
            renderTensionHandle(tRx, tRy, kHandleReleaseTension);
        }

        // 8. Tooltip HUD during Drag
        if (fDragging && fActiveHandle != kHandleNone)
        {
            char buf[64];
            switch (fActiveHandle)
            {
            case kHandleAttack:
                std::snprintf(buf, sizeof(buf), "Attack: %.1f ms", getAttackMs());
                break;
            case kHandleDecay:
                std::snprintf(buf, sizeof(buf), "Decay: %.1f ms | Sus: %.0f%%", getDecayMs(), fSustain * 100.0f);
                break;
            case kHandleSustainEnd:
                std::snprintf(buf, sizeof(buf), "Sustain: %.0f%%", fSustain * 100.0f);
                break;
            case kHandleRelease:
                std::snprintf(buf, sizeof(buf), "Release: %.1f ms", getReleaseMs());
                break;
            case kHandleAttackTension:
                std::snprintf(buf, sizeof(buf), "Curve: %+.2f", fAttackTension);
                break;
            case kHandleDecayTension:
                std::snprintf(buf, sizeof(buf), "Curve: %+.2f", fDecayTension);
                break;
            case kHandleReleaseTension:
                std::snprintf(buf, sizeof(buf), "Curve: %+.2f", fReleaseTension);
                break;
            default:
                buf[0] = '\0';
                break;
            }

            if (buf[0] != '\0')
            {
                fontSize(11.0f);
                textAlign(ALIGN_RIGHT | ALIGN_TOP);
                fillColor(fCurveColor);
                text(w - padR, padT + 2.0f, buf, nullptr);
            }
        }
    }

private:
    float fAttack;
    float fDecay;
    float fSustain;
    float fRelease;
    float fSustainWidth;

    float fAttackTension;
    float fDecayTension;
    float fReleaseTension;

    float fMaxAttackMs;
    float fMaxDecayMs;
    float fMaxReleaseMs;

    HandleId fActiveHandle;
    HandleId fHoverHandle;
    bool fDragging;
    float fDragStartX;
    float fDragStartY;

    bool fShowGrid;
    bool fShowTensionHandles;
    bool fShowPlayhead;
    bool fShowLabels;

    float fCornerRadius;
    float fHandleRadius;

    int32_t fPlayheadStage;
    float fPlayheadProgress;

    Color fBgColor;
    Color fGridColor;
    Color fBorderColor;
    Color fCurveColor;
    Color fFillColorTop;
    Color fFillColorBottom;
    Color fHandleColor;
    Color fHandleBorderColor;
    Color fHandleGlowColor;
    Color fPlayheadColor;
    Color fPlayheadGlowColor;
    Color fTextColor;

    Callback* fUserCallback;
    ContextMenuCallback fContextMenuCb;
    void* fContextMenuUserData;

    void renderHandle(const float x, const float y, const HandleId id)
    {
        const bool active = (fActiveHandle == id);
        const bool hover = (fHoverHandle == id);
        const float r = active ? (fHandleRadius + 2.5f) : (hover ? (fHandleRadius + 1.5f) : fHandleRadius);

        if (active || hover)
        {
            beginPath();
            circle(x, y, r + 4.0f);
            fillColor(fHandleGlowColor);
            fill();
        }

        beginPath();
        circle(x, y, r);
        fillColor(fHandleColor);
        fill();
        strokeColor(fHandleBorderColor);
        strokeWidth(1.5f);
        stroke();
    }

    void renderTensionHandle(const float x, const float y, const HandleId id)
    {
        const bool active = (fActiveHandle == id);
        const bool hover = (fHoverHandle == id);
        const float r = active ? 4.5f : (hover ? 3.5f : 2.5f);

        beginPath();
        circle(x, y, r);
        fillColor(active ? fPlayheadColor : (hover ? fCurveColor : fGridColor));
        fill();
        strokeColor(fBorderColor);
        strokeWidth(1.0f);
        stroke();
    }

    HandleId hitTestHandle(const float mx, const float my) const
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        const float padL = 16.0f;
        const float padR = 16.0f;
        const float padT = 16.0f;
        const float padB = fShowLabels ? 26.0f : 16.0f;
        const float plotW = w - padL - padR;
        const float plotH = h - padT - padB;

        const float totalWeight = fAttack + fDecay + fSustainWidth + fRelease;
        const float aW = plotW * (fAttack / totalWeight);
        const float dW = plotW * (fDecay / totalWeight);
        const float sW = plotW * (fSustainWidth / totalWeight);
        const float rW = plotW * (fRelease / totalWeight);

        const float x0 = padL;
        const float y0 = padT + plotH;
        const float x1 = x0 + aW;
        const float y1 = padT;
        const float x2 = x1 + dW;
        const float y2 = padT + plotH * (1.0f - fSustain);
        const float x3 = x2 + sW;
        const float y3 = y2;
        const float x4 = x3 + rW;
        const float y4 = padT + plotH;

        const float hitRadiusSq = 12.0f * 12.0f;

        auto distSq = [](float ax, float ay, float bx, float by) {
            const float dx = ax - bx;
            const float dy = ay - by;
            return dx * dx + dy * dy;
        };

        if (distSq(mx, my, x1, y1) <= hitRadiusSq) return kHandleAttack;
        if (distSq(mx, my, x2, y2) <= hitRadiusSq) return kHandleDecay;
        if (distSq(mx, my, x3, y3) <= hitRadiusSq) return kHandleSustainEnd;
        if (distSq(mx, my, x4, y4) <= hitRadiusSq) return kHandleRelease;

        if (fShowTensionHandles)
        {
            const float cAx = (x0 + x1) * 0.5f;
            const float cAy = y0 + (y1 - y0) * (0.5f + 0.45f * fAttackTension);
            const float tAx = 0.25f * x0 + 0.5f * cAx + 0.25f * x1;
            const float tAy = 0.25f * y0 + 0.5f * cAy + 0.25f * y1;
            if (distSq(mx, my, tAx, tAy) <= hitRadiusSq) return kHandleAttackTension;

            const float cDx = (x1 + x2) * 0.5f;
            const float cDy = y1 + (y2 - y1) * (0.5f - 0.45f * fDecayTension);
            const float tDx = 0.25f * x1 + 0.5f * cDx + 0.25f * x2;
            const float tDy = 0.25f * y1 + 0.5f * cDy + 0.25f * y2;
            if (distSq(mx, my, tDx, tDy) <= hitRadiusSq) return kHandleDecayTension;

            const float cRx = (x3 + x4) * 0.5f;
            const float cRy = y3 + (y4 - y3) * (0.5f - 0.45f * fReleaseTension);
            const float tRx = 0.25f * x3 + 0.5f * cRx + 0.25f * x4;
            const float tRy = 0.25f * y3 + 0.5f * cRy + 0.25f * y4;
            if (distSq(mx, my, tRx, tRy) <= hitRadiusSq) return kHandleReleaseTension;
        }

        return kHandleNone;
    }

    void updateHandleDrag(const float mx, const float my)
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        const float padL = 16.0f;
        const float padR = 16.0f;
        const float padT = 16.0f;
        const float padB = fShowLabels ? 26.0f : 16.0f;
        const float plotW = w - padL - padR;
        const float plotH = h - padT - padB;
        if (plotW <= 0.0f || plotH <= 0.0f)
            return;

        const float normX = std::max(0.0f, std::min(1.0f, (mx - padL) / plotW));
        const float normY = std::max(0.0f, std::min(1.0f, 1.0f - (my - padT) / plotH));

        switch (fActiveHandle)
        {
        case kHandleAttack:
            setAttack(normX * 2.0f, true);
            break;
        case kHandleDecay:
            setDecay(normX * 1.5f, true);
            setSustain(normY, true);
            break;
        case kHandleSustainEnd:
            setSustain(normY, true);
            break;
        case kHandleRelease:
            setRelease((1.0f - normX) * 2.0f, true);
            break;
        case kHandleAttackTension:
            setAttackTension((normY - 0.5f) * 2.0f, true);
            break;
        case kHandleDecayTension:
            setDecayTension((normY - 0.5f) * 2.0f, true);
            break;
        case kHandleReleaseTension:
            setReleaseTension((normY - 0.5f) * 2.0f, true);
            break;
        default:
            break;
        }
    }

    void adjustActiveHandle(const float dx, const float dy)
    {
        switch (fActiveHandle)
        {
        case kHandleAttack:
            setAttack(fAttack + dx, true);
            break;
        case kHandleDecay:
            setDecay(fDecay + dx, true);
            setSustain(fSustain + dy, true);
            break;
        case kHandleSustainEnd:
            setSustain(fSustain + dy, true);
            break;
        case kHandleRelease:
            setRelease(fRelease + dx, true);
            break;
        case kHandleAttackTension:
            setAttackTension(fAttackTension + dy, true);
            break;
        case kHandleDecayTension:
            setDecayTension(fDecayTension + dy, true);
            break;
        case kHandleReleaseTension:
            setReleaseTension(fReleaseTension + dy, true);
            break;
        default:
            break;
        }
    }

    void initAccessibility()
    {
        setAccessibleRole(kAccessibleRoleIndicator);
        setAccessibleLabel("ADSR Envelope");
        setAccessibleRange(0.0f, 1.0f, 0.01f);
        updateAccessibility();
    }

    void updateAccessibility()
    {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "A: %.0fms, D: %.0fms, S: %.0f%%, R: %.0fms",
                      getAttackMs(), getDecayMs(), fSustain * 100.0f, getReleaseMs());
        setAccessibleValue(fSustain, buf);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorEnvelope)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_ENVELOPE_HPP_INCLUDED
