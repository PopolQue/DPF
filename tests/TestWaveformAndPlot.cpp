/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for VectorWaveform and VectorPlot Visualizer Widgets
 */

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "dgl/Application.hpp"
#include "dgl/Color.hpp"
#include "dgl/Geometry.hpp"
#include "dgl/Window.hpp"
#include "dgl/widgets/VectorWaveform.hpp"
#include "dgl/widgets/VectorPlot.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Mock Callbacks for Testing

struct MockWaveformCallback : DGL_NAMESPACE::VectorWaveform::Callback {
    bool scrubbedCalled;
    uint32_t lastScrubbedFrame;
    bool loopChangedCalled;
    uint32_t lastLoopStart;
    uint32_t lastLoopEnd;
    bool zoomChangedCalled;
    float lastZoomFactor;

    MockWaveformCallback()
        : scrubbedCalled(false),
          lastScrubbedFrame(0),
          loopChangedCalled(false),
          lastLoopStart(0),
          lastLoopEnd(0),
          zoomChangedCalled(false),
          lastZoomFactor(1.0f)
    {
    }

    void vectorWaveformPlayheadScrubbed(DGL_NAMESPACE::VectorWaveform*, uint32_t frame) override
    {
        scrubbedCalled = true;
        lastScrubbedFrame = frame;
    }

    void vectorWaveformLoopChanged(DGL_NAMESPACE::VectorWaveform*, uint32_t start, uint32_t end) override
    {
        loopChangedCalled = true;
        lastLoopStart = start;
        lastLoopEnd = end;
    }

    void vectorWaveformZoomChanged(DGL_NAMESPACE::VectorWaveform*, float factor) override
    {
        zoomChangedCalled = true;
        lastZoomFactor = factor;
    }
};

struct MockPlotCallback : DGL_NAMESPACE::VectorPlot::Callback {
    bool bandChangedCalled;
    int lastBandIndex;
    float lastFreq;
    float lastGainDb;
    float lastQ;
    bool bandSelectedCalled;
    int lastSelectedBand;

    MockPlotCallback()
        : bandChangedCalled(false),
          lastBandIndex(-1),
          lastFreq(0.0f),
          lastGainDb(0.0f),
          lastQ(0.0f),
          bandSelectedCalled(false),
          lastSelectedBand(-1)
    {
    }

    void vectorPlotBandChanged(DGL_NAMESPACE::VectorPlot*, int bandIndex, float freq, float gainDb, float q) override
    {
        bandChangedCalled = true;
        lastBandIndex = bandIndex;
        lastFreq = freq;
        lastGainDb = gainDb;
        lastQ = q;
    }

    void vectorPlotBandSelected(DGL_NAMESPACE::VectorPlot*, int bandIndex) override
    {
        bandSelectedCalled = true;
        lastSelectedBand = bandIndex;
    }
};

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    DGL_NAMESPACE::Application app(true);
    DGL_NAMESPACE::Window win(app);
    DGL_NAMESPACE::TopLevelWidget container(win);

    std::cout << "=== Testing VectorWaveform and VectorPlot Architecture ===" << std::endl;

    // ================================================================================================================
    // Part 1: VectorWaveform Architecture Tests
    // ================================================================================================================
    std::cout << "--- Testing VectorWaveform Buffer Loading & Multi-Resolution Mips ---" << std::endl;
    {
        DGL_NAMESPACE::VectorWaveform waveform(&container);
        waveform.setSize(800, 300);

        const uint32_t numFrames = 44100; // 1 second of audio at 44.1kHz
        std::vector<float> leftChannel(numFrames);
        std::vector<float> rightChannel(numFrames);

        // Generate synthetic stereo audio: 440Hz sine in left, 880Hz sine in right
        for (uint32_t i = 0; i < numFrames; ++i)
        {
            const double t = static_cast<double>(i) / 44100.0;
            leftChannel[i] = static_cast<float>(std::sin(2.0 * M_PI * 440.0 * t) * 0.8);
            rightChannel[i] = static_cast<float>(std::sin(2.0 * M_PI * 880.0 * t) * 0.6);
        }

        const float* const channels[2] = { leftChannel.data(), rightChannel.data() };
        waveform.setAudioData(channels, 2, numFrames, 44100.0);

        assert(waveform.getNumFrames() == numFrames);
        assert(waveform.getNumChannels() == 2);
        assert(std::abs(waveform.getDurationSeconds() - 1.0) < 1e-4);
        assert(waveform.getViewStartFrame() == 0);
        assert(waveform.getViewEndFrame() == numFrames);

        // Verify Mip-Maps (Steps: 16, 256, 4096)
        const std::vector<DGL_NAMESPACE::VectorWaveform::MipLevel>& mips = waveform.getMipLevels();
        assert(mips.size() == 3);
        assert(mips[0].step == 16);
        assert(mips[0].samples.size() == (numFrames + 15) / 16);
        assert(mips[1].step == 256);
        assert(mips[1].samples.size() == (numFrames + 255) / 256);
        assert(mips[2].step == 4096);
        assert(mips[2].samples.size() == (numFrames + 4095) / 4096);

        // Verify that sample min/max envelope covers expected ranges
        for (size_t i = 0; i < mips[0].samples.size(); ++i)
        {
            assert(mips[0].samples[i].maxVal >= mips[0].samples[i].minVal);
            assert(mips[0].samples[i].rms >= 0.0f);
        }

        std::cout << "  ✓ Stereo downmix, frame count, duration, and 3-tier Mip-Maps verified" << std::endl;
    }

    std::cout << "--- Testing VectorWaveform Viewport Navigation & Zoom Math ---" << std::endl;
    {
        DGL_NAMESPACE::VectorWaveform waveform(&container);
        waveform.setSize(1000, 400);

        const uint32_t numFrames = 100000;
        std::vector<float> buffer(numFrames, 0.0f);
        waveform.setAudioData(buffer.data(), numFrames, 44100.0);

        MockWaveformCallback cb;
        waveform.setCallback(&cb);

        assert(waveform.getZoomFactor() == 1.0f);

        // Zoom 2x centered at middle (focusNorm = 0.5)
        waveform.zoom(2.0f, 0.5f);
        assert(cb.zoomChangedCalled == true);
        const uint32_t span = waveform.getViewEndFrame() - waveform.getViewStartFrame();
        assert(std::abs(static_cast<int>(span) - 50000) < 5);
        assert(std::abs(static_cast<int>(waveform.getViewStartFrame()) - 25000) < 5);
        assert(std::abs(static_cast<int>(waveform.getViewEndFrame()) - 75000) < 5);
        (void)span;

        // Pan +10%
        const uint32_t prevStart = waveform.getViewStartFrame();
        waveform.pan(0.10f);
        assert(waveform.getViewStartFrame() > prevStart);
        (void)prevStart;

        // Reset view
        waveform.resetView();
        assert(waveform.getViewStartFrame() == 0);
        assert(waveform.getViewEndFrame() == numFrames);
        assert(waveform.getZoomFactor() == 1.0f);

        std::cout << "  ✓ Zoom ratio, centered focus zooming, panning, and resetView verified" << std::endl;
    }

    std::cout << "--- Testing VectorWaveform Playhead, Loop, and Slice Markers ---" << std::endl;
    {
        DGL_NAMESPACE::VectorWaveform waveform(&container);
        waveform.setSize(800, 300);

        const uint32_t numFrames = 50000;
        std::vector<float> buffer(numFrames, 0.0f);
        waveform.setAudioData(buffer.data(), numFrames, 44100.0);

        MockWaveformCallback cb;
        waveform.setCallback(&cb);

        // Playhead normalized
        waveform.setPlayheadNormalized(0.75f);
        assert(waveform.getPlayheadPosition() == 37500);
        assert(std::abs(waveform.getPlayheadNormalized() - 0.75f) < 1e-4);

        // Loop Range
        waveform.setLoopRange(10000, 30000);
        assert(cb.loopChangedCalled == true);
        assert(waveform.getLoopStartFrame() == 10000);
        assert(waveform.getLoopEndFrame() == 30000);
        waveform.setLoopEnabled(true);
        assert(waveform.isLoopEnabled() == true);

        // Slice Markers
        waveform.addSliceMarker(12000);
        waveform.addSliceMarker(4000);
        waveform.addSliceMarker(25000);

        const std::vector<uint32_t>& slices = waveform.getSliceMarkers();
        assert(slices.size() == 3);
        // Automatic sort order
        assert(slices[0] == 4000);
        assert(slices[1] == 12000);
        assert(slices[2] == 25000);
        (void)slices;

        waveform.clearSliceMarkers();
        assert(waveform.getSliceMarkers().empty());

        std::cout << "  ✓ Normalized playhead positioning, loop region bounds, and sorted slice markers verified" << std::endl;
    }

    std::cout << "--- Testing VectorWaveform Coordinate Conversions & Hit-Testing ---" << std::endl;
    {
        DGL_NAMESPACE::VectorWaveform waveform(&container);
        waveform.setSize(1000, 400);

        const uint32_t numFrames = 100000;
        std::vector<float> buffer(numFrames, 0.0f);
        waveform.setAudioData(buffer.data(), numFrames, 44100.0);

        waveform.setViewport(20000, 60000); // 40000 span across 1000 pixels = 40 frames/px

        // Detail coordinate conversions
        const float pixelX = 500.0f; // Halfway across detail view
        const uint32_t calculatedFrame = waveform.frameFromDetailX(pixelX);
        assert(calculatedFrame == 40000);
        (void)calculatedFrame;

        const float backToPixel = waveform.detailXFromFrame(calculatedFrame);
        assert(std::abs(backToPixel - pixelX) < 1e-3);
        (void)backToPixel;

        // Minimap coordinate conversions (covers full 100000 frames)
        const float minimapX = 250.0f; // 25% of 1000px
        const uint32_t minimapFrame = waveform.frameFromMinimapX(minimapX);
        assert(minimapFrame == 25000);
        (void)minimapFrame;

        std::cout << "  ✓ Bi-directional coordinate mapping (pixel <-> frame) verified" << std::endl;
    }

    // ================================================================================================================
    // Part 2: VectorPlot Architecture Tests
    // ================================================================================================================
    std::cout << "--- Testing VectorPlot Frequency & Decibel Logarithmic Axis Mapping ---" << std::endl;
    {
        DGL_NAMESPACE::VectorPlot plot(&container);
        const float width = 1000.0f;
        const float height = 400.0f;
        plot.setSize(static_cast<uint>(width), static_cast<uint>(height));
        plot.setFreqRange(20.0f, 20000.0f);
        plot.setDbRange(-24.0f, 24.0f);

        // Frequency mapping
        assert(std::abs(plot.freqToX(20.0f) - 0.0f) < 1e-3);
        assert(std::abs(plot.freqToX(20000.0f) - width) < 1e-3);

        // 1000 Hz: log10(1000/20) / log10(20000/20) = 1.69897 / 3.0 = ~0.56632
        const float x1k = plot.freqToX(1000.0f);
        const float freqBack = plot.xToFreq(x1k);
        assert(std::abs(freqBack - 1000.0f) < 0.1f);
        (void)freqBack;

        // Decibel mapping
        assert(std::abs(plot.dbToY(24.0f) - 0.0f) < 1e-3);      // Top
        assert(std::abs(plot.dbToY(-24.0f) - height) < 1e-3);   // Bottom
        assert(std::abs(plot.dbToY(0.0f) - height * 0.5f) < 1e-3); // Center

        const float y6db = plot.dbToY(6.0f);
        const float dbBack = plot.yToDb(y6db);
        assert(std::abs(dbBack - 6.0f) < 1e-3);
        (void)dbBack;

        std::cout << "  ✓ Logarithmic frequency & decibel linear scale roundtrip mapping verified" << std::endl;
    }

    std::cout << "--- Testing VectorPlot Digital Biquad Transfer Functions ---" << std::endl;
    {
        const double sampleRate = 44100.0;

        // 1. Lowpass Filter @ 1000 Hz, Q = 0.707
        DGL_NAMESPACE::VectorPlot::FilterBand lpBand(DGL_NAMESPACE::VectorPlot::kFilterLowpass,
                                                    1000.0f, 0.0f, 0.7071f, DGL_NAMESPACE::Color(80, 180, 255));
        const double lpLow = lpBand.evaluateMagnitudeDb(100.0, sampleRate);
        const double lpCutoff = lpBand.evaluateMagnitudeDb(1000.0, sampleRate);
        const double lpHigh = lpBand.evaluateMagnitudeDb(10000.0, sampleRate);

        assert(std::abs(lpLow - 0.0) < 0.5);      // Passband near 0 dB
        assert(std::abs(lpCutoff - -3.0) < 0.6);  // -3 dB at cutoff
        assert(lpHigh < -20.0);                   // Attenuation at 10 kHz
        (void)lpLow; (void)lpCutoff; (void)lpHigh;

        // 2. Highpass Filter @ 1000 Hz, Q = 0.707
        DGL_NAMESPACE::VectorPlot::FilterBand hpBand(DGL_NAMESPACE::VectorPlot::kFilterHighpass,
                                                    1000.0f, 0.0f, 0.7071f, DGL_NAMESPACE::Color(80, 180, 255));
        const double hpLow = hpBand.evaluateMagnitudeDb(100.0, sampleRate);
        const double hpCutoff = hpBand.evaluateMagnitudeDb(1000.0, sampleRate);
        const double hpHigh = hpBand.evaluateMagnitudeDb(10000.0, sampleRate);

        assert(hpLow < -20.0);                    // Attenuation at 100 Hz
        assert(std::abs(hpCutoff - -3.0) < 0.6);  // -3 dB at cutoff
        assert(std::abs(hpHigh - 0.0) < 0.5);     // Passband near 0 dB
        (void)hpLow; (void)hpCutoff; (void)hpHigh;

        // 3. Peak / Bell Filter @ 2500 Hz, Gain = +6 dB, Q = 1.414
        DGL_NAMESPACE::VectorPlot::FilterBand peakBand(DGL_NAMESPACE::VectorPlot::kFilterPeak,
                                                      2500.0f, 6.0f, 1.414f, DGL_NAMESPACE::Color(255, 120, 0));
        const double peakCenter = peakBand.evaluateMagnitudeDb(2500.0, sampleRate);
        const double peakFar = peakBand.evaluateMagnitudeDb(100.0, sampleRate);

        assert(std::abs(peakCenter - 6.0) < 0.2); // +6 dB at peak
        assert(std::abs(peakFar - 0.0) < 0.2);    // 0 dB far from center
        (void)peakCenter; (void)peakFar;

        // 4. Notch Filter @ 1000 Hz
        DGL_NAMESPACE::VectorPlot::FilterBand notchBand(DGL_NAMESPACE::VectorPlot::kFilterNotch,
                                                       1000.0f, 0.0f, 2.0f, DGL_NAMESPACE::Color(255, 0, 0));
        const double notchCenter = notchBand.evaluateMagnitudeDb(1000.0, sampleRate);
        const double notchAway = notchBand.evaluateMagnitudeDb(5000.0, sampleRate);

        assert(notchCenter < -30.0);              // Deep notch
        assert(std::abs(notchAway - 0.0) < 0.5);
        (void)notchCenter; (void)notchAway;

        std::cout << "  ✓ Biquad transfer equations (Lowpass, Highpass, Peak, Notch) verified" << std::endl;
    }

    std::cout << "--- Testing VectorPlot Multi-Band Composite Evaluation ---" << std::endl;
    {
        DGL_NAMESPACE::VectorPlot plot(&container);
        plot.setSize(800, 300);
        plot.setSampleRate(44100.0);

        // Add 3 bands:
        // Band 1: LowShelf @ 150 Hz, +4 dB
        // Band 2: Peak @ 1000 Hz, -6 dB
        // Band 3: HighShelf @ 8000 Hz, +3 dB
        plot.addBand(DGL_NAMESPACE::VectorPlot::kFilterLowShelf, 150.0f, 4.0f, 0.707f, DGL_NAMESPACE::Color(100, 200, 100));
        plot.addBand(DGL_NAMESPACE::VectorPlot::kFilterPeak, 1000.0f, -6.0f, 1.0f, DGL_NAMESPACE::Color(200, 100, 100));
        plot.addBand(DGL_NAMESPACE::VectorPlot::kFilterHighShelf, 8000.0f, 3.0f, 0.707f, DGL_NAMESPACE::Color(100, 100, 200));

        assert(plot.getBandCount() == 3);

        // Evaluate composite at 50 Hz (dominated by low shelf)
        const double dbLow = plot.evaluateCompositeDb(50.0);
        assert(std::abs(dbLow - 4.0) < 0.5);
        (void)dbLow;

        // Evaluate composite at 1000 Hz (dominated by peak notch)
        const double dbMid = plot.evaluateCompositeDb(1000.0);
        assert(std::abs(dbMid - -6.0) < 0.8);
        (void)dbMid;

        // Evaluate composite at 15000 Hz (dominated by high shelf)
        const double dbHigh = plot.evaluateCompositeDb(15000.0);
        assert(std::abs(dbHigh - 3.0) < 0.5);
        (void)dbHigh;

        // Bypass Band 2 (Peak)
        plot.setBandEnabled(1, false);
        const double dbMidBypassed = plot.evaluateCompositeDb(1000.0);
        assert(std::abs(dbMidBypassed - 0.0) < 0.5); // Should be flat near 0 dB without the dip
        (void)dbMidBypassed;

        std::cout << "  ✓ Multi-band composite curve summation and bypass state verified" << std::endl;
    }

    std::cout << "--- Testing VectorPlot Interactive Node Hit-Testing & Callbacks ---" << std::endl;
    {
        DGL_NAMESPACE::VectorPlot plot(&container);
        const float w = 1000.0f;
        const float h = 400.0f;
        plot.setSize(static_cast<uint>(w), static_cast<uint>(h));
        plot.setFreqRange(20.0f, 20000.0f);
        plot.setDbRange(-24.0f, 24.0f);

        plot.addBand(DGL_NAMESPACE::VectorPlot::kFilterPeak, 1000.0f, 6.0f, 1.0f, DGL_NAMESPACE::Color(255, 180, 0));

        MockPlotCallback cb;
        plot.setCallback(&cb);

        const float bandX = plot.freqToX(1000.0f);
        const float bandY = plot.dbToY(6.0f);

        // Simulate Mouse Press on Band Node
        DGL_NAMESPACE::Widget::MouseEvent pressEv;
        pressEv.button = 1;
        pressEv.press = true;
        pressEv.pos = DGL_NAMESPACE::Point<double>(bandX, bandY);
        const bool handledPress = plot.onMouse(pressEv);
        assert(handledPress == true);
        assert(plot.getSelectedBand() == 0);
        assert(cb.bandSelectedCalled == true);
        (void)handledPress;

        // Simulate Mouse Drag to new frequency and gain
        DGL_NAMESPACE::Widget::MotionEvent dragEv;
        const float targetX = plot.freqToX(3000.0f);
        const float targetY = plot.dbToY(12.0f);
        dragEv.pos = DGL_NAMESPACE::Point<double>(targetX, targetY);
        const bool handledDrag = plot.onMotion(dragEv);
        assert(handledDrag == true);
        assert(cb.bandChangedCalled == true);
        assert(std::abs(plot.getBand(0)->freq - 3000.0f) < 5.0f);
        assert(std::abs(plot.getBand(0)->gainDb - 12.0f) < 0.2f);
        (void)handledDrag;

        // Simulate Mouse Wheel scroll to adjust Q factor
        DGL_NAMESPACE::Widget::ScrollEvent scrollEv;
        scrollEv.delta = DGL_NAMESPACE::Point<double>(0.0, 1.0); // Scroll up
        scrollEv.pos = DGL_NAMESPACE::Point<double>(targetX, targetY);
        const bool handledScroll = plot.onScroll(scrollEv);
        assert(handledScroll == true);
        assert(plot.getBand(0)->q > 1.0f);
        (void)handledScroll;

        std::cout << "  ✓ Interactive node selection, dragging, and Q-factor mouse scroll verified" << std::endl;
    }

    std::cout << "=== All VectorWaveform and VectorPlot Tests Passed Successfully! ===" << std::endl;
    return 0;
}
