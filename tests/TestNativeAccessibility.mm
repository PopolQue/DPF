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

#import <Cocoa/Cocoa.h>
#include <cassert>
#include <cmath>
#include <iostream>

#include "dgl/VectorWidgets.hpp"
#include "dgl/Application.hpp"
#include "dgl/Window.hpp"
#include "dgl/TopLevelWidget.hpp"

USE_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Mock Callbacks for Vector Widgets

class TestKnobCallback : public VectorKnob::Callback
{
public:
    int valueChangedCount = 0;
    float lastValue = 0.0f;

    void vectorKnobValueChanged(VectorKnob*, float value) override
    {
        valueChangedCount++;
        lastValue = value;
    }
};

class TestSliderCallback : public VectorSlider::Callback
{
public:
    int valueChangedCount = 0;
    float lastValue = 0.0f;

    void vectorSliderValueChanged(VectorSlider*, float value) override
    {
        valueChangedCount++;
        lastValue = value;
    }
};

class TestToggleCallback : public VectorToggleSwitch::Callback
{
public:
    int clickedCount = 0;
    bool lastChecked = false;

    void vectorToggleSwitchChanged(VectorToggleSwitch*, bool checked) override
    {
        clickedCount++;
        lastChecked = checked;
    }
};

class TestButtonCallback : public VectorButton::Callback
{
public:
    int clickCount = 0;

    void vectorButtonClicked(VectorButton*, int) override
    {
        clickCount++;
    }
};

static id findChildByLabel(NSArray* children, NSString* label)
{
    for (id child in children)
    {
        if ([[child accessibilityLabel] isEqualToString:label])
            return child;
    }
    return nil;
}

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    @autoreleasepool {
        std::cout << "=== Testing macOS Native Accessibility Bridge (VoiceOver / NSAccessibility) ===" << std::endl;

        Application app(true);
        Window win(app);
        win.setTitle("Native Accessibility Test");
        win.setSize(500, 400);

        TopLevelWidget container(win);
        container.setSize(500, 400);

        // 1. Instantiate vector widgets
        VectorKnob knob(&container);
        knob.setAbsolutePos(20, 20);
        knob.setSize(60, 60);
        knob.setRange(0.0f, 100.0f);
        knob.setDefault(50.0f);
        knob.setValue(50.0f);
        knob.setName("Gain");
        knob.setLabel("Gain");
        TestKnobCallback knobCb;
        knob.setCallback(&knobCb);

        VectorSlider slider(&container);
        slider.setAbsolutePos(100, 20);
        slider.setSize(30, 100);
        slider.setRange(0.0f, 1.0f);
        slider.setStep(0.05f);
        slider.setDefault(0.2f);
        slider.setValue(0.2f);
        slider.setName("Mix");
        slider.setAccessibleLabel("Mix");
        TestSliderCallback sliderCb;
        slider.setCallback(&sliderCb);

        VectorToggleSwitch toggle(&container);
        toggle.setAbsolutePos(150, 20);
        toggle.setSize(40, 24);
        toggle.setChecked(false);
        toggle.setName("Bypass");
        toggle.setAccessibleLabel("Bypass");
        TestToggleCallback toggleCb;
        toggle.setCallback(&toggleCb);

        VectorButton button(&container);
        button.setAbsolutePos(210, 20);
        button.setSize(60, 30);
        button.setName("Trigger");
        button.setAccessibleLabel("Trigger");
        TestButtonCallback buttonCb;
        button.setCallback(&buttonCb);

        VectorEnvelope env(&container);
        env.setAbsolutePos(20, 150);
        env.setSize(200, 100);
        env.setName("Filter Envelope");
        env.setAccessibleLabel("Filter Envelope");

        win.show();

        // 2. Query native Cocoa NSView
        NSView* view = (NSView*)win.getNativeWindowHandle();
        assert(view != nil);
        std::cout << "  ✓ Retrieved native Cocoa NSView handle: " << [view.className UTF8String] << std::endl;

        // Container view itself is not a leaf accessibility element
        assert([view isAccessibilityElement] == NO);
        std::cout << "  ✓ Verified NSView isAccessibilityElement == NO" << std::endl;

        // 3. Query accessibilityChildren
        NSArray* children = [view accessibilityChildren];
        assert(children != nil);
        assert(children.count == 5);
        std::cout << "  ✓ Verified accessibilityChildren count: " << children.count << std::endl;

        id knobElem = findChildByLabel(children, @"Gain");
        id sliderElem = findChildByLabel(children, @"Mix");
        id toggleElem = findChildByLabel(children, @"Bypass");
        id buttonElem = findChildByLabel(children, @"Trigger");
        id envElem = findChildByLabel(children, @"Filter Envelope");

        assert(knobElem != nil);
        assert(sliderElem != nil);
        assert(toggleElem != nil);
        assert(buttonElem != nil);
        assert(envElem != nil);
        (void)envElem;

        // 4. Verify Roles and Role Descriptions
        assert([knobElem accessibilityRole] == NSAccessibilitySliderRole);
        assert([[knobElem accessibilityRoleDescription] isEqualToString:@"knob"]);
        assert([[knobElem accessibilityLabel] isEqualToString:@"Gain"]);
        assert([[knobElem accessibilityValue] isEqual:@(50.0f)]);
        assert([[knobElem accessibilityMinValue] isEqual:@(0.0f)]);
        assert([[knobElem accessibilityMaxValue] isEqual:@(100.0f)]);
        std::cout << "  ✓ Verified VectorKnob NSAccessibilitySliderRole, label, and range" << std::endl;

        assert([sliderElem accessibilityRole] == NSAccessibilitySliderRole);
        assert([[sliderElem accessibilityRoleDescription] isEqualToString:@"slider"]);
        assert([[sliderElem accessibilityLabel] isEqualToString:@"Mix"]);
        assert(std::abs([[sliderElem accessibilityValue] floatValue] - 0.2f) < 0.001f);
        assert([[sliderElem accessibilityMinValue] isEqual:@(0.0f)]);
        assert([[sliderElem accessibilityMaxValue] isEqual:@(1.0f)]);
        std::cout << "  ✓ Verified VectorSlider NSAccessibilitySliderRole, label, and range" << std::endl;

        assert([toggleElem accessibilityRole] == NSAccessibilityCheckBoxRole);
        assert([[toggleElem accessibilityRoleDescription] isEqualToString:@"switch"]);
        assert([[toggleElem accessibilityLabel] isEqualToString:@"Bypass"]);
        assert([[toggleElem accessibilityValue] isEqual:@(0)]);
        std::cout << "  ✓ Verified VectorToggleSwitch NSAccessibilityCheckBoxRole, label, and state" << std::endl;

        assert([buttonElem accessibilityRole] == NSAccessibilityButtonRole);
        assert([[buttonElem accessibilityRoleDescription] isEqualToString:@"button"]);
        assert([[buttonElem accessibilityLabel] isEqualToString:@"Trigger"]);
        assert([buttonElem accessibilityValue] == nil);
        std::cout << "  ✓ Verified VectorButton NSAccessibilityButtonRole and label" << std::endl;

        assert([envElem accessibilityRole] == NSAccessibilityValueIndicatorRole);
        assert([[envElem accessibilityRoleDescription] isEqualToString:@"indicator"]);
        assert([[envElem accessibilityLabel] isEqualToString:@"Filter Envelope"]);
        std::cout << "  ✓ Verified VectorEnvelope NSAccessibilityValueIndicatorRole and label" << std::endl;

        // 5. Test VoiceOver Keyboard Actions
        std::cout << "--- Testing VoiceOver Action Invocations ---" << std::endl;

        // Knob increment: 50 -> 55
        BOOL knobInc = [(id)knobElem accessibilityPerformIncrement];
        assert(knobInc == YES);
        (void)knobInc;
        assert(knobCb.valueChangedCount == 1);
        assert(std::abs(knob.getValue() - 55.0f) < 0.001f);
        assert([[knobElem accessibilityValue] isEqual:@(55.0f)]);

        // Knob decrement: 55 -> 50
        BOOL knobDec = [(id)knobElem accessibilityPerformDecrement];
        assert(knobDec == YES);
        (void)knobDec;
        assert(knobCb.valueChangedCount == 2);
        assert(std::abs(knob.getValue() - 50.0f) < 0.001f);
        assert([[knobElem accessibilityValue] isEqual:@(50.0f)]);
        std::cout << "  ✓ Verified VectorKnob accessibilityPerformIncrement / Decrement" << std::endl;

        // Slider increment: 0.20 -> 0.25
        BOOL sliderInc = [(id)sliderElem accessibilityPerformIncrement];
        assert(sliderInc == YES);
        (void)sliderInc;
        assert(sliderCb.valueChangedCount == 1);
        assert(std::abs(slider.getValue() - 0.25f) < 0.001f);

        // Slider decrement: 0.25 -> 0.20
        BOOL sliderDec = [(id)sliderElem accessibilityPerformDecrement];
        assert(sliderDec == YES);
        (void)sliderDec;
        assert(sliderCb.valueChangedCount == 2);
        assert(std::abs(slider.getValue() - 0.20f) < 0.001f);
        std::cout << "  ✓ Verified VectorSlider accessibilityPerformIncrement / Decrement" << std::endl;

        // Toggle press: false -> true
        BOOL togglePress = [(id)toggleElem accessibilityPerformPress];
        assert(togglePress == YES);
        assert(toggleCb.clickedCount == 1);
        assert(toggleCb.lastChecked == true);
        assert([[toggleElem accessibilityValue] isEqual:@(1)]);

        // Toggle press again: true -> false
        togglePress = [(id)toggleElem accessibilityPerformPress];
        assert(togglePress == YES);
        (void)togglePress;
        assert(toggleCb.clickedCount == 2);
        assert(toggleCb.lastChecked == false);
        assert([[toggleElem accessibilityValue] isEqual:@(0)]);
        std::cout << "  ✓ Verified VectorToggleSwitch accessibilityPerformPress toggle actions" << std::endl;

        // Button press
        BOOL buttonPress = [(id)buttonElem accessibilityPerformPress];
        assert(buttonPress == YES);
        (void)buttonPress;
        assert(buttonCb.clickCount == 1);
        std::cout << "  ✓ Verified VectorButton accessibilityPerformPress action" << std::endl;

        // 6. Test accessibilityHitTest:
        std::cout << "--- Testing VoiceOver accessibilityHitTest ---" << std::endl;
        NSRect knobFrame = [knobElem accessibilityFrame];
        NSPoint knobCenter = NSMakePoint(NSMidX(knobFrame), NSMidY(knobFrame));
        id hitKnob = [view accessibilityHitTest:knobCenter];
        assert(hitKnob == knobElem);
        (void)hitKnob;

        NSRect sliderFrame = [sliderElem accessibilityFrame];
        NSPoint sliderCenter = NSMakePoint(NSMidX(sliderFrame), NSMidY(sliderFrame));
        id hitSlider = [view accessibilityHitTest:sliderCenter];
        assert(hitSlider == sliderElem);
        (void)hitSlider;

        // Outside hit test
        NSPoint outsidePoint = NSMakePoint(-999.0, -999.0);
        id hitOutside = [view accessibilityHitTest:outsidePoint];
        assert(hitOutside == view);
        (void)hitOutside;
        std::cout << "  ✓ Verified accessibilityHitTest point routing" << std::endl;

        // Clean up
        win.close();
        app.quit();

        std::cout << "=== All Native Accessibility Tests Passed Successfully! ===" << std::endl;
        return 0;
    }
}
