/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for VectorXYPad & VectorToggleSwitch widgets
 */

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#include "dgl/VectorWidgets.hpp"
#include "dgl/Application.hpp"
#include "dgl/Window.hpp"
#include "dgl/TopLevelWidget.hpp"

USE_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Mock Callbacks

class MockXYPadCallback : public VectorXYPad::Callback
{
public:
    int dragStartedCount = 0;
    int dragFinishedCount = 0;
    int valueChangedCount = 0;
    float lastX = 0.0f;
    float lastY = 0.0f;

    void vectorXYPadDragStarted(VectorXYPad*) override
    {
        dragStartedCount++;
    }

    void vectorXYPadDragFinished(VectorXYPad*) override
    {
        dragFinishedCount++;
    }

    void vectorXYPadValueChanged(VectorXYPad*, float x, float y) override
    {
        valueChangedCount++;
        lastX = x;
        lastY = y;
    }
};

class MockToggleCallback : public VectorToggleSwitch::Callback
{
public:
    int changedCount = 0;
    bool lastChecked = false;

    void vectorToggleSwitchChanged(VectorToggleSwitch*, bool checked) override
    {
        changedCount++;
        lastChecked = checked;
    }
};

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    Application app(true);
    Window win(app);
    TopLevelWidget container(win);

    std::cout << "--- Testing VectorXYPad 2D Modulation Surface ---" << std::endl;
    {
        VectorXYPad pad(&container);
        pad.setSize(200, 200);
        MockXYPadCallback cb;
        pad.setCallback(&cb);

        // 1. Initial State
        assert(pad.isAccessible());
        assert(pad.getAccessibleInfo().role == kAccessibleRoleIndicator);
        assert(std::fabs(pad.getNormalizedX() - 0.5f) < 0.01f);
        assert(std::fabs(pad.getNormalizedY() - 0.5f) < 0.01f);
        assert(!pad.isSpringReturn());

        // 2. Custom Ranges
        pad.setRangeX(-1.0f, +1.0f);
        pad.setRangeY(20.0f, 20000.0f);
        pad.setDefaultValues(0.0f, 1000.0f);

        pad.setValues(0.0f, 1000.0f, true);
        assert(std::fabs(pad.getX() - 0.0f) < 0.001f);
        assert(std::fabs(pad.getY() - 1000.0f) < 0.001f);
        assert(cb.valueChangedCount == 1);
        assert(std::fabs(cb.lastX - 0.0f) < 0.001f);
        assert(std::fabs(cb.lastY - 1000.0f) < 0.001f);

        // Normalized mapping check
        pad.setNormalizedValues(1.0f, 1.0f, true);
        assert(std::fabs(pad.getX() - 1.0f) < 0.001f);
        assert(std::fabs(pad.getY() - 20000.0f) < 0.001f);

        // 3. Mouse Interaction & Spring Return
        pad.setSpringReturn(true);
        assert(pad.isSpringReturn());

        // Mouse Press at (150, 50) -> start drag
        Widget::MouseEvent pressEv;
        pressEv.button = kMouseButtonLeft;
        pressEv.press = true;
        pressEv.pos = Point<double>(150.0, 50.0);
        bool handledPress = pad.onMouse(pressEv);
        (void)handledPress;
        assert(handledPress);
        assert(cb.dragStartedCount == 1);

        // Mouse Release -> snap to default (0.0, 1000.0)
        Widget::MouseEvent releaseEv;
        releaseEv.button = kMouseButtonLeft;
        releaseEv.press = false;
        releaseEv.pos = Point<double>(150.0, 50.0);
        bool handledRelease = pad.onMouse(releaseEv);
        (void)handledRelease;
        assert(handledRelease);
        assert(cb.dragFinishedCount == 1);
        assert(std::fabs(pad.getX() - 0.0f) < 0.001f);
        assert(std::fabs(pad.getY() - 1000.0f) < 0.001f);

        // Visual properties
        pad.setBipolar(true, false);
        pad.setShowCrosshairs(true);
        pad.setShowGrid(true);
        pad.setShowTrails(true);
        pad.setHandleColor(Color(255, 100, 50));

        std::cout << "  ✓ VectorXYPad 2D coordinates, normalized mapping, spring return, and drag events verified" << std::endl;
    }

    std::cout << "--- Testing VectorToggleSwitch Mechanical Switch ---" << std::endl;
    {
        VectorToggleSwitch sw(&container, VectorToggleSwitch::Horizontal);
        sw.setSize(60, 28);
        MockToggleCallback cb;
        sw.setCallback(&cb);

        // 1. Initial State
        assert(sw.isAccessible());
        assert(sw.getAccessibleInfo().role == kAccessibleRoleCheckBox);
        assert(!sw.isChecked());
        assert(sw.getOrientation() == VectorToggleSwitch::Horizontal);
        assert(std::strcmp(sw.getAccessibleInfo().valueText, "Off") == 0);

        // 2. Direct Toggle
        sw.toggle(true);
        assert(sw.isChecked());
        assert(cb.changedCount == 1);
        assert(cb.lastChecked == true);
        assert(std::strcmp(sw.getAccessibleInfo().valueText, "On") == 0);

        sw.toggle(true);
        assert(!sw.isChecked());
        assert(cb.changedCount == 2);
        assert(cb.lastChecked == false);
        assert(std::strcmp(sw.getAccessibleInfo().valueText, "Off") == 0);

        // 3. Mouse Click Toggle
        Widget::MouseEvent clickEv;
        clickEv.button = kMouseButtonLeft;
        clickEv.press = true;
        bool handledClick = sw.onMouse(clickEv);
        (void)handledClick;
        assert(handledClick);
        assert(sw.isChecked());
        assert(cb.changedCount == 3);

        // 4. Keyboard Navigation (Space key)
        Widget::KeyboardEvent spaceEv;
        spaceEv.press = true;
        spaceEv.key = kKeySpace;
        bool handledSpace = sw.onKeyboard(spaceEv);
        (void)handledSpace;
        assert(handledSpace);
        assert(!sw.isChecked());
        assert(cb.changedCount == 4);

        // 5. Vertical Orientation
        sw.setOrientation(VectorToggleSwitch::Vertical);
        assert(sw.getOrientation() == VectorToggleSwitch::Vertical);

        std::cout << "  ✓ VectorToggleSwitch horizontal/vertical layouts, mouse/keyboard triggers, and state verified" << std::endl;
    }

    std::cout << "--- Testing Full Suite Accessibility (A11y) Conformance ---" << std::endl;
    {
        VectorTabGroup tabs(&container);
        tabs.addTab("Oscillator");
        tabs.addTab("Filter");
        assert(tabs.isAccessible());
        assert(tabs.getAccessibleInfo().role == kAccessibleRoleTab);
        tabs.setCurrentTab(1);
        assert(std::strcmp(tabs.getAccessibleInfo().valueText, "Filter") == 0);

        VectorMeter meter(&container);
        assert(meter.isAccessible());
        assert(meter.getAccessibleInfo().role == kAccessibleRoleIndicator);
        meter.setValueDb(-12.0f, 0);
        assert(std::fabs(meter.getAccessibleInfo().currentValue - (-12.0f)) < 0.1f);

        std::cout << "  ✓ Suite-wide AccessibleElement conformance verified" << std::endl;
    }

    std::cout << "All VectorXYPad and VectorToggleSwitch tests passed successfully!" << std::endl;
    return 0;
}
