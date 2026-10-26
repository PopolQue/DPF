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

#ifndef DGL_VECTOR_SCOPE_HPP_INCLUDED
#define DGL_VECTOR_SCOPE_HPP_INCLUDED

#include "../Color.hpp"
#include "../NanoVG.hpp"
#include "../../distrho/extra/DataRingBuffer.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   High-performance, resolution-independent NanoVG vector oscilloscope & waveform visualizer.
   Accepts audio blocks directly or decodes lock-free DataRingBuffer telemetry streams.
   Features edge trigger modes, grid reticle, multi-channel support, and neon glow trace rendering.
 */
class VectorScope : public NanoSubWidget,
                    public AccessibleElement
{
public:
    enum TriggerMode {
        kTriggerFreeRun     = 0,
        kTriggerAuto        = 1,
        kTriggerRisingEdge  = 2,
        kTriggerFallingEdge = 3
    };

    explicit VectorScope(Widget* const parentWidget, const uint32_t bufferCapacity = 2048)
        : NanoSubWidget(parentWidget),
          fCapacity(bufferCapacity > 256 ? bufferCapacity : 256),
          fChannels(1),
          fTriggerMode(kTriggerAuto),
          fTriggerThreshold(0.0f),
          fVerticalScale(1.0f),
          fTimebase(1.0f),
          fTraceWidth(2.0f),
          fShowGrid(true),
          fGlow(true),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(45, 48, 55)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(65, 68, 75)),
          fCornerRadius(4.0f)
    {
        initBuffers();
    }

    explicit VectorScope(NanoSubWidget* const parentWidget, const uint32_t bufferCapacity = 2048)
        : NanoSubWidget(parentWidget),
          fCapacity(bufferCapacity > 256 ? bufferCapacity : 256),
          fChannels(1),
          fTriggerMode(kTriggerAuto),
          fTriggerThreshold(0.0f),
          fVerticalScale(1.0f),
          fTimebase(1.0f),
          fTraceWidth(2.0f),
          fShowGrid(true),
          fGlow(true),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(45, 48, 55)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(65, 68, 75)),
          fCornerRadius(4.0f)
    {
        initBuffers();
    }

    explicit VectorScope(NanoTopLevelWidget* const parentWidget, const uint32_t bufferCapacity = 2048)
        : NanoSubWidget(parentWidget),
          fCapacity(bufferCapacity > 256 ? bufferCapacity : 256),
          fChannels(1),
          fTriggerMode(kTriggerAuto),
          fTriggerThreshold(0.0f),
          fVerticalScale(1.0f),
          fTimebase(1.0f),
          fTraceWidth(2.0f),
          fShowGrid(true),
          fGlow(true),
          fBgColor(Color(20, 22, 26)),
          fGridColor(Color(45, 48, 55)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(65, 68, 75)),
          fCornerRadius(4.0f)
    {
        initBuffers();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration

    void setChannelCount(const uint32_t channels)
    {
        if (channels == 0 || channels == fChannels)
            return;

        fChannels = std::min(channels, 4U);
        initBuffers();
        repaint();
    }

    uint32_t getChannelCount() const noexcept
    {
        return fChannels;
    }

    AccessibleElement* getAccessibleElement() noexcept override
    {
        return this;
    }

    void setTriggerMode(const TriggerMode mode) noexcept
    {
        fTriggerMode = mode;
        repaint();
    }

    TriggerMode getTriggerMode() const noexcept
    {
        return fTriggerMode;
    }

    void setTriggerThreshold(const float threshold) noexcept
    {
        fTriggerThreshold = threshold;
        repaint();
    }

    float getTriggerThreshold() const noexcept
    {
        return fTriggerThreshold;
    }

    void setVerticalScale(const float scale) noexcept
    {
        fVerticalScale = scale > 0.01f ? scale : 0.01f;
        repaint();
    }

    float getVerticalScale() const noexcept
    {
        return fVerticalScale;
    }

    void setTimebase(const float timebase) noexcept
    {
        fTimebase = timebase > 0.05f ? timebase : 0.05f;
        repaint();
    }

    float getTimebase() const noexcept
    {
        return fTimebase;
    }

    void setShowGrid(const bool show) noexcept
    {
        fShowGrid = show;
        repaint();
    }

    void setGlow(const bool glow) noexcept
    {
        fGlow = glow;
        repaint();
    }

    void setTraceColor(const uint32_t channel, const Color& color)
    {
        if (channel < fTraceColors.size())
        {
            fTraceColors[channel] = color;
            repaint();
        }
    }

    void setBackgroundColor(const Color& color) noexcept
    {
        fBgColor = color;
        repaint();
    }

    void setGridColor(const Color& color) noexcept
    {
        fGridColor = color;
        repaint();
    }

    void setBorderColor(const Color& color) noexcept
    {
        fBorderColor = color;
        repaint();
    }

    void setTraceWidth(const float width) noexcept
    {
        fTraceWidth = width;
        repaint();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Audio Sample Ingestion

    /**
       Feed audio sample data directly from the DSP callback or UI telemetry.
     */
    void addSamples(const float* const samples, const uint32_t count, const uint32_t channel = 0)
    {
        if (samples == nullptr || count == 0 || channel >= fChannels)
            return;

        std::vector<float>& buf = fBuffers[channel];
        for (uint32_t i = 0; i < count; ++i)
        {
            buf[fWritePos[channel]] = samples[i];
            fWritePos[channel] = (fWritePos[channel] + 1) % fCapacity;
        }

        if (channel == 0)
            repaint();
    }

    /**
       Feed telemetry payload directly from UI::uiTelemetryReceived().
       Expects tag matching telemetryTag and payload formatted as float array.
     */
    bool feedTelemetry(const uint32_t tag, const uint32_t targetTag, const void* const data, const uint32_t size, const uint32_t channel = 0)
    {
        if (tag != targetTag || data == nullptr || size < sizeof(float))
            return false;

        const uint32_t floatCount = size / sizeof(float);
        addSamples(static_cast<const float*>(data), floatCount, channel);
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

        // 2. Grid Reticle
        if (fShowGrid)
        {
            const int vDivs = 8;
            const int hDivs = 6;

            strokeColor(fGridColor);
            strokeWidth(0.8f);

            // Vertical division lines
            for (int i = 1; i < vDivs; ++i)
            {
                const float x = (w / static_cast<float>(vDivs)) * static_cast<float>(i);
                beginPath();
                moveTo(x, 1.0f);
                lineTo(x, h - 1.0f);
                stroke();
            }

            // Horizontal division lines
            for (int j = 1; j < hDivs; ++j)
            {
                const float y = (h / static_cast<float>(hDivs)) * static_cast<float>(j);
                beginPath();
                moveTo(1.0f, y);
                lineTo(w - 1.0f, y);
                stroke();
            }

            // Center Horizontal Axis (Zero line)
            const float midY = h * 0.5f;
            beginPath();
            moveTo(1.0f, midY);
            lineTo(w - 1.0f, midY);
            strokeColor(fAxisColor);
            strokeWidth(1.2f);
            stroke();
        }

        // 3. Render Waveform Traces for each channel
        const float halfH = h * 0.5f;
        const uint32_t numPixels = static_cast<uint32_t>(w);

        for (uint32_t ch = 0; ch < fChannels; ++ch)
        {
            const std::vector<float>& buf = fBuffers[ch];
            if (buf.empty())
                continue;

            // Find trigger point in channel 0
            uint32_t startIdx = 0;
            if (fTriggerMode != kTriggerFreeRun && !fBuffers[0].empty())
            {
                startIdx = findTriggerIndex(fBuffers[0], fWritePos[0]);
            }
            else
            {
                // Free run: display from current write position wrapped back
                const uint32_t windowSize = static_cast<uint32_t>(static_cast<float>(numPixels) * fTimebase);
                startIdx = (fWritePos[ch] + fCapacity - (windowSize % fCapacity)) % fCapacity;
            }

            const Color& traceColor = fTraceColors[ch % fTraceColors.size()];

            // Optional Glow Pass (semi-transparent wider stroke)
            if (fGlow)
            {
                beginPath();
                renderTracePath(buf, startIdx, w, halfH, numPixels);
                strokeColor(Color(traceColor.red, traceColor.green, traceColor.blue, 60));
                strokeWidth(fTraceWidth * 2.5f);
                stroke();
            }

            // Sharp Primary Trace Pass
            beginPath();
            renderTracePath(buf, startIdx, w, halfH, numPixels);
            strokeColor(traceColor);
            strokeWidth(fTraceWidth);
            stroke();
        }

        // 4. Border Frame
        beginPath();
        roundedRect(1.0f, 1.0f, w - 2.0f, h - 2.0f, fCornerRadius);
        strokeColor(fBorderColor);
        strokeWidth(1.2f);
        stroke();
    }

private:
    uint32_t fCapacity;
    uint32_t fChannels;
    TriggerMode fTriggerMode;
    float fTriggerThreshold;
    float fVerticalScale;
    float fTimebase;
    float fTraceWidth;
    bool fShowGrid;
    bool fGlow;

    Color fBgColor;
    Color fGridColor;
    Color fAxisColor;
    Color fBorderColor;
    float fCornerRadius;

    std::vector<std::vector<float>> fBuffers;
    std::vector<uint32_t> fWritePos;
    std::vector<Color> fTraceColors;

    void initBuffers()
    {
        fBuffers.resize(fChannels);
        fWritePos.resize(fChannels, 0);
        for (uint32_t c = 0; c < fChannels; ++c)
        {
            fBuffers[c].assign(fCapacity, 0.0f);
        }

        fTraceColors.clear();
        fTraceColors.push_back(Color(52, 220, 100)); // Neon Phosphor Green
        fTraceColors.push_back(Color(52, 160, 240)); // Cyan Blue
        fTraceColors.push_back(Color(240, 180, 52));  // Amber Yellow
        fTraceColors.push_back(Color(240, 60, 120));  // Magenta
    }

    uint32_t findTriggerIndex(const std::vector<float>& buf, const uint32_t currentWrite) const
    {
        const uint32_t searchRange = std::min(fCapacity / 2, 1024U);
        uint32_t triggerIdx = currentWrite;
        bool found = false;

        // Scan backwards from write position looking for threshold crossing
        for (uint32_t i = 1; i < searchRange; ++i)
        {
            const uint32_t idx = (currentWrite + fCapacity - i) % fCapacity;
            const uint32_t prevIdx = (idx + fCapacity - 1) % fCapacity;

            const float curr = buf[idx];
            const float prev = buf[prevIdx];

            if (fTriggerMode == kTriggerRisingEdge || fTriggerMode == kTriggerAuto)
            {
                if (prev <= fTriggerThreshold && curr > fTriggerThreshold)
                {
                    triggerIdx = idx;
                    found = true;
                    break;
                }
            }
            else if (fTriggerMode == kTriggerFallingEdge)
            {
                if (prev >= fTriggerThreshold && curr < fTriggerThreshold)
                {
                    triggerIdx = idx;
                    found = true;
                    break;
                }
            }
        }

        if (!found && fTriggerMode == kTriggerAuto)
        {
            // Fallback: use recent frame window
            triggerIdx = (currentWrite + fCapacity - (searchRange / 2)) % fCapacity;
        }

        return triggerIdx;
    }

    void renderTracePath(const std::vector<float>& buf, const uint32_t startIdx, const float w, const float halfH, const uint32_t numPixels)
    {
        bool first = true;
        for (uint32_t x = 0; x < numPixels; ++x)
        {
            const float sampleOffset = static_cast<float>(x) * fTimebase;
            const uint32_t sIdx = (startIdx + static_cast<uint32_t>(sampleOffset)) % fCapacity;

            const float sampleVal = buf[sIdx] * fVerticalScale;
            // Invert Y: positive is upward
            const float y = halfH - (sampleVal * halfH);

            if (first)
            {
                moveTo(static_cast<float>(x), y);
                first = false;
            }
            else
            {
                lineTo(static_cast<float>(x), y);
            }
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VectorScope)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_SCOPE_HPP_INCLUDED
