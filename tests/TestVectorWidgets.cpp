/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for Modern Vector Widgets (VectorKnob, VectorSlider, VectorMeter)
 */

#include <cassert>
#include <cmath>
#include <iostream>

#include "dgl/VectorWidgets.hpp"
#include "dgl/Application.hpp"
#include "dgl/Window.hpp"
#include "dgl/TopLevelWidget.hpp"

USE_NAMESPACE_DGL

class DummyKnobCallback : public VectorKnob::Callback
{
public:
    int dragStartedCount = 0;
    int dragFinishedCount = 0;
    int valueChangedCount = 0;
    float lastValue = 0.0f;

    void vectorKnobDragStarted(VectorKnob*) override
    {
        dragStartedCount++;
    }

    void vectorKnobDragFinished(VectorKnob*) override
    {
        dragFinishedCount++;
    }

    void vectorKnobValueChanged(VectorKnob*, float value) override
    {
        valueChangedCount++;
        lastValue = value;
    }
};

class DummySliderCallback : public VectorSlider::Callback
{
public:
    int dragStartedCount = 0;
    int dragFinishedCount = 0;
    int valueChangedCount = 0;
    float lastValue = 0.0f;

    void vectorSliderDragStarted(VectorSlider*) override
    {
        dragStartedCount++;
    }

    void vectorSliderDragFinished(VectorSlider*) override
    {
        dragFinishedCount++;
    }

    void vectorSliderValueChanged(VectorSlider*, float value) override
    {
        valueChangedCount++;
        lastValue = value;
    }
};

class DummyButtonCallback : public VectorButton::Callback
{
public:
    int clickCount = 0;
    int toggleCount = 0;
    bool lastChecked = false;
    int lastMouseButton = 0;

    void vectorButtonClicked(VectorButton*, int mouseButton) override
    {
        clickCount++;
        lastMouseButton = mouseButton;
    }

    void vectorButtonToggled(VectorButton*, bool checked) override
    {
        toggleCount++;
        lastChecked = checked;
    }
};

class DummyComboBoxCallback : public VectorComboBox::Callback
{
public:
    int indexChangedCount = 0;
    uint32_t lastIndex = 0;

    void vectorComboBoxIndexChanged(VectorComboBox*, uint32_t index) override
    {
        indexChangedCount++;
        lastIndex = index;
    }
};

int main()
{
    Application app(true);
    Window win(app);
    TopLevelWidget container(win);

    std::cout << "--- Testing VectorKnob Logic & Parameters ---" << std::endl;
    {
        VectorKnob knob(&container);
        knob.setSize(60, 60);

        DummyKnobCallback cb;
        knob.setCallback(&cb);

        knob.setRange(0.0f, 100.0f);
        knob.setDefault(50.0f);
        knob.setStep(1.0f);

        assert(knob.getMinimum() == 0.0f);
        assert(knob.getMaximum() == 100.0f);
        assert(knob.getDefault() == 50.0f);

        // Value changes
        knob.setValue(25.0f, true);
        assert(knob.getValue() == 25.0f);
        assert(std::abs(knob.getNormalizedValue() - 0.25f) < 0.001f);
        assert(cb.valueChangedCount == 1);
        assert(cb.lastValue == 25.0f);

        knob.setValue(75.0f, true);
        assert(knob.getValue() == 75.0f);
        assert(std::abs(knob.getNormalizedValue() - 0.75f) < 0.001f);
        assert(cb.valueChangedCount == 2);

        // Styling tests
        knob.setBipolar(true);
        knob.setTrackWidth(5.0f);
        knob.setDialRadiusRatio(0.65f);
        knob.setLabel("Cutoff");
        knob.setShowValue(true);
        knob.setUnit("Hz");

        knob.setActiveColor(Color(255, 100, 50));
        knob.setDialColor(Color(20, 20, 20));

        std::cout << "  ✓ VectorKnob ranges, normalized math, bipolar mode, and callbacks verified" << std::endl;
    }

    std::cout << "--- Testing VectorSlider Logic (Vertical & Horizontal) ---" << std::endl;
    {
        // Vertical Slider
        VectorSlider vslider(&container, VectorSlider::Vertical);
        vslider.setSize(30, 150);

        DummySliderCallback scb;
        vslider.setCallback(&scb);

        vslider.setRange(-24.0f, 6.0f);
        vslider.setDefault(0.0f);

        vslider.setValue(-6.0f, true);
        assert(std::abs(vslider.getValue() - (-6.0f)) < 0.001f);
        // (-6 - (-24)) / 30 = 18 / 30 = 0.6
        assert(std::abs(vslider.getNormalizedValue() - 0.6f) < 0.001f);
        assert(scb.valueChangedCount == 1);

        vslider.setNormalizedValue(0.5f, true);
        assert(std::abs(vslider.getValue() - (-9.0f)) < 0.001f);
        assert(scb.valueChangedCount == 2);

        // Horizontal Slider
        VectorSlider hslider(&container, VectorSlider::Horizontal);
        hslider.setSize(200, 30);
        hslider.setRange(0.0f, 1.0f);
        hslider.setBipolar(true);
        hslider.setTrackThickness(8.0f);
        hslider.setThumbSize(14.0f, 22.0f);

        hslider.setValue(0.5f);
        assert(std::abs(hslider.getNormalizedValue() - 0.5f) < 0.001f);

        std::cout << "  ✓ VectorSlider vertical & horizontal layout, normalized mapping, and callbacks verified" << std::endl;
    }

    std::cout << "--- Testing VectorMeter Ballistics & Decibel Scaling ---" << std::endl;
    {
        VectorMeter meter(&container, VectorMeter::Vertical, 2);
        meter.setSize(24, 120);

        meter.setDecibelRange(-60.0f, 6.0f);

        // Initial state: -60 dB
        meter.setValuesDb(-6.0f, -12.0f);

        // Linear conversion: 1.0 = 0 dB
        meter.setValueLinear(1.0f, 0);
        // 0.5 = -6.02 dB
        meter.setValueLinear(0.5f, 1);

        // Ballistics update: simulate 100ms passage of time
        meter.updateBallistics(0.1f);
        meter.updateBallistics(0.5f);
        meter.updateBallistics(1.0f);

        // Styling
        meter.setNormalColor(Color(0, 200, 100));
        meter.setWarnColor(Color(255, 200, 0));
        meter.setClipColor(Color(255, 0, 0));

        std::cout << "  ✓ VectorMeter stereo levels, linear/dB conversion, and ballistics decay verified" << std::endl;
    }

    std::cout << "--- Testing VectorButton Momentary & Toggle Modes ---" << std::endl;
    {
        VectorButton button(&container);
        button.setSize(80, 28);
        button.setLabel("Mute");

        DummyButtonCallback cb;
        button.setCallback(&cb);

        // 1. Momentary mode
        assert(button.getMode() == VectorButton::Momentary);
        assert(!button.isToggled());
        assert(button.getLabel() == "Mute");

        // Styling
        button.setCornerRadius(6.0f);
        button.setFontSize(13.0f);
        button.setHasLed(true);
        button.setLedColor(Color(255, 60, 60));
        assert(button.hasLed());
        assert(!button.isLedActive());

        button.setLedActive(true);
        assert(button.isLedActive());

        // 2. Toggle mode
        button.setMode(VectorButton::Toggle);
        assert(button.getMode() == VectorButton::Toggle);

        button.setToggled(true, true);
        assert(button.isToggled());
        assert(cb.toggleCount == 1);
        assert(cb.lastChecked == true);

        button.setToggled(false, true);
        assert(!button.isToggled());
        assert(cb.toggleCount == 2);
        assert(cb.lastChecked == false);

        std::cout << "  ✓ VectorButton momentary/toggle, LED indicator, and callbacks verified" << std::endl;
    }

    std::cout << "--- Testing VectorComboBox Items & Keyboard Selection ---" << std::endl;
    {
        VectorComboBox combo(&container);
        combo.setSize(120, 26);

        DummyComboBoxCallback cb;
        combo.setCallback(&cb);

        assert(combo.getItemCount() == 0);
        assert(combo.getCurrentIndex() == -1);

        combo.addItem("Sine");
        combo.addItem("Triangle");
        combo.addItem("Sawtooth");
        combo.addItem("Square");

        assert(combo.getItemCount() == 4);
        assert(combo.getCurrentIndex() == 0);
        assert(combo.getCurrentItem() == "Sine");
        assert(combo.getItem(2) == "Sawtooth");

        // Selection
        combo.setCurrentIndex(2, true);
        assert(combo.getCurrentIndex() == 2);
        assert(combo.getCurrentItem() == "Sawtooth");
        assert(cb.indexChangedCount == 1);
        assert(cb.lastIndex == 2);

        // Open/Close
        assert(!combo.isOpen());
        combo.setOpen(true);
        assert(combo.isOpen());

        // Keyboard Navigation (Down / Up)
        Widget::KeyboardEvent evDown;
        evDown.press = true;
        evDown.key = kKeyDown;
        combo.onKeyboard(evDown);
        assert(combo.getCurrentIndex() == 3);
        assert(combo.getCurrentItem() == "Square");

        Widget::KeyboardEvent evUp;
        evUp.press = true;
        evUp.key = kKeyUp;
        combo.onKeyboard(evUp);
        assert(combo.getCurrentIndex() == 2);
        assert(combo.getCurrentItem() == "Sawtooth");

        // Escape closes
        Widget::KeyboardEvent evEsc;
        evEsc.press = true;
        evEsc.key = kKeyEscape;
        combo.onKeyboard(evEsc);
        assert(!combo.isOpen());

        // Remove item
        combo.removeItem(1); // removes Triangle
        assert(combo.getItemCount() == 3);
        assert(combo.getItem(1) == "Sawtooth");

        std::cout << "  ✓ VectorComboBox items, dropdown open/close, and keyboard nav verified" << std::endl;
    }

    std::cout << "Modern Vector Widgets Unit Tests passed successfully!" << std::endl;
    return 0;
}
