/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for VectorScope & VectorTabGroup widgets
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

class DummyTabCallback : public VectorTabGroup::Callback
{
public:
    int tabChangedCount = 0;
    uint32_t lastTab = 0;

    void vectorTabChanged(VectorTabGroup*, uint32_t tabIndex) override
    {
        tabChangedCount++;
        lastTab = tabIndex;
    }
};

int main()
{
    Application app(true);
    Window win(app);
    TopLevelWidget container(win);

    std::cout << "--- Testing VectorScope Audio Ingestion & Telemetry ---" << std::endl;
    {
        VectorScope scope(&container, 1024);
        scope.setSize(240, 160);

        // 1. Initial State
        assert(scope.getChannelCount() == 1);
        assert(scope.getTriggerMode() == VectorScope::kTriggerAuto);
        assert(scope.getVerticalScale() == 1.0f);
        assert(scope.getTimebase() == 1.0f);

        // 2. Multi-channel configuration
        scope.setChannelCount(2);
        assert(scope.getChannelCount() == 2);

        // 3. Audio Sample Ingestion (Sine wave)
        float sineWave[256];
        for (int i = 0; i < 256; ++i)
        {
            sineWave[i] = std::sin(2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / 64.0f);
        }
        scope.addSamples(sineWave, 256, 0);

        // Channel 1: inverted sine wave
        float invertedSine[256];
        for (int i = 0; i < 256; ++i)
        {
            invertedSine[i] = -sineWave[i];
        }
        scope.addSamples(invertedSine, 256, 1);

        // 4. Telemetry Stream Ingestion (DataRingBuffer packet)
        const uint32_t kTelemetryScopeTag = 0x53434F50; // "SCOP"
        bool fed = scope.feedTelemetry(kTelemetryScopeTag, kTelemetryScopeTag, sineWave, sizeof(sineWave), 0);
        (void)fed;
        assert(fed);

        // Mismatched tag should reject
        bool rejected = scope.feedTelemetry(0x1234, kTelemetryScopeTag, sineWave, sizeof(sineWave), 0);
        (void)rejected;
        assert(!rejected);

        // 5. Trigger Modes
        scope.setTriggerMode(VectorScope::kTriggerRisingEdge);
        assert(scope.getTriggerMode() == VectorScope::kTriggerRisingEdge);

        scope.setTriggerThreshold(0.25f);
        assert(scope.getTriggerThreshold() == 0.25f);

        scope.setTimebase(2.0f);
        assert(scope.getTimebase() == 2.0f);

        scope.setVerticalScale(1.5f);
        assert(scope.getVerticalScale() == 1.5f);

        // Visual options
        scope.setShowGrid(true);
        scope.setGlow(true);
        scope.setTraceWidth(2.5f);

        std::cout << "  ✓ VectorScope multi-channel buffers, telemetry decoding, and triggers verified" << std::endl;
    }

    std::cout << "--- Testing VectorTabGroup Panel Switching & Keyboard Nav ---" << std::endl;
    {
        VectorTabGroup tabs(&container);
        tabs.setSize(300, 32);

        DummyTabCallback cb;
        tabs.setCallback(&cb);

        assert(tabs.getTabCount() == 0);
        assert(tabs.getCurrentTab() == 0);

        tabs.addTab("Oscillator");
        tabs.addTab("Filter");
        tabs.addTab("Envelopes");
        tabs.addTab("Effects");

        assert(tabs.getTabCount() == 4);
        assert(tabs.getCurrentTab() == 0);
        assert(tabs.getTabLabel(0) == "Oscillator");
        assert(tabs.getTabLabel(1) == "Filter");
        assert(tabs.getTabLabel(2) == "Envelopes");
        assert(tabs.getTabLabel(3) == "Effects");

        // Switch Tab Programmatically with callback
        tabs.setCurrentTab(2, true);
        assert(tabs.getCurrentTab() == 2);
        assert(cb.tabChangedCount == 1);
        assert(cb.lastTab == 2);

        // Keyboard Navigation (Right / Left)
        Widget::KeyboardEvent evRight;
        evRight.press = true;
        evRight.key = kKeyRight;
        tabs.onKeyboard(evRight);
        assert(tabs.getCurrentTab() == 3);
        assert(cb.tabChangedCount == 2);
        assert(cb.lastTab == 3);

        Widget::KeyboardEvent evLeft;
        evLeft.press = true;
        evLeft.key = kKeyLeft;
        tabs.onKeyboard(evLeft);
        assert(tabs.getCurrentTab() == 2);
        assert(cb.tabChangedCount == 3);
        assert(cb.lastTab == 2);

        // Styles
        tabs.setStyle(VectorTabGroup::kStyleUnderline);
        tabs.setStyle(VectorTabGroup::kStylePill);

        std::cout << "  ✓ VectorTabGroup tab collection, switching, and keyboard nav verified" << std::endl;
    }

    std::cout << "VectorScope & VectorTabGroup Unit Tests passed successfully!" << std::endl;
    return 0;
}
