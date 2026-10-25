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

#ifndef DGL_VECTOR_METER_HPP_INCLUDED
#define DGL_VECTOR_METER_HPP_INCLUDED

#include "../Color.hpp"
#include "../NanoVG.hpp"

#include <cmath>
#include <algorithm>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Resolution-independent, customizable NanoVG vector audio level / peak meter.
 */
class VectorMeter : public NanoSubWidget
{
public:
    enum Orientation {
        Vertical,
        Horizontal
    };

    explicit VectorMeter(Widget* const parentWidget,
                         const Orientation orientation = Vertical,
                         const uint32_t channelCount = 2)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fChannelCount(std::max(1u, std::min(2u, channelCount))),
          fMinDb(-60.0f),
          fMaxDb(6.0f),
          fFalloffRate(24.0f), // dB per second
          fPeakHoldTime(1.5f), // seconds
          fTrackColor(Color(24, 25, 28)),
          fNormalColor(Color(46, 204, 113)),
          fWarnColor(Color(241, 196, 15)),
          fClipColor(Color(231, 76, 60)),
          fPeakColor(Color(255, 255, 255))
    {
        for (int i = 0; i < 2; ++i)
        {
            fLevelDb[i] = fMinDb;
            fPeakDb[i] = fMinDb;
            fPeakHoldTimer[i] = 0.0f;
        }
    }

    explicit VectorMeter(NanoSubWidget* const parentWidget,
                         const Orientation orientation = Vertical,
                         const uint32_t channelCount = 2)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fChannelCount(std::max(1u, std::min(2u, channelCount))),
          fMinDb(-60.0f),
          fMaxDb(6.0f),
          fFalloffRate(24.0f),
          fPeakHoldTime(1.5f),
          fTrackColor(Color(24, 25, 28)),
          fNormalColor(Color(46, 204, 113)),
          fWarnColor(Color(241, 196, 15)),
          fClipColor(Color(231, 76, 60)),
          fPeakColor(Color(255, 255, 255))
    {
        for (int i = 0; i < 2; ++i)
        {
            fLevelDb[i] = fMinDb;
            fPeakDb[i] = fMinDb;
            fPeakHoldTimer[i] = 0.0f;
        }
    }

    explicit VectorMeter(NanoTopLevelWidget* const parentWidget,
                         const Orientation orientation = Vertical,
                         const uint32_t channelCount = 2)
        : NanoSubWidget(parentWidget),
          fOrientation(orientation),
          fChannelCount(std::max(1u, std::min(2u, channelCount))),
          fMinDb(-60.0f),
          fMaxDb(6.0f),
          fFalloffRate(24.0f),
          fPeakHoldTime(1.5f),
          fTrackColor(Color(24, 25, 28)),
          fNormalColor(Color(46, 204, 113)),
          fWarnColor(Color(241, 196, 15)),
          fClipColor(Color(231, 76, 60)),
          fPeakColor(Color(255, 255, 255))
    {
        for (int i = 0; i < 2; ++i)
        {
            fLevelDb[i] = fMinDb;
            fPeakDb[i] = fMinDb;
            fPeakHoldTimer[i] = 0.0f;
        }
    }

    ~VectorMeter() override {}

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration

    void setOrientation(const Orientation orientation) noexcept
    {
        if (fOrientation == orientation) return;
        fOrientation = orientation;
        repaint();
    }

    void setChannelCount(const uint32_t count) noexcept
    {
        fChannelCount = std::max(1u, std::min(2u, count));
        repaint();
    }

    void setDecibelRange(const float minDb, const float maxDb) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(maxDb > minDb,);
        fMinDb = minDb;
        fMaxDb = maxDb;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Values & Ballistics

    void setValueDb(const float db, const uint32_t channel = 0) noexcept
    {
        if (channel >= fChannelCount) return;

        const float clamped = std::max(fMinDb, std::min(fMaxDb, db));
        fLevelDb[channel] = clamped;

        if (clamped >= fPeakDb[channel])
        {
            fPeakDb[channel] = clamped;
            fPeakHoldTimer[channel] = fPeakHoldTime;
        }

        repaint();
    }

    void setValuesDb(const float leftDb, const float rightDb) noexcept
    {
        setValueDb(leftDb, 0);
        if (fChannelCount > 1)
            setValueDb(rightDb, 1);
    }

    void setValueLinear(const float linear, const uint32_t channel = 0) noexcept
    {
        float db = fMinDb;
        if (linear > 0.00001f)
            db = 20.0f * std::log10(linear);
        setValueDb(db, channel);
    }

    void setValuesLinear(const float leftLin, const float rightLin) noexcept
    {
        setValueLinear(leftLin, 0);
        if (fChannelCount > 1)
            setValueLinear(rightLin, 1);
    }

    void updateBallistics(const float deltaTimeSeconds) noexcept
    {
        if (deltaTimeSeconds <= 0.0f)
            return;

        bool changed = false;
        const float decay = fFalloffRate * deltaTimeSeconds;

        for (uint32_t ch = 0; ch < fChannelCount; ++ch)
        {
            // Level decay
            if (fLevelDb[ch] > fMinDb)
            {
                fLevelDb[ch] = std::max(fMinDb, fLevelDb[ch] - decay);
                changed = true;
            }

            // Peak hold timer and decay
            if (fPeakHoldTimer[ch] > 0.0f)
            {
                fPeakHoldTimer[ch] -= deltaTimeSeconds;
            }
            else if (fPeakDb[ch] > fMinDb)
            {
                fPeakDb[ch] = std::max(fMinDb, fPeakDb[ch] - (decay * 1.5f));
                changed = true;
            }
        }

        if (changed)
            repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Colors

    void setTrackColor(const Color& color) noexcept { fTrackColor = color; repaint(); }
    void setNormalColor(const Color& color) noexcept { fNormalColor = color; repaint(); }
    void setWarnColor(const Color& color) noexcept { fWarnColor = color; repaint(); }
    void setClipColor(const Color& color) noexcept { fClipColor = color; repaint(); }
    void setPeakColor(const Color& color) noexcept { fPeakColor = color; repaint(); }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // NanoDisplay

    void onNanoDisplay() override
    {
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());
        if (w <= 1.0f || h <= 1.0f)
            return;

        const float gap = 2.0f;

        if (fOrientation == Vertical)
        {
            const float barWidth = (w - (fChannelCount - 1) * gap) / static_cast<float>(fChannelCount);

            for (uint32_t ch = 0; ch < fChannelCount; ++ch)
            {
                const float x = ch * (barWidth + gap);

                // 1. Background Groove
                beginPath();
                roundedRect(x, 0.0f, barWidth, h, 2.0f);
                fillColor(fTrackColor);
                fill();

                // 2. Active Level Bar
                const float normLevel = getNormalizedDb(fLevelDb[ch]);
                if (normLevel > 0.001f)
                {
                    const float barH = normLevel * h;
                    const float y = h - barH;

                    beginPath();
                    roundedRect(x, y, barWidth, barH, 2.0f);

                    // Dynamic color based on level
                    if (fLevelDb[ch] > -3.0f)
                        fillColor(fClipColor);
                    else if (fLevelDb[ch] > -12.0f)
                        fillColor(fWarnColor);
                    else
                        fillColor(fNormalColor);

                    fill();
                }

                // 3. Peak Hold Indicator
                const float normPeak = getNormalizedDb(fPeakDb[ch]);
                if (normPeak > 0.01f)
                {
                    const float peakY = std::max(1.0f, h - (normPeak * h));
                    beginPath();
                    rect(x, peakY, barWidth, 1.5f);
                    fillColor(fPeakColor);
                    fill();
                }
            }
        }
        else // Horizontal
        {
            const float barHeight = (h - (fChannelCount - 1) * gap) / static_cast<float>(fChannelCount);

            for (uint32_t ch = 0; ch < fChannelCount; ++ch)
            {
                const float y = ch * (barHeight + gap);

                // 1. Background Groove
                beginPath();
                roundedRect(0.0f, y, w, barHeight, 2.0f);
                fillColor(fTrackColor);
                fill();

                // 2. Active Level Bar
                const float normLevel = getNormalizedDb(fLevelDb[ch]);
                if (normLevel > 0.001f)
                {
                    const float barW = normLevel * w;
                    beginPath();
                    roundedRect(0.0f, y, barW, barHeight, 2.0f);

                    if (fLevelDb[ch] > -3.0f)
                        fillColor(fClipColor);
                    else if (fLevelDb[ch] > -12.0f)
                        fillColor(fWarnColor);
                    else
                        fillColor(fNormalColor);

                    fill();
                }

                // 3. Peak Hold Indicator
                const float normPeak = getNormalizedDb(fPeakDb[ch]);
                if (normPeak > 0.01f)
                {
                    const float peakX = std::min(w - 2.0f, normPeak * w);
                    beginPath();
                    rect(peakX, y, 1.5f, barHeight);
                    fillColor(fPeakColor);
                    fill();
                }
            }
        }
    }

private:
    float getNormalizedDb(const float db) const noexcept
    {
        if (d_isEqual(fMaxDb, fMinDb))
            return 0.0f;
        const float clamped = std::max(fMinDb, std::min(fMaxDb, db));
        return (clamped - fMinDb) / (fMaxDb - fMinDb);
    }

    Orientation fOrientation;
    uint32_t fChannelCount;
    float fMinDb;
    float fMaxDb;
    float fFalloffRate;
    float fPeakHoldTime;
    float fLevelDb[2];
    float fPeakDb[2];
    float fPeakHoldTimer[2];

    Color fTrackColor;
    Color fNormalColor;
    Color fWarnColor;
    Color fClipColor;
    Color fPeakColor;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorMeter)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_METER_HPP_INCLUDED
