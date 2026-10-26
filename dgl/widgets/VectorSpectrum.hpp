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

#ifndef DGL_VECTOR_SPECTRUM_HPP_INCLUDED
#define DGL_VECTOR_SPECTRUM_HPP_INCLUDED

#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/DataRingBuffer.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent NanoVG real-time FFT / RTA frequency spectrum visualizer.
   Accepts frequency magnitude bins directly from DSP or lock-free DataRingBuffer telemetry.
   Features logarithmic frequency axis (20 Hz - 20 kHz), decibel scaling, smooth falloff ballistics,
   peak-hold contour, ISO frequency reticle, and gradient fill rendering.
 */
class VectorSpectrum : public NanoSubWidget,
                       public AccessibleElement
{
public:
    explicit VectorSpectrum(Widget* const parentWidget, const uint32_t binCount = 256)
        : NanoSubWidget(parentWidget),
          fMinFreq(20.0f),
          fMaxFreq(20000.0f),
          fMinDb(-90.0f),
          fMaxDb(0.0f),
          fDecayRate(35.0f), // dB per second
          fPeakHoldDecay(12.0f),
          fTraceWidth(2.0f),
          fShowGrid(true),
          fShowPeakHold(true),
          fFillCurve(true),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(45, 48, 55)),
          fBorderColor(Color(65, 68, 75)),
          fTraceColor(Color(52, 199, 89)),
          fPeakColor(Color(100, 230, 130, 180)),
          fFillColorTop(Color(52, 199, 89, 70)),
          fFillColorBottom(Color(52, 199, 89, 5)),
          fCornerRadius(4.0f)
    {
        setBinCount(binCount);
    }

    explicit VectorSpectrum(NanoSubWidget* const parentWidget, const uint32_t binCount = 256)
        : NanoSubWidget(parentWidget),
          fMinFreq(20.0f),
          fMaxFreq(20000.0f),
          fMinDb(-90.0f),
          fMaxDb(0.0f),
          fDecayRate(35.0f),
          fPeakHoldDecay(12.0f),
          fTraceWidth(2.0f),
          fShowGrid(true),
          fShowPeakHold(true),
          fFillCurve(true),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(45, 48, 55)),
          fBorderColor(Color(65, 68, 75)),
          fTraceColor(Color(52, 199, 89)),
          fPeakColor(Color(100, 230, 130, 180)),
          fFillColorTop(Color(52, 199, 89, 70)),
          fFillColorBottom(Color(52, 199, 89, 5)),
          fCornerRadius(4.0f)
    {
        setBinCount(binCount);
    }

    explicit VectorSpectrum(NanoTopLevelWidget* const parentWidget, const uint32_t binCount = 256)
        : NanoSubWidget(parentWidget),
          fMinFreq(20.0f),
          fMaxFreq(20000.0f),
          fMinDb(-90.0f),
          fMaxDb(0.0f),
          fDecayRate(35.0f),
          fPeakHoldDecay(12.0f),
          fTraceWidth(2.0f),
          fShowGrid(true),
          fShowPeakHold(true),
          fFillCurve(true),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(45, 48, 55)),
          fBorderColor(Color(65, 68, 75)),
          fTraceColor(Color(52, 199, 89)),
          fPeakColor(Color(100, 230, 130, 180)),
          fFillColorTop(Color(52, 199, 89, 70)),
          fFillColorBottom(Color(52, 199, 89, 5)),
          fCornerRadius(4.0f)
    {
        setBinCount(binCount);
    }

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration

    void setBinCount(const uint32_t count)
    {
        const uint32_t valid = count > 16 ? count : 16;
        fCurrentDb.assign(valid, fMinDb);
        fPeakDb.assign(valid, fMinDb);
        repaint();
    }

    uint32_t getBinCount() const noexcept
    {
        return static_cast<uint32_t>(fCurrentDb.size());
    }

    void setDecibelRange(const float minDb, const float maxDb)
    {
        if (minDb < maxDb)
        {
            fMinDb = minDb;
            fMaxDb = maxDb;
            repaint();
        }
    }

    float getMinDecibels() const noexcept { return fMinDb; }
    float getMaxDecibels() const noexcept { return fMaxDb; }

    float getCurrentDb(const uint32_t bin) const noexcept
    {
        return bin < fCurrentDb.size() ? fCurrentDb[bin] : fMinDb;
    }

    float getPeakDb(const uint32_t bin) const noexcept
    {
        return bin < fPeakDb.size() ? fPeakDb[bin] : fMinDb;
    }

    void setDecayRate(const float decayRate) noexcept
    {
        fDecayRate = decayRate > 1.0f ? decayRate : 1.0f;
    }

    void setTraceColor(const Color& color) noexcept
    {
        fTraceColor = color;
        fFillColorTop = Color(color.red, color.green, color.blue, 70);
        fFillColorBottom = Color(color.red, color.green, color.blue, 5);
        repaint();
    }

    void setBackgroundColor(const Color& color) noexcept
    {
        fBgColor = color;
        repaint();
    }

    void setShowGrid(const bool show) noexcept
    {
        fShowGrid = show;
        repaint();
    }

    void setShowPeakHold(const bool show) noexcept
    {
        fShowPeakHold = show;
        repaint();
    }

    void setFillCurve(const bool fill) noexcept
    {
        fFillCurve = fill;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Magnitude Ingestion

    /**
       Feed linear or dB magnitude spectrum directly.
       @param magnitudes Array of frequency bin values.
       @param count Number of bins.
       @param isDb True if input is already in dB, false if linear (0.0 to 1.0).
     */
    void setMagnitudes(const float* const magnitudes, const uint32_t count, const bool isDb = false)
    {
        if (magnitudes == nullptr || count == 0)
            return;

        if (count != fCurrentDb.size())
            setBinCount(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            float db = isDb ? magnitudes[i] : (magnitudes[i] > 1e-5f ? 20.0f * std::log10(magnitudes[i]) : fMinDb);
            db = std::max(fMinDb, std::min(fMaxDb, db));

            if (db > fCurrentDb[i])
            {
                fCurrentDb[i] = db;
            }
            if (db > fPeakDb[i])
            {
                fPeakDb[i] = db;
            }
        }

        repaint();
    }

    /**
       Feed telemetry payload directly from UI::uiTelemetryReceived().
     */
    bool feedTelemetry(const uint32_t tag, const uint32_t targetTag, const void* const data, const uint32_t size, const bool isDb = false)
    {
        if (tag != targetTag || data == nullptr || size < sizeof(float))
            return false;

        const uint32_t count = size / sizeof(float);
        setMagnitudes(static_cast<const float*>(data), count, isDb);
        return true;
    }

    /**
       Update spectrum falloff ballistics.
       @param deltaSeconds Time elapsed since last update in seconds.
     */
    void updateBallistics(const float deltaSeconds)
    {
        if (deltaSeconds <= 0.0f)
            return;

        const float decay = fDecayRate * deltaSeconds;
        const float peakDecay = fPeakHoldDecay * deltaSeconds;
        bool changed = false;

        for (size_t i = 0; i < fCurrentDb.size(); ++i)
        {
            if (fCurrentDb[i] > fMinDb)
            {
                fCurrentDb[i] = std::max(fMinDb, fCurrentDb[i] - decay);
                changed = true;
            }
            if (fPeakDb[i] > fMinDb)
            {
                fPeakDb[i] = std::max(fMinDb, fPeakDb[i] - peakDecay);
                changed = true;
            }
        }

        if (changed)
            repaint();
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

        // 2. Frequency & Decibel Reticle
        if (fShowGrid)
        {
            strokeColor(fGridColor);
            strokeWidth(0.8f);

            // Vertical ISO frequency lines: 50Hz, 100Hz, 500Hz, 1kHz, 5kHz, 10kHz
            static const float sGridFreqs[] = { 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2500.0f, 5000.0f, 10000.0f };
            const float logMin = std::log10(fMinFreq);
            const float logRange = std::log10(fMaxFreq) - logMin;

            for (size_t i = 0; i < sizeof(sGridFreqs) / sizeof(sGridFreqs[0]); ++i)
            {
                const float freq = sGridFreqs[i];
                if (freq >= fMinFreq && freq <= fMaxFreq && logRange > 0.0f)
                {
                    const float x = ((std::log10(freq) - logMin) / logRange) * (w - 2.0f) + 1.0f;
                    beginPath();
                    moveTo(x, 1.0f);
                    lineTo(x, h - 1.0f);
                    stroke();
                }
            }

            // Horizontal dB division lines: -12, -24, -36, -48, -60, -72
            static const float sGridDbs[] = { -12.0f, -24.0f, -36.0f, -48.0f, -60.0f, -72.0f };
            const float dbRange = fMaxDb - fMinDb;

            for (size_t j = 0; j < sizeof(sGridDbs) / sizeof(sGridDbs[0]); ++j)
            {
                const float db = sGridDbs[j];
                if (db > fMinDb && db < fMaxDb && dbRange > 0.0f)
                {
                    const float y = (1.0f - (db - fMinDb) / dbRange) * (h - 2.0f) + 1.0f;
                    beginPath();
                    moveTo(1.0f, y);
                    lineTo(w - 1.0f, y);
                    stroke();
                }
            }
        }

        const uint32_t binCount = static_cast<uint32_t>(fCurrentDb.size());
        if (binCount < 2)
            return;

        const float dbRange = fMaxDb - fMinDb;
        if (dbRange <= 0.0f)
            return;

        // 3. Optional Fill Under Spectrum Curve
        if (fFillCurve)
        {
            beginPath();
            moveTo(1.0f, h - 1.0f);

            for (uint32_t i = 0; i < binCount; ++i)
            {
                const float normX = static_cast<float>(i) / static_cast<float>(binCount - 1);
                const float x = 1.0f + normX * (w - 2.0f);
                const float normY = (fCurrentDb[i] - fMinDb) / dbRange;
                const float y = (1.0f - normY) * (h - 2.0f) + 1.0f;
                lineTo(x, y);
            }

            lineTo(w - 1.0f, h - 1.0f);
            closePath();

            // Vertical linear gradient fill
            fillColor(fFillColorTop);
            fill();
        }

        // 4. Primary Spectrum Curve Stroke
        beginPath();
        for (uint32_t i = 0; i < binCount; ++i)
        {
            const float normX = static_cast<float>(i) / static_cast<float>(binCount - 1);
            const float x = 1.0f + normX * (w - 2.0f);
            const float normY = (fCurrentDb[i] - fMinDb) / dbRange;
            const float y = (1.0f - normY) * (h - 2.0f) + 1.0f;

            if (i == 0)
                moveTo(x, y);
            else
                lineTo(x, y);
        }
        strokeColor(fTraceColor);
        strokeWidth(fTraceWidth);
        stroke();

        // 5. Peak Hold Contour
        if (fShowPeakHold)
        {
            beginPath();
            for (uint32_t i = 0; i < binCount; ++i)
            {
                const float normX = static_cast<float>(i) / static_cast<float>(binCount - 1);
                const float x = 1.0f + normX * (w - 2.0f);
                const float normY = (fPeakDb[i] - fMinDb) / dbRange;
                const float y = (1.0f - normY) * (h - 2.0f) + 1.0f;

                if (i == 0)
                    moveTo(x, y);
                else
                    lineTo(x, y);
            }
            strokeColor(fPeakColor);
            strokeWidth(1.0f);
            stroke();
        }

        // 6. Border Frame
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        strokeColor(fBorderColor);
        strokeWidth(1.2f);
        stroke();
    }

private:
    float fMinFreq;
    float fMaxFreq;
    float fMinDb;
    float fMaxDb;
    float fDecayRate;
    float fPeakHoldDecay;
    float fTraceWidth;
    bool fShowGrid;
    bool fShowPeakHold;
    bool fFillCurve;

    Color fBgColor;
    Color fGridColor;
    Color fBorderColor;
    Color fTraceColor;
    Color fPeakColor;
    Color fFillColorTop;
    Color fFillColorBottom;
    float fCornerRadius;

    std::vector<float> fCurrentDb;
    std::vector<float> fPeakDb;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorSpectrum)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_SPECTRUM_HPP_INCLUDED
