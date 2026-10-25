/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for VectorSpectrum real-time FFT / RTA visualizer
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "dgl/VectorWidgets.hpp"
#include "dgl/Application.hpp"
#include "dgl/Window.hpp"
#include "dgl/TopLevelWidget.hpp"

USE_NAMESPACE_DGL

int main()
{
    Application app(true);
    Window win(app);
    TopLevelWidget container(win);

    std::cout << "--- Testing VectorSpectrum FFT Magnitude Ingestion & Ballistics ---" << std::endl;
    {
        VectorSpectrum spectrum(&container, 128);
        spectrum.setSize(300, 180);

        // 1. Initial State
        assert(spectrum.getBinCount() == 128);
        assert(spectrum.getMinDecibels() == -90.0f);
        assert(spectrum.getMaxDecibels() == 0.0f);

        // Clamped minimum bin count check
        spectrum.setBinCount(8);
        assert(spectrum.getBinCount() == 16);

        spectrum.setBinCount(64);
        assert(spectrum.getBinCount() == 64);

        // Decibel range configuration
        spectrum.setDecibelRange(-100.0f, +6.0f);
        assert(spectrum.getMinDecibels() == -100.0f);
        assert(spectrum.getMaxDecibels() == +6.0f);

        // Reset to standard -90 .. 0 dB
        spectrum.setDecibelRange(-90.0f, 0.0f);

        // 2. Linear Magnitude Ingestion (dB calculation)
        // Bin 0: 1.0 (0 dB)
        // Bin 1: 0.1 (-20 dB)
        // Bin 2: 0.01 (-40 dB)
        // Bin 3: 0.001 (-60 dB)
        // Bin 4: 0.0 (silent -> clamped to minDb -90 dB)
        float linearMags[64] = { 0 };
        linearMags[0] = 1.0f;
        linearMags[1] = 0.1f;
        linearMags[2] = 0.01f;
        linearMags[3] = 0.001f;
        linearMags[4] = 0.0f;

        spectrum.setMagnitudes(linearMags, 64, false);

        assert(std::fabs(spectrum.getCurrentDb(0) - 0.0f) < 0.1f);
        assert(std::fabs(spectrum.getCurrentDb(1) - (-20.0f)) < 0.1f);
        assert(std::fabs(spectrum.getCurrentDb(2) - (-40.0f)) < 0.1f);
        assert(std::fabs(spectrum.getCurrentDb(3) - (-60.0f)) < 0.1f);
        assert(spectrum.getCurrentDb(4) <= -90.0f);

        // Peak values match initial burst
        assert(std::fabs(spectrum.getPeakDb(0) - 0.0f) < 0.1f);
        assert(std::fabs(spectrum.getPeakDb(1) - (-20.0f)) < 0.1f);

        // 3. Telemetry Stream Ingestion (DataRingBuffer packet)
        const uint32_t kTelemetrySpecTag = 0x53504543; // "SPEC"
        float telemetryData[64];
        for (int i = 0; i < 64; ++i)
            telemetryData[i] = -12.0f; // in dB

        bool fed = spectrum.feedTelemetry(kTelemetrySpecTag, kTelemetrySpecTag, telemetryData, sizeof(telemetryData), true);
        (void)fed;
        assert(fed);
        assert(std::fabs(spectrum.getCurrentDb(10) - (-12.0f)) < 0.1f);

        // Mismatched tag should reject
        bool rejected = spectrum.feedTelemetry(0x9999, kTelemetrySpecTag, telemetryData, sizeof(telemetryData), true);
        (void)rejected;
        assert(!rejected);

        // 4. Ballistics Falloff & Peak Hold Decay
        // Set all bins to 0 dB
        float burst[64];
        for (int i = 0; i < 64; ++i)
            burst[i] = 0.0f; // in dB
        spectrum.setMagnitudes(burst, 64, true);

        assert(std::fabs(spectrum.getCurrentDb(0) - 0.0f) < 0.01f);
        assert(std::fabs(spectrum.getPeakDb(0) - 0.0f) < 0.01f);

        // Decay ballistics: default decay rate is 35 dB/s, peak decay is 12 dB/s
        // After 1.0 second:
        // currentDb should be ~ -35 dB
        // peakDb should be ~ -12 dB
        spectrum.updateBallistics(1.0f);

        assert(std::fabs(spectrum.getCurrentDb(0) - (-35.0f)) < 0.5f);
        assert(std::fabs(spectrum.getPeakDb(0) - (-12.0f)) < 0.5f);
        assert(spectrum.getPeakDb(0) > spectrum.getCurrentDb(0));

        // After another 2.0 seconds:
        // currentDb decays further (35 * 2 = 70 dB -> clamped to -90 dB)
        // peakDb decays (12 * 2 = 24 dB -> -36 dB)
        spectrum.updateBallistics(2.0f);
        assert(spectrum.getCurrentDb(0) <= -90.0f);
        assert(std::fabs(spectrum.getPeakDb(0) - (-36.0f)) < 0.5f);

        // Visual properties
        spectrum.setShowGrid(true);
        spectrum.setShowPeakHold(true);
        spectrum.setFillCurve(true);
        spectrum.setTraceColor(Color(0, 200, 255));
        spectrum.setBackgroundColor(Color(10, 12, 16));

        std::cout << "  ✓ VectorSpectrum magnitude scaling, telemetry ingestion, and ballistics decay verified" << std::endl;
    }

    std::cout << "All VectorSpectrum tests passed successfully!" << std::endl;
    return 0;
}
