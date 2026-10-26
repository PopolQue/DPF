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

#ifndef DGL_VECTOR_PLOT_HPP_INCLUDED
#define DGL_VECTOR_PLOT_HPP_INCLUDED

#include "../Accessibility.hpp"
#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/String.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
   High-performance, resolution-independent NanoVG vector Bode plot and filter curve visualizer/editor.
   Features logarithmic frequency axis (20 Hz - 20 kHz), decibel vertical scale, digital biquad transfer
   function calculation, interactive draggable filter band pucks, composite curve shading, and accessibility.
 */
class VectorPlot : public NanoSubWidget,
                   public AccessibleElement
{
public:
    enum FilterType {
        kFilterLowpass = 0,
        kFilterHighpass,
        kFilterBandpass,
        kFilterNotch,
        kFilterPeak,
        kFilterLowShelf,
        kFilterHighShelf
    };

    struct FilterBand {
        FilterType type;
        float freq;      // Frequency in Hz (20 to 20000)
        float gainDb;    // Gain in dB (-24 to +24)
        float q;         // Q factor / bandwidth (0.1 to 10.0)
        bool enabled;
        Color color;

        FilterBand()
            : type(kFilterPeak),
              freq(1000.0f),
              gainDb(0.0f),
              q(1.0f),
              enabled(true),
              color(Color(80, 180, 255))
        {
        }

        FilterBand(FilterType t, float f, float g, float qval, const Color& c)
            : type(t),
              freq(f),
              gainDb(g),
              q(qval),
              enabled(true),
              color(c)
        {
        }

        void computeCoeffs(const double sampleRate, double& b0, double& b1, double& b2, double& a1, double& a2) const
        {
            const double w0 = 2.0 * M_PI * static_cast<double>(freq) / sampleRate;
            const double cosw0 = std::cos(w0);
            const double sinw0 = std::sin(w0);
            const double alpha = sinw0 / (2.0 * static_cast<double>(q > 0.05f ? q : 0.05f));
            const double A = std::pow(10.0, static_cast<double>(gainDb) / 40.0);

            double a0 = 1.0;

            switch (type)
            {
            case kFilterLowpass:
                b0 = (1.0 - cosw0) * 0.5;
                b1 = 1.0 - cosw0;
                b2 = (1.0 - cosw0) * 0.5;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cosw0;
                a2 = 1.0 - alpha;
                break;
            case kFilterHighpass:
                b0 = (1.0 + cosw0) * 0.5;
                b1 = -(1.0 + cosw0);
                b2 = (1.0 + cosw0) * 0.5;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cosw0;
                a2 = 1.0 - alpha;
                break;
            case kFilterBandpass:
                b0 = alpha;
                b1 = 0.0;
                b2 = -alpha;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cosw0;
                a2 = 1.0 - alpha;
                break;
            case kFilterNotch:
                b0 = 1.0;
                b1 = -2.0 * cosw0;
                b2 = 1.0;
                a0 = 1.0 + alpha;
                a1 = -2.0 * cosw0;
                a2 = 1.0 - alpha;
                break;
            case kFilterPeak:
                b0 = 1.0 + alpha * A;
                b1 = -2.0 * cosw0;
                b2 = 1.0 - alpha * A;
                a0 = 1.0 + alpha / A;
                a1 = -2.0 * cosw0;
                a2 = 1.0 - alpha / A;
                break;
            case kFilterLowShelf:
            {
                const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;
                b0 = A * ((A + 1.0) - (A - 1.0) * cosw0 + twoSqrtAAlpha);
                b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosw0);
                b2 = A * ((A + 1.0) - (A - 1.0) * cosw0 - twoSqrtAAlpha);
                a0 = (A + 1.0) + (A - 1.0) * cosw0 + twoSqrtAAlpha;
                a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosw0);
                a2 = (A + 1.0) + (A - 1.0) * cosw0 - twoSqrtAAlpha;
                break;
            }
            case kFilterHighShelf:
            {
                const double twoSqrtAAlpha = 2.0 * std::sqrt(A) * alpha;
                b0 = A * ((A + 1.0) + (A - 1.0) * cosw0 + twoSqrtAAlpha);
                b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw0);
                b2 = A * ((A + 1.0) + (A - 1.0) * cosw0 - twoSqrtAAlpha);
                a0 = (A + 1.0) - (A - 1.0) * cosw0 + twoSqrtAAlpha;
                a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosw0);
                a2 = (A + 1.0) - (A - 1.0) * cosw0 - twoSqrtAAlpha;
                break;
            }
            }

            // Normalize by a0
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
            a1 /= a0;
            a2 /= a0;
        }

        double evaluateMagnitudeDb(const double evalFreq, const double sampleRate) const
        {
            if (!enabled)
                return 0.0;

            double b0, b1, b2, a1, a2;
            computeCoeffs(sampleRate, b0, b1, b2, a1, a2);

            const double w = 2.0 * M_PI * evalFreq / sampleRate;
            const double cosw = std::cos(w);
            const double cos2w = std::cos(2.0 * w);
            const double sinw = std::sin(w);
            const double sin2w = std::sin(2.0 * w);

            const double numReal = b0 + b1 * cosw + b2 * cos2w;
            const double numImag = -b1 * sinw - b2 * sin2w;
            const double denReal = 1.0 + a1 * cosw + a2 * cos2w;
            const double denImag = -a1 * sinw - a2 * sin2w;

            const double numMagSq = numReal * numReal + numImag * numImag;
            const double denMagSq = denReal * denReal + denImag * denImag;

            if (denMagSq <= 1e-12)
                return 0.0;

            const double magSq = numMagSq / denMagSq;
            return 10.0 * std::log10(std::max(1e-12, magSq));
        }
    };

    typedef double (*ResponseFunc)(double freqHz, void* userData);

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorPlotBandChanged(VectorPlot* plot, int bandIndex, float freq, float gainDb, float q)
        {
            (void)plot; (void)bandIndex; (void)freq; (void)gainDb; (void)q;
        }
        virtual void vectorPlotBandSelected(VectorPlot* plot, int bandIndex)
        {
            (void)plot; (void)bandIndex;
        }
        virtual void vectorPlotContextMenu(VectorPlot* plot, int32_t x, int32_t y)
        {
            (void)plot; (void)x; (void)y;
        }
    };

    typedef void (*ContextMenuCallback)(Widget* widget, int32_t x, int32_t y, void* userData);

    explicit VectorPlot(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCallback(nullptr),
          fContextMenuCallback(nullptr),
          fContextMenuUserData(nullptr),
          fCustomResponseFunc(nullptr),
          fCustomResponseUserData(nullptr),
          fSampleRate(44100.0),
          fMinFreq(20.0f),
          fMaxFreq(20000.0f),
          fMinDb(-24.0f),
          fMaxDb(24.0f),
          fSelectedBand(-1),
          fHoverBand(-1),
          fDragging(false),
          fDragBand(-1),
          fDragStartX(0.0f),
          fDragStartY(0.0f),
          fShowGrid(true),
          fShowLegend(true),
          fCornerRadius(4.0f),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(38, 42, 50)),
          fAxisColor(Color(65, 72, 85)),
          fBorderColor(Color(55, 60, 70)),
          fCurveColor(Color(52, 152, 219)),
          fCurveFillTop(Color(52, 152, 219, 130)),
          fCurveFillBottom(Color(41, 128, 185, 20)),
          fTextColor(Color(140, 150, 165))
    {
        initAccessible();
    }

    explicit VectorPlot(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCallback(nullptr),
          fContextMenuCallback(nullptr),
          fContextMenuUserData(nullptr),
          fCustomResponseFunc(nullptr),
          fCustomResponseUserData(nullptr),
          fSampleRate(44100.0),
          fMinFreq(20.0f),
          fMaxFreq(20000.0f),
          fMinDb(-24.0f),
          fMaxDb(24.0f),
          fSelectedBand(-1),
          fHoverBand(-1),
          fDragging(false),
          fDragBand(-1),
          fDragStartX(0.0f),
          fDragStartY(0.0f),
          fShowGrid(true),
          fShowLegend(true),
          fCornerRadius(4.0f),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(38, 42, 50)),
          fAxisColor(Color(65, 72, 85)),
          fBorderColor(Color(55, 60, 70)),
          fCurveColor(Color(52, 152, 219)),
          fCurveFillTop(Color(52, 152, 219, 130)),
          fCurveFillBottom(Color(41, 128, 185, 20)),
          fTextColor(Color(140, 150, 165))
    {
        initAccessible();
    }

    explicit VectorPlot(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCallback(nullptr),
          fContextMenuCallback(nullptr),
          fContextMenuUserData(nullptr),
          fCustomResponseFunc(nullptr),
          fCustomResponseUserData(nullptr),
          fSampleRate(44100.0),
          fMinFreq(20.0f),
          fMaxFreq(20000.0f),
          fMinDb(-24.0f),
          fMaxDb(24.0f),
          fSelectedBand(-1),
          fHoverBand(-1),
          fDragging(false),
          fDragBand(-1),
          fDragStartX(0.0f),
          fDragStartY(0.0f),
          fShowGrid(true),
          fShowLegend(true),
          fCornerRadius(4.0f),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(38, 42, 50)),
          fAxisColor(Color(65, 72, 85)),
          fBorderColor(Color(55, 60, 70)),
          fCurveColor(Color(52, 152, 219)),
          fCurveFillTop(Color(52, 152, 219, 130)),
          fCurveFillBottom(Color(41, 128, 185, 20)),
          fTextColor(Color(140, 150, 165))
    {
        initAccessible();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Band Management

    int addBand(const FilterType type, const float freq, const float gainDb, const float q, const Color& color)
    {
        fBands.push_back(FilterBand(type, freq, gainDb, q, color));
        repaint();
        return static_cast<int>(fBands.size()) - 1;
    }

    int addBand(const FilterBand& band)
    {
        fBands.push_back(band);
        repaint();
        return static_cast<int>(fBands.size()) - 1;
    }

    void clearBands()
    {
        fBands.clear();
        fSelectedBand = -1;
        fHoverBand = -1;
        repaint();
    }

    int getBandCount() const noexcept { return static_cast<int>(fBands.size()); }

    const FilterBand* getBand(const int index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= fBands.size())
            return nullptr;
        return &fBands[static_cast<size_t>(index)];
    }

    FilterBand* getBand(const int index)
    {
        if (index < 0 || static_cast<size_t>(index) >= fBands.size())
            return nullptr;
        return &fBands[static_cast<size_t>(index)];
    }

    void setBand(const int index, const float freq, const float gainDb, const float q)
    {
        if (index >= 0 && static_cast<size_t>(index) < fBands.size())
        {
            fBands[static_cast<size_t>(index)].freq = clampVal(freq, fMinFreq, fMaxFreq);
            fBands[static_cast<size_t>(index)].gainDb = clampVal(gainDb, fMinDb, fMaxDb);
            fBands[static_cast<size_t>(index)].q = clampVal(q, 0.1f, 10.0f);
            repaint();
        }
    }

    void setBandEnabled(const int index, const bool enabled)
    {
        if (index >= 0 && static_cast<size_t>(index) < fBands.size())
        {
            fBands[static_cast<size_t>(index)].enabled = enabled;
            repaint();
        }
    }

    int getSelectedBand() const noexcept { return fSelectedBand; }
    void setSelectedBand(const int index)
    {
        fSelectedBand = index;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration & Custom Response

    void setSampleRate(const double sampleRate) noexcept
    {
        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        repaint();
    }

    double getSampleRate() const noexcept { return fSampleRate; }

    void setFreqRange(const float minFreq, const float maxFreq)
    {
        fMinFreq = std::max(1.0f, minFreq);
        fMaxFreq = std::max(fMinFreq + 10.0f, maxFreq);
        repaint();
    }

    void setDbRange(const float minDb, const float maxDb)
    {
        fMinDb = minDb;
        fMaxDb = std::max(minDb + 1.0f, maxDb);
        repaint();
    }

    void setCustomResponseFunc(ResponseFunc const func, void* const userData = nullptr) noexcept
    {
        fCustomResponseFunc = func;
        fCustomResponseUserData = userData;
        repaint();
    }

    void setExternalCurve(const std::vector<float>& dbValues)
    {
        fExternalCurve = dbValues;
        repaint();
    }

    void clearExternalCurve()
    {
        fExternalCurve.clear();
        repaint();
    }

    void setShowGrid(const bool show) noexcept { fShowGrid = show; repaint(); }
    void setShowLegend(const bool show) noexcept { fShowLegend = show; repaint(); }
    void setCallback(Callback* const cb) noexcept { fCallback = cb; }
    void setContextMenuCallback(ContextMenuCallback const cb, void* const userData = nullptr) noexcept
    {
        fContextMenuCallback = cb;
        fContextMenuUserData = userData;
    }

    // Colors
    void setCurveColor(const Color& c) noexcept { fCurveColor = c; repaint(); }
    void setBgColor(const Color& c) noexcept { fBgColor = c; repaint(); }

    // ----------------------------------------------------------------------------------------------------------------
    // Coordinate Conversions

    float freqToX(const float freq) const
    {
        const float w = static_cast<float>(getWidth());
        const float clampedF = clampVal(freq, fMinFreq, fMaxFreq);
        const float norm = std::log10(clampedF / fMinFreq) / std::log10(fMaxFreq / fMinFreq);
        return norm * w;
    }

    float xToFreq(const float x) const
    {
        const float w = static_cast<float>(getWidth());
        if (w <= 0.0f) return fMinFreq;
        const float norm = clampVal(x / w, 0.0f, 1.0f);
        return fMinFreq * std::pow(10.0f, norm * std::log10(fMaxFreq / fMinFreq));
    }

    float dbToY(const float db) const
    {
        const float h = static_cast<float>(getHeight());
        const float clampedDb = clampVal(db, fMinDb, fMaxDb);
        const float norm = (clampedDb - fMinDb) / (fMaxDb - fMinDb);
        return h * (1.0f - norm);
    }

    float yToDb(const float y) const
    {
        const float h = static_cast<float>(getHeight());
        if (h <= 0.0f) return 0.0f;
        const float norm = 1.0f - clampVal(y / h, 0.0f, 1.0f);
        return fMinDb + norm * (fMaxDb - fMinDb);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Response Evaluation

    double evaluateCompositeDb(const double freqHz) const
    {
        if (fCustomResponseFunc != nullptr)
            return fCustomResponseFunc(freqHz, fCustomResponseUserData);

        double totalDb = 0.0;
        for (size_t i = 0; i < fBands.size(); ++i)
        {
            if (fBands[i].enabled)
                totalDb += fBands[i].evaluateMagnitudeDb(freqHz, fSampleRate);
        }
        return totalDb;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Mouse Event Handling

    bool onMouse(const MouseEvent& ev) override
    {
        const float mx = static_cast<float>(ev.pos.getX());
        const float my = static_cast<float>(ev.pos.getY());

        // Context menu (Right click)
        if (ev.press && ev.button == 3)
        {
            if (fContextMenuCallback != nullptr)
                fContextMenuCallback(this, static_cast<int32_t>(mx), static_cast<int32_t>(my), fContextMenuUserData);
            if (fCallback != nullptr)
                fCallback->vectorPlotContextMenu(this, static_cast<int32_t>(mx), static_cast<int32_t>(my));
            return true;
        }

        if (ev.press && ev.button == 1)
        {
            // Hit-test band nodes
            const float handleRadius = 12.0f;
            int hitBand = -1;

            for (int i = static_cast<int>(fBands.size()) - 1; i >= 0; --i)
            {
                const float bx = freqToX(fBands[static_cast<size_t>(i)].freq);
                const float by = dbToY(fBands[static_cast<size_t>(i)].gainDb);
                const float dx = mx - bx;
                const float dy = my - by;

                if (dx * dx + dy * dy <= handleRadius * handleRadius)
                {
                    hitBand = i;
                    break;
                }
            }

            if (hitBand >= 0)
            {
                fSelectedBand = hitBand;
                fDragBand = hitBand;
                fDragging = true;
                fDragStartX = mx;
                fDragStartY = my;

                if (fCallback != nullptr)
                    fCallback->vectorPlotBandSelected(this, hitBand);

                updateAccessible();
                repaint();
                return true;
            }
        }
        else if (!ev.press && ev.button == 1)
        {
            if (fDragging)
            {
                fDragging = false;
                fDragBand = -1;
                repaint();
                return true;
            }
        }

        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        const float mx = static_cast<float>(ev.pos.getX());
        const float my = static_cast<float>(ev.pos.getY());

        if (fDragging && fDragBand >= 0 && static_cast<size_t>(fDragBand) < fBands.size())
        {
            FilterBand& band = fBands[static_cast<size_t>(fDragBand)];
            band.freq = clampVal(xToFreq(mx), fMinFreq, fMaxFreq);

            // Highpass/Lowpass usually stay pinned at 0dB or cut gain
            if (band.type != kFilterLowpass && band.type != kFilterHighpass)
                band.gainDb = clampVal(yToDb(my), fMinDb, fMaxDb);

            if (fCallback != nullptr)
                fCallback->vectorPlotBandChanged(this, fDragBand, band.freq, band.gainDb, band.q);

            updateAccessible();
            repaint();
            return true;
        }

        // Hover test
        const float handleRadius = 12.0f;
        int hover = -1;
        for (int i = static_cast<int>(fBands.size()) - 1; i >= 0; --i)
        {
            const float bx = freqToX(fBands[static_cast<size_t>(i)].freq);
            const float by = dbToY(fBands[static_cast<size_t>(i)].gainDb);
            const float dx = mx - bx;
            const float dy = my - by;

            if (dx * dx + dy * dy <= handleRadius * handleRadius)
            {
                hover = i;
                break;
            }
        }

        if (hover != fHoverBand)
        {
            fHoverBand = hover;
            repaint();
        }

        return false;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const int targetBand = (fSelectedBand >= 0) ? fSelectedBand : fHoverBand;
        if (targetBand >= 0 && static_cast<size_t>(targetBand) < fBands.size())
        {
            FilterBand& band = fBands[static_cast<size_t>(targetBand)];
            const float factor = ev.delta.getY() > 0 ? 1.15f : 0.85f;
            band.q = clampVal(band.q * factor, 0.1f, 10.0f);

            if (fCallback != nullptr)
                fCallback->vectorPlotBandChanged(this, targetBand, band.freq, band.gainDb, band.q);

            updateAccessible();
            repaint();
            return true;
        }

        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // NanoVG Display

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 2.0f || h <= 2.0f)
            return;

        // 1. Background Frame
        beginPath();
        roundedRect(0.0f, 0.0f, w, h, fCornerRadius);
        fillColor(fBgColor);
        fill();
        strokeColor(fBorderColor);
        strokeWidth(1.0f);
        stroke();

        save();
        scissor(0.0f, 0.0f, w, h);

        // 2. Frequency & Decibel Grid Lines
        if (fShowGrid)
            drawGrid(w, h);

        // 3. Composite Response Curve Fill & Line
        drawCurve(w, h);

        // 4. Filter Band Pucks & Tooltips
        drawBands(w, h);

        restore();
    }

protected:
    void initAccessible()
    {
        fAccessibleInfo.setRole(kAccessibleRoleIndicator);
        fAccessibleInfo.setLabel("Filter Bode Plot Visualizer");
        fAccessibleInfo.setDescription("Interactive parametric filter curve and transfer function display");
        fAccessibleInfo.minValue = -24.0f;
        fAccessibleInfo.maxValue = 24.0f;
        fAccessibleInfo.step = 0.5f;
    }

    void updateAccessible()
    {
        if (fSelectedBand >= 0 && static_cast<size_t>(fSelectedBand) < fBands.size())
        {
            const FilterBand& b = fBands[static_cast<size_t>(fSelectedBand)];
            char buf[128];
            std::snprintf(buf, sizeof(buf), "Band %d: %.0f Hz, %+.1f dB, Q: %.2f",
                          fSelectedBand + 1, b.freq, b.gainDb, b.q);
            fAccessibleInfo.setValue(b.gainDb, buf);
        }
    }

private:
    Callback* fCallback;
    ContextMenuCallback fContextMenuCallback;
    void* fContextMenuUserData;
    ResponseFunc fCustomResponseFunc;
    void* fCustomResponseUserData;

    double fSampleRate;
    float fMinFreq;
    float fMaxFreq;
    float fMinDb;
    float fMaxDb;

    std::vector<FilterBand> fBands;
    std::vector<float> fExternalCurve;

    int fSelectedBand;
    int fHoverBand;
    bool fDragging;
    int fDragBand;
    float fDragStartX;
    float fDragStartY;

    bool fShowGrid;
    bool fShowLegend;
    float fCornerRadius;

    // Theme colors
    Color fBgColor;
    Color fGridColor;
    Color fAxisColor;
    Color fBorderColor;
    Color fCurveColor;
    Color fCurveFillTop;
    Color fCurveFillBottom;
    Color fTextColor;

    template<typename T>
    static inline T clampVal(const T val, const T minVal, const T maxVal)
    {
        return (val < minVal) ? minVal : ((val > maxVal) ? maxVal : val);
    }

    void drawGrid(const float w, const float h)
    {
        // 1. Decibel horizontal grid lines
        const float dbs[] = { 18.0f, 12.0f, 6.0f, 0.0f, -6.0f, -12.0f, -18.0f };
        for (size_t i = 0; i < 7; ++i)
        {
            const float gy = dbToY(dbs[i]);
            beginPath();
            moveTo(0.0f, gy);
            lineTo(w, gy);

            if (dbs[i] == 0.0f)
            {
                strokeColor(fAxisColor);
                strokeWidth(1.25f);
            }
            else
            {
                strokeColor(fGridColor);
                strokeWidth(1.0f);
            }
            stroke();

            // dB labels on right edge
            if (fShowLegend && w > 120.0f)
            {
                char label[16];
                std::snprintf(label, sizeof(label), "%+.0fdB", dbs[i]);
                fontSize(9.0f);
                fillColor(fTextColor);
                textAlign(ALIGN_RIGHT | ALIGN_MIDDLE);
                text(w - 4.0f, gy - 1.0f, label, nullptr);
            }
        }

        // 2. Frequency logarithmic vertical grid lines
        const float freqs[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
        const char* const freqLabels[] = { "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

        for (size_t i = 0; i < 9; ++i)
        {
            if (freqs[i] >= fMinFreq && freqs[i] <= fMaxFreq)
            {
                const float gx = freqToX(freqs[i]);
                beginPath();
                moveTo(gx, 0.0f);
                lineTo(gx, h);
                strokeColor(fGridColor);
                strokeWidth(1.0f);
                stroke();

                // Frequency label at bottom
                if (fShowLegend && h > 60.0f)
                {
                    fontSize(9.0f);
                    fillColor(fTextColor);
                    textAlign(ALIGN_CENTER | ALIGN_BOTTOM);
                    text(gx, h - 3.0f, freqLabels[i], nullptr);
                }
            }
        }
    }

    void drawCurve(const float w, const float h)
    {
        const int numPoints = std::max(64, static_cast<int>(w / 2.0f));
        const float zeroY = dbToY(0.0f);

        // Pre-compute curve points
        std::vector<Point<float>> points;
        points.reserve(static_cast<size_t>(numPoints));

        for (int i = 0; i < numPoints; ++i)
        {
            const float px = (static_cast<float>(i) / static_cast<float>(numPoints - 1)) * w;
            const float freq = xToFreq(px);
            const double db = evaluateCompositeDb(freq);
            const float py = dbToY(static_cast<float>(db));
            points.push_back(Point<float>(px, py));
        }

        if (points.empty())
            return;

        // 1. Shaded Area Fill
        beginPath();
        moveTo(points[0].getX(), zeroY);
        for (size_t i = 0; i < points.size(); ++i)
            lineTo(points[i].getX(), points[i].getY());
        lineTo(points.back().getX(), zeroY);
        closePath();

        Paint fillGrad = linearGradient(0.0f, 0.0f, 0.0f, h, fCurveFillTop, fCurveFillBottom);
        fillPaint(fillGrad);
        fill();

        // 2. Main Response Line
        beginPath();
        moveTo(points[0].getX(), points[0].getY());
        for (size_t i = 1; i < points.size(); ++i)
            lineTo(points[i].getX(), points[i].getY());

        strokeColor(fCurveColor);
        strokeWidth(2.0f);
        stroke();
    }

    void drawBands(const float w, const float h)
    {
        (void)w;
        (void)h;

        for (int i = 0; i < static_cast<int>(fBands.size()); ++i)
        {
            const FilterBand& band = fBands[static_cast<size_t>(i)];
            const float bx = freqToX(band.freq);
            const float by = dbToY(band.gainDb);
            const bool isSelected = (i == fSelectedBand);
            const bool isHover = (i == fHoverBand);
            const float puckRadius = isSelected ? 8.0f : (isHover ? 7.0f : 6.0f);

            // Halo glow when selected/hover
            if (isSelected || isHover)
            {
                beginPath();
                circle(bx, by, puckRadius + 4.0f);
                fillColor(Color(band.color.red, band.color.green, band.color.blue, 0.25f));
                fill();
            }

            // Puck body
            beginPath();
            circle(bx, by, puckRadius);
            fillColor(band.enabled ? band.color : Color(80, 85, 95));
            fill();
            strokeColor(Color(255, 255, 255, 220));
            strokeWidth(1.5f);
            stroke();

            // Center band index number
            char numBuf[8];
            std::snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
            fontSize(9.0f);
            fillColor(Color(20, 22, 26));
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            text(bx, by, numBuf, nullptr);

            // Floating Tooltip when hovered or dragging
            if (isSelected || isHover)
            {
                char tip[64];
                if (band.freq >= 1000.0f)
                    std::snprintf(tip, sizeof(tip), "%.1fkHz %+.1fdB Q:%.1f", band.freq / 1000.0f, band.gainDb, band.q);
                else
                    std::snprintf(tip, sizeof(tip), "%.0fHz %+.1fdB Q:%.1f", band.freq, band.gainDb, band.q);

                const float tipY = by > 30.0f ? (by - 16.0f) : (by + 20.0f);
                fontSize(9.5f);
                fillColor(Color(240, 245, 255));
                textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                text(bx, tipY, tip, nullptr);
            }
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_PLOT_HPP_INCLUDED
