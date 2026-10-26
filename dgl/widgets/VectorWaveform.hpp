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

#ifndef DGL_VECTOR_WAVEFORM_HPP_INCLUDED
#define DGL_VECTOR_WAVEFORM_HPP_INCLUDED

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

/**
   High-performance, resolution-independent NanoVG vector audio waveform visualizer and sample editor.
   Features multi-resolution peak/RMS overview mip-mapping, dual-view navigation (overview minimap +
   detail zoom/pan), interactive loop markers, slice/cue markers, playhead tracking, and accessibility.
 */
class VectorWaveform : public NanoSubWidget,
                       public AccessibleElement
{
public:
    struct MipSample {
        float minVal;
        float maxVal;
        float rms;
    };

    struct MipLevel {
        uint32_t step;
        std::vector<MipSample> samples;
    };

    enum DragMode {
        kDragNone = 0,
        kDragScrub,
        kDragMinimapViewport,
        kDragMinimapLeft,
        kDragMinimapRight,
        kDragLoopStart,
        kDragLoopEnd,
        kDragPan
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void vectorWaveformPlayheadScrubbed(VectorWaveform* wf, uint32_t frame)
        {
            (void)wf; (void)frame;
        }
        virtual void vectorWaveformLoopChanged(VectorWaveform* wf, uint32_t startFrame, uint32_t endFrame)
        {
            (void)wf; (void)startFrame; (void)endFrame;
        }
        virtual void vectorWaveformSelectionChanged(VectorWaveform* wf, uint32_t startFrame, uint32_t endFrame)
        {
            (void)wf; (void)startFrame; (void)endFrame;
        }
        virtual void vectorWaveformZoomChanged(VectorWaveform* wf, float zoomFactor)
        {
            (void)wf; (void)zoomFactor;
        }
        virtual void vectorWaveformContextMenu(VectorWaveform* wf, int32_t x, int32_t y)
        {
            (void)wf; (void)x; (void)y;
        }
    };

    typedef void (*ContextMenuCallback)(Widget* widget, int32_t x, int32_t y, void* userData);

    explicit VectorWaveform(Widget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCallback(nullptr),
          fContextMenuCallback(nullptr),
          fContextMenuUserData(nullptr),
          fSampleRate(44100.0),
          fNumFrames(0),
          fNumChannels(0),
          fViewStartFrame(0),
          fViewEndFrame(0),
          fPlayheadFrame(0),
          fPlayheadVisible(true),
          fLoopEnabled(false),
          fLoopStartFrame(0),
          fLoopEndFrame(0),
          fShowMinimap(true),
          fMinimapHeight(28.0f),
          fMinimapAtTop(true),
          fShowGrid(true),
          fCornerRadius(4.0f),
          fDragMode(kDragNone),
          fDragStartFrame(0),
          fDragMouseX(0.0f),
          fDragMouseY(0.0f),
          fBgColor(Color(20, 22, 26)),
          fMinimapBgColor(Color(15, 17, 20)),
          fMinimapWaveColor(Color(70, 80, 95)),
          fViewportBracketColor(Color(80, 160, 255, 120)),
          fGridColor(Color(40, 44, 52)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(55, 60, 70)),
          fWaveformColor(Color(52, 152, 219)),
          fWaveformFillTop(Color(52, 152, 219, 140)),
          fWaveformFillBottom(Color(41, 128, 185, 30)),
          fRmsColor(Color(100, 200, 255, 200)),
          fPlayheadColor(Color(255, 170, 0)),
          fLoopColor(Color(46, 204, 113)),
          fMarkerColor(Color(231, 76, 60))
    {
        initAccessible();
    }

    explicit VectorWaveform(NanoSubWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCallback(nullptr),
          fContextMenuCallback(nullptr),
          fContextMenuUserData(nullptr),
          fSampleRate(44100.0),
          fNumFrames(0),
          fNumChannels(0),
          fViewStartFrame(0),
          fViewEndFrame(0),
          fPlayheadFrame(0),
          fPlayheadVisible(true),
          fLoopEnabled(false),
          fLoopStartFrame(0),
          fLoopEndFrame(0),
          fShowMinimap(true),
          fMinimapHeight(28.0f),
          fMinimapAtTop(true),
          fShowGrid(true),
          fCornerRadius(4.0f),
          fDragMode(kDragNone),
          fDragStartFrame(0),
          fDragMouseX(0.0f),
          fDragMouseY(0.0f),
          fBgColor(Color(20, 22, 26)),
          fMinimapBgColor(Color(15, 17, 20)),
          fMinimapWaveColor(Color(70, 80, 95)),
          fViewportBracketColor(Color(80, 160, 255, 120)),
          fGridColor(Color(40, 44, 52)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(55, 60, 70)),
          fWaveformColor(Color(52, 152, 219)),
          fWaveformFillTop(Color(52, 152, 219, 140)),
          fWaveformFillBottom(Color(41, 128, 185, 30)),
          fRmsColor(Color(100, 200, 255, 200)),
          fPlayheadColor(Color(255, 170, 0)),
          fLoopColor(Color(46, 204, 113)),
          fMarkerColor(Color(231, 76, 60))
    {
        initAccessible();
    }

    explicit VectorWaveform(NanoTopLevelWidget* const parentWidget)
        : NanoSubWidget(parentWidget),
          fCallback(nullptr),
          fContextMenuCallback(nullptr),
          fContextMenuUserData(nullptr),
          fSampleRate(44100.0),
          fNumFrames(0),
          fNumChannels(0),
          fViewStartFrame(0),
          fViewEndFrame(0),
          fPlayheadFrame(0),
          fPlayheadVisible(true),
          fLoopEnabled(false),
          fLoopStartFrame(0),
          fLoopEndFrame(0),
          fShowMinimap(true),
          fMinimapHeight(28.0f),
          fMinimapAtTop(true),
          fShowGrid(true),
          fCornerRadius(4.0f),
          fDragMode(kDragNone),
          fDragStartFrame(0),
          fDragMouseX(0.0f),
          fDragMouseY(0.0f),
          fBgColor(Color(20, 22, 26)),
          fMinimapBgColor(Color(15, 17, 20)),
          fMinimapWaveColor(Color(70, 80, 95)),
          fViewportBracketColor(Color(80, 160, 255, 120)),
          fGridColor(Color(40, 44, 52)),
          fAxisColor(Color(65, 70, 80)),
          fBorderColor(Color(55, 60, 70)),
          fWaveformColor(Color(52, 152, 219)),
          fWaveformFillTop(Color(52, 152, 219, 140)),
          fWaveformFillBottom(Color(41, 128, 185, 30)),
          fRmsColor(Color(100, 200, 255, 200)),
          fPlayheadColor(Color(255, 170, 0)),
          fLoopColor(Color(46, 204, 113)),
          fMarkerColor(Color(231, 76, 60))
    {
        initAccessible();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Audio Data Management

    void setAudioData(const float* const* const channelData,
                      const uint32_t numChannels,
                      const uint32_t numFrames,
                      const double sampleRate = 44100.0)
    {
        if (channelData == nullptr || numChannels == 0 || numFrames == 0)
        {
            clearAudioData();
            return;
        }

        fSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        fNumFrames = numFrames;
        fNumChannels = numChannels;

        fRawData.resize(numFrames);
        for (uint32_t i = 0; i < numFrames; ++i)
        {
            float sum = 0.0f;
            for (uint32_t ch = 0; ch < numChannels; ++ch)
            {
                if (channelData[ch] != nullptr)
                    sum += channelData[ch][i];
            }
            fRawData[i] = sum / static_cast<float>(numChannels);
        }

        buildMipMaps();

        fViewStartFrame = 0;
        fViewEndFrame = numFrames;
        fLoopStartFrame = 0;
        fLoopEndFrame = numFrames;
        fPlayheadFrame = 0;

        updateAccessible();
        repaint();
    }

    void setAudioData(const float* const monoData,
                      const uint32_t numFrames,
                      const double sampleRate = 44100.0)
    {
        const float* const ptrs[1] = { monoData };
        setAudioData(ptrs, 1, numFrames, sampleRate);
    }

    void clearAudioData()
    {
        fNumFrames = 0;
        fNumChannels = 0;
        fRawData.clear();
        fMipLevels.clear();
        fViewStartFrame = 0;
        fViewEndFrame = 0;
        fLoopStartFrame = 0;
        fLoopEndFrame = 0;
        fPlayheadFrame = 0;
        fSliceMarkers.clear();

        updateAccessible();
        repaint();
    }

    uint32_t getNumFrames() const noexcept { return fNumFrames; }
    uint32_t getNumChannels() const noexcept { return fNumChannels; }
    double getSampleRate() const noexcept { return fSampleRate; }
    double getDurationSeconds() const noexcept
    {
        return fSampleRate > 0.0 ? static_cast<double>(fNumFrames) / fSampleRate : 0.0;
    }

    const std::vector<float>& getRawData() const noexcept { return fRawData; }
    const std::vector<MipLevel>& getMipLevels() const noexcept { return fMipLevels; }

    // ----------------------------------------------------------------------------------------------------------------
    // Viewport & Zoom Navigation

    void setViewport(const uint32_t startFrame, const uint32_t endFrame)
    {
        if (fNumFrames == 0)
            return;

        const uint32_t minSpan = 16;
        fViewStartFrame = std::min(startFrame, fNumFrames - minSpan);
        fViewEndFrame = std::max(fViewStartFrame + minSpan, std::min(endFrame, fNumFrames));

        if (fCallback != nullptr)
            fCallback->vectorWaveformZoomChanged(this, getZoomFactor());

        repaint();
    }

    uint32_t getViewStartFrame() const noexcept { return fViewStartFrame; }
    uint32_t getViewEndFrame() const noexcept { return fViewEndFrame; }

    float getZoomFactor() const noexcept
    {
        if (fNumFrames == 0 || fViewEndFrame <= fViewStartFrame)
            return 1.0f;
        return static_cast<float>(fNumFrames) / static_cast<float>(fViewEndFrame - fViewStartFrame);
    }

    void zoom(const float factor, const float focusNorm = 0.5f)
    {
        if (fNumFrames == 0 || factor <= 0.0f)
            return;

        const uint32_t currentSpan = fViewEndFrame - fViewStartFrame;
        const uint32_t minSpan = 16;
        const uint32_t newSpan = static_cast<uint32_t>(clampVal(static_cast<float>(currentSpan) / factor,
                                                                 static_cast<float>(minSpan),
                                                                 static_cast<float>(fNumFrames)));

        const double focusFrame = static_cast<double>(fViewStartFrame) + static_cast<double>(currentSpan) * clampVal(focusNorm, 0.0f, 1.0f);
        int64_t newStart = static_cast<int64_t>(focusFrame - static_cast<double>(newSpan) * clampVal(focusNorm, 0.0f, 1.0f));
        int64_t newEnd = newStart + newSpan;

        if (newStart < 0)
        {
            newEnd -= newStart;
            newStart = 0;
        }
        if (newEnd > static_cast<int64_t>(fNumFrames))
        {
            newStart -= (newEnd - static_cast<int64_t>(fNumFrames));
            newEnd = fNumFrames;
            if (newStart < 0)
                newStart = 0;
        }

        setViewport(static_cast<uint32_t>(newStart), static_cast<uint32_t>(newEnd));
    }

    void pan(const float deltaNorm)
    {
        if (fNumFrames == 0)
            return;

        const uint32_t span = fViewEndFrame - fViewStartFrame;
        const int64_t deltaFrames = static_cast<int64_t>(static_cast<float>(span) * deltaNorm);
        int64_t newStart = static_cast<int64_t>(fViewStartFrame) + deltaFrames;
        int64_t newEnd = static_cast<int64_t>(fViewEndFrame) + deltaFrames;

        if (newStart < 0)
        {
            newEnd += -newStart;
            newStart = 0;
        }
        if (newEnd > static_cast<int64_t>(fNumFrames))
        {
            newStart -= (newEnd - static_cast<int64_t>(fNumFrames));
            newEnd = fNumFrames;
            if (newStart < 0)
                newStart = 0;
        }

        setViewport(static_cast<uint32_t>(newStart), static_cast<uint32_t>(newEnd));
    }

    void resetView()
    {
        setViewport(0, fNumFrames);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Loop Region & Playhead

    void setLoopRange(const uint32_t startFrame, const uint32_t endFrame)
    {
        if (fNumFrames == 0)
            return;

        fLoopStartFrame = std::min(startFrame, fNumFrames);
        fLoopEndFrame = std::max(fLoopStartFrame, std::min(endFrame, fNumFrames));

        if (fCallback != nullptr)
            fCallback->vectorWaveformLoopChanged(this, fLoopStartFrame, fLoopEndFrame);

        repaint();
    }

    uint32_t getLoopStartFrame() const noexcept { return fLoopStartFrame; }
    uint32_t getLoopEndFrame() const noexcept { return fLoopEndFrame; }

    void setLoopEnabled(const bool enabled)
    {
        if (fLoopEnabled == enabled)
            return;

        fLoopEnabled = enabled;
        repaint();
    }

    bool isLoopEnabled() const noexcept { return fLoopEnabled; }

    void setPlayheadPosition(const uint32_t frame)
    {
        fPlayheadFrame = std::min(frame, fNumFrames);
        repaint();
    }

    void setPlayheadNormalized(const float norm)
    {
        if (fNumFrames == 0)
            return;
        setPlayheadPosition(static_cast<uint32_t>(clampVal(norm, 0.0f, 1.0f) * static_cast<float>(fNumFrames)));
    }

    uint32_t getPlayheadPosition() const noexcept { return fPlayheadFrame; }

    float getPlayheadNormalized() const noexcept
    {
        return fNumFrames > 0 ? static_cast<float>(fPlayheadFrame) / static_cast<float>(fNumFrames) : 0.0f;
    }

    void setPlayheadVisible(const bool visible)
    {
        if (fPlayheadVisible == visible)
            return;
        fPlayheadVisible = visible;
        repaint();
    }

    bool isPlayheadVisible() const noexcept { return fPlayheadVisible; }

    // ----------------------------------------------------------------------------------------------------------------
    // Slice / Cue Markers

    void addSliceMarker(const uint32_t frame)
    {
        if (frame <= fNumFrames)
        {
            fSliceMarkers.push_back(frame);
            std::sort(fSliceMarkers.begin(), fSliceMarkers.end());
            repaint();
        }
    }

    void clearSliceMarkers()
    {
        fSliceMarkers.clear();
        repaint();
    }

    const std::vector<uint32_t>& getSliceMarkers() const noexcept { return fSliceMarkers; }

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration & Visual Styling

    void setShowMinimap(const bool show)
    {
        if (fShowMinimap == show)
            return;
        fShowMinimap = show;
        repaint();
    }

    bool getShowMinimap() const noexcept { return fShowMinimap; }

    void setMinimapHeight(const float height)
    {
        fMinimapHeight = height > 16.0f ? height : 16.0f;
        repaint();
    }

    void setMinimapAtTop(const bool atTop)
    {
        if (fMinimapAtTop == atTop)
            return;
        fMinimapAtTop = atTop;
        repaint();
    }

    void setShowGrid(const bool show)
    {
        if (fShowGrid == show)
            return;
        fShowGrid = show;
        repaint();
    }

    void setCallback(Callback* const cb) noexcept { fCallback = cb; }
    void setContextMenuCallback(ContextMenuCallback const cb, void* const userData = nullptr) noexcept
    {
        fContextMenuCallback = cb;
        fContextMenuUserData = userData;
    }

    // Color customization
    void setWaveformColor(const Color& c) noexcept { fWaveformColor = c; repaint(); }
    void setPlayheadColor(const Color& c) noexcept { fPlayheadColor = c; repaint(); }
    void setLoopColor(const Color& c) noexcept { fLoopColor = c; repaint(); }
    void setMarkerColor(const Color& c) noexcept { fMarkerColor = c; repaint(); }
    void setBgColor(const Color& c) noexcept { fBgColor = c; repaint(); }

    // ----------------------------------------------------------------------------------------------------------------
    // Coordinate & Hit-Testing Helpers

    struct ViewportRects {
        Rectangle<float> minimap;
        Rectangle<float> detail;
        Rectangle<float> bracket;
    };

    ViewportRects getLayoutRects() const
    {
        ViewportRects rects;
        const float w = static_cast<float>(getWidth());
        const float h = static_cast<float>(getHeight());

        if (fShowMinimap && h > fMinimapHeight + 20.0f)
        {
            if (fMinimapAtTop)
            {
                rects.minimap = Rectangle<float>(0.0f, 0.0f, w, fMinimapHeight);
                rects.detail = Rectangle<float>(0.0f, fMinimapHeight, w, h - fMinimapHeight);
            }
            else
            {
                rects.detail = Rectangle<float>(0.0f, 0.0f, w, h - fMinimapHeight);
                rects.minimap = Rectangle<float>(0.0f, h - fMinimapHeight, w, fMinimapHeight);
            }

            // Viewport bracket on minimap
            if (fNumFrames > 0)
            {
                const float x0 = (static_cast<float>(fViewStartFrame) / static_cast<float>(fNumFrames)) * w;
                const float x1 = (static_cast<float>(fViewEndFrame) / static_cast<float>(fNumFrames)) * w;
                rects.bracket = Rectangle<float>(x0, rects.minimap.getY(), std::max(6.0f, x1 - x0), fMinimapHeight);
            }
            else
            {
                rects.bracket = rects.minimap;
            }
        }
        else
        {
            rects.minimap = Rectangle<float>(0.0f, 0.0f, 0.0f, 0.0f);
            rects.detail = Rectangle<float>(0.0f, 0.0f, w, h);
            rects.bracket = Rectangle<float>(0.0f, 0.0f, 0.0f, 0.0f);
        }

        return rects;
    }

    uint32_t frameFromDetailX(const float x) const
    {
        if (fNumFrames == 0 || fViewEndFrame <= fViewStartFrame)
            return 0;

        const float w = static_cast<float>(getWidth());
        if (w <= 0.0f)
            return fViewStartFrame;

        const float ratio = clampVal(x / w, 0.0f, 1.0f);
        const double span = static_cast<double>(fViewEndFrame - fViewStartFrame);
        return static_cast<uint32_t>(static_cast<double>(fViewStartFrame) + ratio * span);
    }

    float detailXFromFrame(const uint32_t frame) const
    {
        if (fNumFrames == 0 || fViewEndFrame <= fViewStartFrame)
            return 0.0f;

        const float w = static_cast<float>(getWidth());
        const double span = static_cast<double>(fViewEndFrame - fViewStartFrame);
        const double offset = static_cast<double>(frame) - static_cast<double>(fViewStartFrame);
        return static_cast<float>((offset / span) * static_cast<double>(w));
    }

    uint32_t frameFromMinimapX(const float x) const
    {
        if (fNumFrames == 0)
            return 0;

        const float w = static_cast<float>(getWidth());
        if (w <= 0.0f)
            return 0;

        const float ratio = clampVal(x / w, 0.0f, 1.0f);
        return static_cast<uint32_t>(ratio * static_cast<float>(fNumFrames));
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Mouse Event Handlers

    bool onMouse(const MouseEvent& ev) override
    {
        const ViewportRects rects = getLayoutRects();
        const float mx = static_cast<float>(ev.pos.getX());
        const float my = static_cast<float>(ev.pos.getY());

        // Context menu (Right click)
        if (ev.press && ev.button == 3)
        {
            if (fContextMenuCallback != nullptr)
                fContextMenuCallback(this, static_cast<int32_t>(mx), static_cast<int32_t>(my), fContextMenuUserData);
            if (fCallback != nullptr)
                fCallback->vectorWaveformContextMenu(this, static_cast<int32_t>(mx), static_cast<int32_t>(my));
            return true;
        }

        if (ev.press && ev.button == 1)
        {
            fDragMouseX = mx;
            fDragMouseY = my;

            // 1. Hit-test Minimap
            if (fShowMinimap && rects.minimap.contains(mx, my))
            {
                const float bracketX0 = rects.bracket.getX();
                const float bracketX1 = rects.bracket.getX() + rects.bracket.getWidth();
                const float handleRadius = 6.0f;

                if (std::abs(mx - bracketX0) <= handleRadius)
                {
                    fDragMode = kDragMinimapLeft;
                }
                else if (std::abs(mx - bracketX1) <= handleRadius)
                {
                    fDragMode = kDragMinimapRight;
                }
                else if (rects.bracket.contains(mx, my))
                {
                    fDragMode = kDragMinimapViewport;
                    fDragStartFrame = frameFromMinimapX(mx);
                }
                else
                {
                    // Click outside bracket jumps viewport center
                    const uint32_t targetFrame = frameFromMinimapX(mx);
                    const uint32_t span = fViewEndFrame - fViewStartFrame;
                    const int64_t newStart = static_cast<int64_t>(targetFrame) - static_cast<int64_t>(span / 2);
                    setViewport(static_cast<uint32_t>(std::max<int64_t>(0, newStart)),
                                static_cast<uint32_t>(std::max<int64_t>(0, newStart) + span));
                    fDragMode = kDragMinimapViewport;
                    fDragStartFrame = targetFrame;
                }
                return true;
            }

            // 2. Hit-test Detail View
            if (rects.detail.contains(mx, my))
            {
                // Hit-test loop markers if loop is enabled
                if (fLoopEnabled)
                {
                    const float loopStartX = detailXFromFrame(fLoopStartFrame);
                    const float loopEndX = detailXFromFrame(fLoopEndFrame);
                    const float loopHandleRadius = 8.0f;

                    if (std::abs(mx - loopStartX) <= loopHandleRadius)
                    {
                        fDragMode = kDragLoopStart;
                        return true;
                    }
                    if (std::abs(mx - loopEndX) <= loopHandleRadius)
                    {
                        fDragMode = kDragLoopEnd;
                        return true;
                    }
                }

                // Middle click pans viewport
                if (ev.button == 2)
                {
                    fDragMode = kDragPan;
                    return true;
                }

                // Left click scrubs playhead
                const uint32_t scrubFrame = frameFromDetailX(mx);
                setPlayheadPosition(scrubFrame);
                fDragMode = kDragScrub;

                if (fCallback != nullptr)
                    fCallback->vectorWaveformPlayheadScrubbed(this, scrubFrame);

                return true;
            }
        }
        else if (!ev.press && ev.button == 1)
        {
            if (fDragMode != kDragNone)
            {
                fDragMode = kDragNone;
                return true;
            }
        }

        return false;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        const float mx = static_cast<float>(ev.pos.getX());
        const float my = static_cast<float>(ev.pos.getY());
        const float deltaX = mx - fDragMouseX;
        fDragMouseX = mx;
        fDragMouseY = my;

        if (fDragMode == kDragNone)
            return false;

        const float w = static_cast<float>(getWidth());
        if (w <= 0.0f || fNumFrames == 0)
            return false;

        switch (fDragMode)
        {
        case kDragScrub:
        {
            const uint32_t scrubFrame = frameFromDetailX(mx);
            setPlayheadPosition(scrubFrame);
            if (fCallback != nullptr)
                fCallback->vectorWaveformPlayheadScrubbed(this, scrubFrame);
            return true;
        }
        case kDragMinimapViewport:
        {
            const float deltaNorm = deltaX / w;
            pan(deltaNorm);
            return true;
        }
        case kDragMinimapLeft:
        {
            const uint32_t frame = frameFromMinimapX(mx);
            if (frame + 16 < fViewEndFrame)
                setViewport(frame, fViewEndFrame);
            return true;
        }
        case kDragMinimapRight:
        {
            const uint32_t frame = frameFromMinimapX(mx);
            if (frame > fViewStartFrame + 16)
                setViewport(fViewStartFrame, frame);
            return true;
        }
        case kDragLoopStart:
        {
            const uint32_t frame = frameFromDetailX(mx);
            if (frame < fLoopEndFrame)
                setLoopRange(frame, fLoopEndFrame);
            return true;
        }
        case kDragLoopEnd:
        {
            const uint32_t frame = frameFromDetailX(mx);
            if (frame > fLoopStartFrame)
                setLoopRange(fLoopStartFrame, frame);
            return true;
        }
        case kDragPan:
        {
            const float deltaNorm = -deltaX / w;
            pan(deltaNorm);
            return true;
        }
        default:
            break;
        }

        return false;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const ViewportRects rects = getLayoutRects();
        const float mx = static_cast<float>(ev.pos.getX());
        const float my = static_cast<float>(ev.pos.getY());

        if (rects.detail.contains(mx, my))
        {
            const float zoomDelta = ev.delta.getY() > 0 ? 1.25f : 0.80f;
            const float focusNorm = clampVal(mx / static_cast<float>(getWidth()), 0.0f, 1.0f);
            zoom(zoomDelta, focusNorm);
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
        if (w <= 2.0f || h <= 2.0f)
            return;

        const ViewportRects rects = getLayoutRects();

        // 1. Base Container Background
        beginPath();
        roundedRect(0.0f, 0.0f, w, h, fCornerRadius);
        fillColor(fBgColor);
        fill();
        strokeColor(fBorderColor);
        strokeWidth(1.0f);
        stroke();

        // 2. Minimap Strip
        if (fShowMinimap && rects.minimap.getHeight() > 0.0f)
            drawMinimap(rects.minimap, rects.bracket);

        // 3. Detail View
        if (rects.detail.getHeight() > 0.0f)
            drawDetailView(rects.detail);
    }

protected:
    void initAccessible()
    {
        fAccessibleInfo.setRole(kAccessibleRoleIndicator);
        fAccessibleInfo.setLabel("Audio Waveform Visualizer");
        fAccessibleInfo.setDescription("Audio buffer waveform overview and playback display");
        fAccessibleInfo.minValue = 0.0f;
        fAccessibleInfo.maxValue = 1.0f;
        fAccessibleInfo.step = 0.01f;
    }

    void updateAccessible()
    {
        const float norm = getPlayheadNormalized();
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Frames: %u, Playhead: %.1f%%", fNumFrames, norm * 100.0f);
        fAccessibleInfo.setValue(norm, buf);
    }

private:
    Callback* fCallback;
    ContextMenuCallback fContextMenuCallback;
    void* fContextMenuUserData;

    double fSampleRate;
    uint32_t fNumFrames;
    uint32_t fNumChannels;
    std::vector<float> fRawData;
    std::vector<MipLevel> fMipLevels;

    uint32_t fViewStartFrame;
    uint32_t fViewEndFrame;
    uint32_t fPlayheadFrame;
    bool fPlayheadVisible;

    bool fLoopEnabled;
    uint32_t fLoopStartFrame;
    uint32_t fLoopEndFrame;

    std::vector<uint32_t> fSliceMarkers;

    bool fShowMinimap;
    float fMinimapHeight;
    bool fMinimapAtTop;
    bool fShowGrid;
    float fCornerRadius;

    DragMode fDragMode;
    uint32_t fDragStartFrame;
    float fDragMouseX;
    float fDragMouseY;

    // Theme colors
    Color fBgColor;
    Color fMinimapBgColor;
    Color fMinimapWaveColor;
    Color fViewportBracketColor;
    Color fGridColor;
    Color fAxisColor;
    Color fBorderColor;
    Color fWaveformColor;
    Color fWaveformFillTop;
    Color fWaveformFillBottom;
    Color fRmsColor;
    Color fPlayheadColor;
    Color fLoopColor;
    Color fMarkerColor;

    template<typename T>
    static inline T clampVal(const T val, const T minVal, const T maxVal)
    {
        return (val < minVal) ? minVal : ((val > maxVal) ? maxVal : val);
    }

    void buildMipMaps()
    {
        fMipLevels.clear();
        if (fRawData.empty())
            return;

        const uint32_t steps[] = { 16, 256, 4096 };
        for (size_t s = 0; s < 3; ++s)
        {
            const uint32_t step = steps[s];
            MipLevel level;
            level.step = step;

            const size_t numBlocks = (fRawData.size() + step - 1) / step;
            level.samples.resize(numBlocks);

            for (size_t b = 0; b < numBlocks; ++b)
            {
                const size_t start = b * step;
                const size_t end = std::min(start + step, fRawData.size());

                float minV = fRawData[start];
                float maxV = fRawData[start];
                float sumSq = 0.0f;

                for (size_t i = start; i < end; ++i)
                {
                    const float sVal = fRawData[i];
                    if (sVal < minV) minV = sVal;
                    if (sVal > maxV) maxV = sVal;
                    sumSq += sVal * sVal;
                }

                level.samples[b].minVal = minV;
                level.samples[b].maxVal = maxV;
                level.samples[b].rms = std::sqrt(sumSq / static_cast<float>(end - start));
            }

            fMipLevels.push_back(level);
        }
    }

    void drawMinimap(const Rectangle<float>& r, const Rectangle<float>& bracket)
    {
        beginPath();
        rect(r.getX(), r.getY(), r.getWidth(), r.getHeight());
        fillColor(fMinimapBgColor);
        fill();

        // Divider line between minimap and detail view
        beginPath();
        const float divY = fMinimapAtTop ? (r.getY() + r.getHeight()) : r.getY();
        moveTo(r.getX(), divY);
        lineTo(r.getX() + r.getWidth(), divY);
        strokeColor(fBorderColor);
        strokeWidth(1.0f);
        stroke();

        // Waveform overview using Mip Level 2 or 1
        if (fNumFrames > 0 && !fMipLevels.empty())
        {
            const MipLevel& mip = fMipLevels.back();
            const float midY = r.getY() + r.getHeight() * 0.5f;
            const float scaleY = r.getHeight() * 0.45f;
            const float w = r.getWidth();

            beginPath();
            for (float px = 0.0f; px < w; px += 1.0f)
            {
                const size_t idx = static_cast<size_t>((px / w) * static_cast<float>(mip.samples.size()));
                if (idx < mip.samples.size())
                {
                    const float y0 = midY - mip.samples[idx].maxVal * scaleY;
                    const float y1 = midY - mip.samples[idx].minVal * scaleY;
                    moveTo(r.getX() + px, y0);
                    lineTo(r.getX() + px, y1);
                }
            }
            strokeColor(fMinimapWaveColor);
            strokeWidth(1.0f);
            stroke();
        }

        // Viewport Bracket
        beginPath();
        rect(bracket.getX(), bracket.getY(), bracket.getWidth(), bracket.getHeight());
        fillColor(Color(80, 160, 255, 45));
        fill();
        strokeColor(fViewportBracketColor);
        strokeWidth(1.5f);
        stroke();
    }

    void drawDetailView(const Rectangle<float>& r)
    {
        const float x = r.getX();
        const float y = r.getY();
        const float w = r.getWidth();
        const float h = r.getHeight();
        const float midY = y + h * 0.5f;

        // Clip to detail rectangle
        save();
        scissor(x, y, w, h);

        // 1. Grid lines
        if (fShowGrid)
        {
            beginPath();
            // Center 0 dB axis
            moveTo(x, midY);
            lineTo(x + w, midY);
            strokeColor(fAxisColor);
            strokeWidth(1.0f);
            stroke();

            // +6dB and -6dB lines (half scale)
            beginPath();
            moveTo(x, midY - h * 0.25f);
            lineTo(x + w, midY - h * 0.25f);
            moveTo(x, midY + h * 0.25f);
            lineTo(x + w, midY + h * 0.25f);
            strokeColor(fGridColor);
            strokeWidth(1.0f);
            stroke();
        }

        // 2. Loop Region Overlay
        if (fLoopEnabled && fLoopEndFrame > fLoopStartFrame)
        {
            const float lx0 = detailXFromFrame(fLoopStartFrame);
            const float lx1 = detailXFromFrame(fLoopEndFrame);

            beginPath();
            rect(lx0, y, std::max(1.0f, lx1 - lx0), h);
            fillColor(Color(fLoopColor.red, fLoopColor.green, fLoopColor.blue, 0.20f));
            fill();

            // Loop boundary lines
            beginPath();
            moveTo(lx0, y); lineTo(lx0, y + h);
            moveTo(lx1, y); lineTo(lx1, y + h);
            strokeColor(fLoopColor);
            strokeWidth(1.5f);
            stroke();

            // Loop start flag (top triangle)
            beginPath();
            moveTo(lx0, y);
            lineTo(lx0 + 8.0f, y);
            lineTo(lx0, y + 8.0f);
            closePath();
            fillColor(fLoopColor);
            fill();

            // Loop end flag (top triangle)
            beginPath();
            moveTo(lx1, y);
            lineTo(lx1 - 8.0f, y);
            lineTo(lx1, y + 8.0f);
            closePath();
            fillColor(fLoopColor);
            fill();
        }

        // 3. Waveform Peak & RMS Drawing
        if (fNumFrames > 0 && fViewEndFrame > fViewStartFrame)
        {
            const uint32_t span = fViewEndFrame - fViewStartFrame;
            const float framesPerPixel = static_cast<float>(span) / w;
            const float scaleY = h * 0.45f;

            if (framesPerPixel <= 2.0f)
            {
                // High zoom: Direct sample line plot
                beginPath();
                bool started = false;
                for (float px = 0.0f; px < w; px += 1.0f)
                {
                    const uint32_t frame = frameFromDetailX(px);
                    if (frame < fRawData.size())
                    {
                        const float sy = midY - fRawData[frame] * scaleY;
                        if (!started)
                        {
                            moveTo(x + px, sy);
                            started = true;
                        }
                        else
                        {
                            lineTo(x + px, sy);
                        }
                    }
                }
                strokeColor(fWaveformColor);
                strokeWidth(1.5f);
                stroke();
            }
            else
            {
                // Mip-mapped peak envelopes
                const MipLevel* selectedMip = nullptr;
                for (size_t i = 0; i < fMipLevels.size(); ++i)
                {
                    if (fMipLevels[i].step <= static_cast<uint32_t>(framesPerPixel) || i == 0)
                        selectedMip = &fMipLevels[i];
                }

                if (selectedMip != nullptr && !selectedMip->samples.empty())
                {
                    // Shaded gradient fill from center to peaks
                    Paint grad = linearGradient(x, midY - scaleY, x, midY + scaleY, fWaveformFillTop, fWaveformFillBottom);

                    // Peak lines
                    beginPath();
                    for (float px = 0.0f; px < w; px += 1.0f)
                    {
                        const uint32_t frame = frameFromDetailX(px);
                        const size_t block = frame / selectedMip->step;
                        if (block < selectedMip->samples.size())
                        {
                            const float topY = midY - selectedMip->samples[block].maxVal * scaleY;
                            const float btmY = midY - selectedMip->samples[block].minVal * scaleY;
                            moveTo(x + px, topY);
                            lineTo(x + px, btmY);
                        }
                    }
                    strokePaint(grad);
                    strokeWidth(1.0f);
                    stroke();

                    // RMS glow layer
                    beginPath();
                    for (float px = 0.0f; px < w; px += 2.0f)
                    {
                        const uint32_t frame = frameFromDetailX(px);
                        const size_t block = frame / selectedMip->step;
                        if (block < selectedMip->samples.size())
                        {
                            const float rms = selectedMip->samples[block].rms;
                            moveTo(x + px, midY - rms * scaleY);
                            lineTo(x + px, midY + rms * scaleY);
                        }
                    }
                    strokeColor(fRmsColor);
                    strokeWidth(1.0f);
                    stroke();
                }
            }
        }

        // 4. Slice / Cue Markers
        for (size_t i = 0; i < fSliceMarkers.size(); ++i)
        {
            const uint32_t markerFrame = fSliceMarkers[i];
            if (markerFrame >= fViewStartFrame && markerFrame <= fViewEndFrame)
            {
                const float mx = detailXFromFrame(markerFrame);
                beginPath();
                moveTo(mx, y);
                lineTo(mx, y + h);
                strokeColor(fMarkerColor);
                strokeWidth(1.0f);
                stroke();

                // Marker top flag
                beginPath();
                moveTo(mx, y);
                lineTo(mx + 6.0f, y);
                lineTo(mx, y + 6.0f);
                closePath();
                fillColor(fMarkerColor);
                fill();
            }
        }

        // 5. Playhead Line
        if (fPlayheadVisible && fPlayheadFrame >= fViewStartFrame && fPlayheadFrame <= fViewEndFrame)
        {
            const float px = detailXFromFrame(fPlayheadFrame);

            // Glowing playhead line
            beginPath();
            moveTo(px, y);
            lineTo(px, y + h);
            strokeColor(fPlayheadColor);
            strokeWidth(2.0f);
            stroke();

            // Playhead triangle cap
            beginPath();
            moveTo(px - 5.0f, y);
            lineTo(px + 5.0f, y);
            lineTo(px, y + 8.0f);
            closePath();
            fillColor(fPlayheadColor);
            fill();
        }

        restore();
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_VECTOR_WAVEFORM_HPP_INCLUDED
