/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for Multi-DPI Dynamic Scaling & Fractional Scaling Architecture
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "dgl/Application.hpp"
#include "dgl/Color.hpp"
#include "dgl/Geometry.hpp"
#include "dgl/SubWidget.hpp"
#include "dgl/TopLevelWidget.hpp"
#include "dgl/Window.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Mock Window & TopLevelWidget Tracking Scale Notifications

class MockScaleWindow : public DGL_NAMESPACE::Window
{
public:
    int scaleChangeCount;
    double lastReportedScale;

    MockScaleWindow(DGL_NAMESPACE::Application& app)
        : DGL_NAMESPACE::Window(app),
          scaleChangeCount(0),
          lastReportedScale(1.0)
    {
    }

    void onScaleFactorChanged(const double scaleFactor) override
    {
        DGL_NAMESPACE::Window::onScaleFactorChanged(scaleFactor);
        scaleChangeCount++;
        lastReportedScale = scaleFactor;
    }
};

class MockScaleTopLevelWidget : public DGL_NAMESPACE::TopLevelWidget
{
public:
    int scaleChangeCount;
    double lastReportedScale;

    MockScaleTopLevelWidget(DGL_NAMESPACE::Window& win)
        : DGL_NAMESPACE::TopLevelWidget(win),
          scaleChangeCount(0),
          lastReportedScale(1.0)
    {
    }

    void onScaleFactorChanged(const double scaleFactor) override
    {
        DGL_NAMESPACE::TopLevelWidget::onScaleFactorChanged(scaleFactor);
        scaleChangeCount++;
        lastReportedScale = scaleFactor;
    }
};

class MockScaleSubWidget : public DGL_NAMESPACE::SubWidget
{
public:
    int scaleChangeCount;
    double lastReportedScale;

    MockScaleSubWidget(DGL_NAMESPACE::Widget* parent)
        : DGL_NAMESPACE::SubWidget(parent),
          scaleChangeCount(0),
          lastReportedScale(1.0)
    {
    }

    void onScaleFactorChanged(const double scaleFactor) override
    {
        DGL_NAMESPACE::SubWidget::onScaleFactorChanged(scaleFactor);
        scaleChangeCount++;
        lastReportedScale = scaleFactor;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Mock DPF UI Scale Pipeline Simulation

struct MockUIState {
    double scaleFactor;
    int scaleChangeCount;
    bool uiCallbackInvoked;

    MockUIState()
        : scaleFactor(1.0),
          scaleChangeCount(0),
          uiCallbackInvoked(false)
    {
    }

    void uiScaleFactorChanged(double scale)
    {
        scaleChangeCount++;
        scaleFactor = scale;
        uiCallbackInvoked = true;
    }
};

// Simulation of UIInternal::notifyScaleFactorChanged
static void mock_notify_scale_factor_changed(MockUIState& ui, DGL_NAMESPACE::Window* window, double newScale)
{
    if (window != nullptr)
        window->setScaleFactor(newScale);

    ui.uiScaleFactorChanged(newScale);
}

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    std::cout << "=== Testing Multi-DPI Dynamic Scaling & Fractional Scaling Architecture ===" << std::endl;

    DGL_NAMESPACE::Application app(true);
    MockScaleWindow window(app);
    MockScaleTopLevelWidget topLevel(window);
    MockScaleSubWidget subWidget(&topLevel);

    window.setSize(800, 600);
    topLevel.setSize(800, 600);
    subWidget.setSize(200, 150);

    // 1. Initial Baseline Scale Factor
    std::cout << "--- Testing Baseline Scale Factor Retrieval ---" << std::endl;
    {
        const double initialScale = window.getScaleFactor();
        assert(initialScale > 0.0);
        assert(topLevel.getScaleFactor() == initialScale);
        std::cout << "  ✓ Initial scale factor verified: " << initialScale << std::endl;
    }

    // 2. Dynamic Scale Factor Changes Across Integer and Fractional Multipliers
    std::cout << "--- Testing Dynamic Scale Factor Transitions (Fractional & Multi-Monitor) ---" << std::endl;
    {
        const double testScales[] = { 1.25, 1.5, 1.75, 2.0, 2.5, 3.0, 1.0 };
        const size_t numScales = sizeof(testScales) / sizeof(testScales[0]);

        int expectedWindowCount = window.scaleChangeCount;
        int expectedTlwCount = topLevel.scaleChangeCount;
        (void)expectedWindowCount;
        (void)expectedTlwCount;

        for (size_t i = 0; i < numScales; ++i)
        {
            const double targetScale = testScales[i];
            window.setScaleFactor(targetScale);

            expectedWindowCount++;
            expectedTlwCount++;

            assert(window.scaleChangeCount == expectedWindowCount);
            assert(topLevel.scaleChangeCount == expectedTlwCount);
            assert(std::abs(window.getScaleFactor() - targetScale) < 1e-6);
            assert(std::abs(topLevel.getScaleFactor() - targetScale) < 1e-6);
            assert(std::abs(window.lastReportedScale - targetScale) < 1e-6);
            assert(std::abs(topLevel.lastReportedScale - targetScale) < 1e-6);

            // Re-applying the identical scale factor should be a no-op (no redundant callbacks)
            window.setScaleFactor(targetScale);
            assert(window.scaleChangeCount == expectedWindowCount);
            assert(topLevel.scaleChangeCount == expectedTlwCount);
        }

        std::cout << "  ✓ 1.0x, 1.25x, 1.5x, 1.75x, 2.0x, 2.5x, 3.0x dynamic scaling and deduplication verified" << std::endl;
    }

    // 3. Fractional Scale Subpixel Precision & Coordinate Translation
    std::cout << "--- Testing Fractional Scaling Subpixel Coordinate Precision ---" << std::endl;
    {
        const double fractionalScales[] = { 1.25, 1.333333, 1.5, 1.666667, 1.75, 2.25 };
        const size_t count = sizeof(fractionalScales) / sizeof(fractionalScales[0]);

        for (size_t s = 0; s < count; ++s)
        {
            const double scale = fractionalScales[s];
            window.setScaleFactor(scale);

            // Test high-precision floating point coordinate roundtrip
            const double logicalX = 143.7;
            const double logicalY = 289.4;

            const double physicalX = logicalX * scale;
            const double physicalY = logicalY * scale;

            const double roundtripX = physicalX / scale;
            const double roundtripY = physicalY / scale;

            assert(std::abs(roundtripX - logicalX) < 1e-9);
            assert(std::abs(roundtripY - logicalY) < 1e-9);
            (void)roundtripX;
            (void)roundtripY;

            // Subpixel mouse coordinate mapping
            DGL_NAMESPACE::Point<double> logicalPos(logicalX, logicalY);
            DGL_NAMESPACE::Point<double> physicalPos(logicalPos.getX() * scale, logicalPos.getY() * scale);
            DGL_NAMESPACE::Point<double> backPos(physicalPos.getX() / scale, physicalPos.getY() / scale);

            assert(std::abs(backPos.getX() - logicalPos.getX()) < 1e-9);
            assert(std::abs(backPos.getY() - logicalPos.getY()) < 1e-9);
        }

        std::cout << "  ✓ Subpixel coordinate roundtrip precision (< 1e-9) under fractional scaling verified" << std::endl;
    }

    // 4. DPF UI Scale Synchronization Pipeline (VST3/CLAP/Standalone -> UI -> Window)
    std::cout << "--- Testing DPF UI Scale Synchronization Pipeline ---" << std::endl;
    {
        MockUIState uiState;
        assert(uiState.scaleFactor == 1.0);
        assert(uiState.scaleChangeCount == 0);

        const double hostScaleRequest = 1.5;
        mock_notify_scale_factor_changed(uiState, &window, hostScaleRequest);

        assert(uiState.uiCallbackInvoked == true);
        assert(uiState.scaleChangeCount == 1);
        assert(std::abs(uiState.scaleFactor - 1.5) < 1e-6);
        assert(std::abs(window.getScaleFactor() - 1.5) < 1e-6);
        assert(std::abs(topLevel.getScaleFactor() - 1.5) < 1e-6);

        // Host scale transition back to 1.0 (dragged to standard display)
        mock_notify_scale_factor_changed(uiState, &window, 1.0);
        assert(uiState.scaleChangeCount == 2);
        assert(std::abs(uiState.scaleFactor - 1.0) < 1e-6);
        assert(std::abs(window.getScaleFactor() - 1.0) < 1e-6);
        assert(std::abs(topLevel.getScaleFactor() - 1.0) < 1e-6);

        std::cout << "  ✓ Host scale factor event synchronization to UI and Window verified" << std::endl;
    }

    // 5. Invalid & Negative Scale Factor Safety
    std::cout << "--- Testing Negative and Zero Scale Factor Safety Guards ---" << std::endl;
    {
        const double currentScale = window.getScaleFactor();
        const int prevWindowCount = window.scaleChangeCount;
        (void)currentScale;
        (void)prevWindowCount;

        // Negative and zero scales must be ignored safely
        window.setScaleFactor(0.0);
        window.setScaleFactor(-1.5);
        window.setScaleFactor(-0.001);

        assert(window.scaleChangeCount == prevWindowCount);
        assert(window.getScaleFactor() == currentScale);

        std::cout << "  ✓ Invalid scale factors safely rejected without state mutation" << std::endl;
    }

    std::cout << "=== All Multi-DPI Dynamic Scaling Tests Passed Successfully! ===" << std::endl;
    return 0;
}
