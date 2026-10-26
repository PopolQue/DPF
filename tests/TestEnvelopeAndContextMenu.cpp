/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for VectorEnvelope widget & CLAP Context Menu extension
 */

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#include "dgl/VectorWidgets.hpp"
#include "dgl/Application.hpp"
#include "dgl/Window.hpp"
#include "dgl/TopLevelWidget.hpp"
#include "distrho/src/clap/ext/context-menu.h"

USE_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Mock Callbacks for Vector Widgets

class MockEnvelopeCallback : public VectorEnvelope::Callback
{
public:
    int stageChangedCount = 0;
    int dragStartedCount = 0;
    int dragFinishedCount = 0;
    int contextMenuCount = 0;

    int lastStage = -1;
    float lastTime = 0.0f;
    float lastLevel = 0.0f;
    float lastTension = 0.0f;
    int lastContextMenuX = 0;
    int lastContextMenuY = 0;

    void vectorEnvelopeStageChanged(VectorEnvelope*, int stage, float time, float level, float tension) override
    {
        stageChangedCount++;
        lastStage = stage;
        lastTime = time;
        lastLevel = level;
        lastTension = tension;
    }

    void vectorEnvelopeDragStarted(VectorEnvelope*, int handleId) override
    {
        (void)handleId;
        dragStartedCount++;
    }

    void vectorEnvelopeDragFinished(VectorEnvelope*, int handleId) override
    {
        (void)handleId;
        dragFinishedCount++;
    }

    void vectorEnvelopeContextMenu(VectorEnvelope*, int32_t x, int32_t y) override
    {
        contextMenuCount++;
        lastContextMenuX = x;
        lastContextMenuY = y;
    }
};

class MockKnobCallback : public VectorKnob::Callback
{
public:
    int contextMenuCount = 0;
    int lastX = 0;
    int lastY = 0;

    void vectorKnobContextMenu(VectorKnob*, int32_t x, int32_t y) override
    {
        contextMenuCount++;
        lastX = x;
        lastY = y;
    }
};

class MockSliderCallback : public VectorSlider::Callback
{
public:
    int contextMenuCount = 0;
    int lastX = 0;
    int lastY = 0;

    void vectorSliderContextMenu(VectorSlider*, int32_t x, int32_t y) override
    {
        contextMenuCount++;
        lastX = x;
        lastY = y;
    }
};

class MockToggleCallback : public VectorToggleSwitch::Callback
{
public:
    int contextMenuCount = 0;
    int lastX = 0;
    int lastY = 0;

    void vectorToggleSwitchContextMenu(VectorToggleSwitch*, int32_t x, int32_t y) override
    {
        contextMenuCount++;
        lastX = x;
        lastY = y;
    }
};

class MockButtonCallback : public VectorButton::Callback
{
public:
    int contextMenuCount = 0;
    int lastX = 0;
    int lastY = 0;

    void vectorButtonContextMenu(VectorButton*, int32_t x, int32_t y) override
    {
        contextMenuCount++;
        lastX = x;
        lastY = y;
    }
};

// Static C-function pointer context menu tracker
static int sStaticContextMenuCount = 0;
static void testStaticContextMenu(Widget*, int32_t, int32_t, void*)
{
    sStaticContextMenuCount++;
}

// --------------------------------------------------------------------------------------------------------------------
// Mock CLAP Host Context Menu

struct MockClapHostContextMenuState {
    bool popupCalled = false;
    uint32_t targetKind = 0;
    clap_id targetId = 0;
    int32_t screenX = 0;
    int32_t screenY = 0;
};

static MockClapHostContextMenuState sHostMenuState;

static bool CLAP_ABI mockClapHostContextMenuPopup(const clap_host_t*,
                                                   const clap_context_menu_target_t* const target,
                                                   const int32_t screen_x,
                                                   const int32_t screen_y)
{
    if (target == nullptr)
        return false;

    sHostMenuState.popupCalled = true;
    sHostMenuState.targetKind = target->kind;
    sHostMenuState.targetId = target->id;
    sHostMenuState.screenX = screen_x;
    sHostMenuState.screenY = screen_y;
    return true;
}

static const clap_host_context_menu_t sMockClapHostContextMenu = {
    mockClapHostContextMenuPopup
};

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    Application app(true);
    Window win(app);
    TopLevelWidget container(win);

    std::cout << "--- Testing VectorEnvelope Visualizer & Editor ---" << std::endl;
    {
        VectorEnvelope env(&container);
        env.setSize(300, 160);
        MockEnvelopeCallback cb;
        env.setCallback(&cb);
        env.setContextMenuCallback(testStaticContextMenu, nullptr);

        // 1. Initial State & Accessibility
        assert(env.isAccessible());
        assert(env.getAccessibleInfo().role == kAccessibleRoleIndicator);
        assert(std::strcmp(env.getAccessibleInfo().label, "ADSR Envelope") == 0);
        assert(std::strlen(env.getAccessibleInfo().value) > 0);
        assert(!env.isPlayheadActive());

        // 2. Default Parameters
        assert(std::fabs(env.getAttack() - 0.15f) < 0.01f);
        assert(std::fabs(env.getDecay() - 0.20f) < 0.01f);
        assert(std::fabs(env.getSustain() - 0.70f) < 0.01f);
        assert(std::fabs(env.getRelease() - 0.35f) < 0.01f);
        assert(std::fabs(env.getAttackTension() - 0.0f) < 0.01f);
        assert(std::fabs(env.getDecayTension() - (-0.35f)) < 0.01f);

        // 3. Setter & Clamping
        env.setAttack(0.25f, true);
        assert(std::fabs(env.getAttack() - 0.25f) < 0.001f);
        assert(cb.stageChangedCount == 1);
        assert(cb.lastStage == VectorEnvelope::kStageAttack);

        env.setSustain(0.50f, true);
        assert(std::fabs(env.getSustain() - 0.50f) < 0.001f);
        assert(cb.stageChangedCount == 2);

        // Out of bounds clamping
        env.setAttack(-1.0f);
        assert(env.getAttack() >= 0.01f);
        env.setSustain(2.0f);
        assert(env.getSustain() <= 1.0f);
        env.setAttackTension(-2.0f);
        assert(env.getAttackTension() >= -1.0f);
        env.setAttackTension(5.0f);
        assert(env.getAttackTension() <= 1.0f);

        // 4. Millisecond Conversions
        env.setMaxTimesMs(2000.0f, 4000.0f, 6000.0f);
        env.setAttack(0.5f);
        assert(std::fabs(env.getAttackMs() - 1000.0f) < 1.0f);

        env.setDecayMs(2000.0f);
        assert(std::fabs(env.getDecay() - 0.5f) < 0.01f);

        // 5. Playhead & Telemetry
        env.setPlayhead(VectorEnvelope::kStageAttack, 0.4f);
        assert(env.isPlayheadActive());

        env.setPlayheadNormalized(0.85f);
        assert(env.isPlayheadActive());

        VectorEnvelope::PlayheadTelemetry t = { VectorEnvelope::kStageDecay, 0.65f };
        const bool handledTelemetry = env.processTelemetry(100, &t, sizeof(t));
        assert(handledTelemetry);
        (void)handledTelemetry;
        assert(env.isPlayheadActive());

        env.clearPlayhead();
        assert(!env.isPlayheadActive());

        // 6. Right-Click Context Menu Trigger
        sStaticContextMenuCount = 0;
        Widget::MouseEvent rClick;
        rClick.button = kMouseButtonRight;
        rClick.press = true;
        rClick.pos = Point<double>(50.0, 50.0);

        const bool rHandled = env.onMouse(rClick);
        assert(rHandled);
        (void)rHandled;
        assert(cb.contextMenuCount == 1);
        assert(cb.lastContextMenuX == 50);
        assert(cb.lastContextMenuY == 50);
        assert(sStaticContextMenuCount == 1);

        // 7. Keyboard Navigation
        Widget::KeyboardEvent kTab;
        kTab.press = true;
        kTab.key = kKeyTab;
        kTab.mod = 0;
        assert(env.onKeyboard(kTab));

        Widget::KeyboardEvent kUp;
        kUp.press = true;
        kUp.key = kKeyUp;
        kUp.mod = 0;
        assert(env.onKeyboard(kUp));

        std::cout << "VectorEnvelope tests passed successfully!" << std::endl;
    }

    std::cout << "--- Testing CLAP Context Menu Extension Specification ---" << std::endl;
    {
        // 1. Extension Identifiers
        assert(std::strcmp(CLAP_EXT_CONTEXT_MENU, "clap.context-menu") == 0);
        assert(std::strcmp(CLAP_EXT_CONTEXT_MENU_COMPAT, "clap.context-menu.draft/0") == 0);
        assert(CLAP_CONTEXT_MENU_TARGET_KIND_GLOBAL == 0);
        assert(CLAP_CONTEXT_MENU_TARGET_KIND_PARAM == 1);

        // 2. Mock Host Context Menu Invocation
        sHostMenuState = MockClapHostContextMenuState();
        clap_context_menu_target_t target;
        target.kind = CLAP_CONTEXT_MENU_TARGET_KIND_PARAM;
        target.id = 42;

        const bool popupSuccess = sMockClapHostContextMenu.popup(nullptr, &target, 250, 180);
        assert(popupSuccess);
        (void)popupSuccess;
        assert(sHostMenuState.popupCalled);
        assert(sHostMenuState.targetKind == CLAP_CONTEXT_MENU_TARGET_KIND_PARAM);
        assert(sHostMenuState.targetId == 42);
        assert(sHostMenuState.screenX == 250);
        assert(sHostMenuState.screenY == 180);

        // 3. Plugin Context Menu Handler
        clap_plugin_context_menu_t pluginMenu = {
            [](const clap_plugin_t*) -> bool { return true; }
        };
        assert(pluginMenu.can_popup != nullptr);
        assert(pluginMenu.can_popup(nullptr));
        (void)pluginMenu;

        std::cout << "CLAP Context Menu extension tests passed successfully!" << std::endl;
    }

    std::cout << "--- Testing Vector Widgets Right-Click Parameter Context Menus ---" << std::endl;
    {
        // 1. VectorKnob Right-Click Context Menu
        {
            VectorKnob knob(&container);
            knob.setSize(60, 60);
            MockKnobCallback kCb;
            knob.setCallback(&kCb);
            sStaticContextMenuCount = 0;
            knob.setContextMenuCallback(testStaticContextMenu, nullptr);

            Widget::MouseEvent rClick;
            rClick.button = kMouseButtonRight;
            rClick.press = true;
            rClick.pos = Point<double>(30.0, 30.0);

            const bool handled = knob.onMouse(rClick);
            assert(handled);
            (void)handled;
            assert(kCb.contextMenuCount == 1);
            assert(kCb.lastX == 30);
            assert(kCb.lastY == 30);
            assert(sStaticContextMenuCount == 1);
        }

        // 2. VectorSlider Right-Click Context Menu
        {
            VectorSlider slider(&container);
            slider.setSize(24, 120);
            MockSliderCallback sCb;
            slider.setCallback(&sCb);
            sStaticContextMenuCount = 0;
            slider.setContextMenuCallback(testStaticContextMenu, nullptr);

            Widget::MouseEvent rClick;
            rClick.button = kMouseButtonRight;
            rClick.press = true;
            rClick.pos = Point<double>(12.0, 60.0);

            const bool handled = slider.onMouse(rClick);
            assert(handled);
            (void)handled;
            assert(sCb.contextMenuCount == 1);
            assert(sCb.lastX == 12);
            assert(sCb.lastY == 60);
            assert(sStaticContextMenuCount == 1);
        }

        // 3. VectorToggleSwitch Right-Click Context Menu
        {
            VectorToggleSwitch sw(&container);
            sw.setSize(44, 22);
            MockToggleCallback tCb;
            sw.setCallback(&tCb);
            sStaticContextMenuCount = 0;
            sw.setContextMenuCallback(testStaticContextMenu, nullptr);

            Widget::MouseEvent rClick;
            rClick.button = kMouseButtonRight;
            rClick.press = true;
            rClick.pos = Point<double>(20.0, 10.0);

            const bool handled = sw.onMouse(rClick);
            assert(handled);
            (void)handled;
            assert(tCb.contextMenuCount == 1);
            assert(tCb.lastX == 20);
            assert(tCb.lastY == 10);
            assert(sStaticContextMenuCount == 1);
        }

        // 4. VectorButton Right-Click Context Menu
        {
            VectorButton btn(&container);
            btn.setSize(80, 28);
            MockButtonCallback bCb;
            btn.setCallback(&bCb);
            sStaticContextMenuCount = 0;
            btn.setContextMenuCallback(testStaticContextMenu, nullptr);

            Widget::MouseEvent rClick;
            rClick.button = kMouseButtonRight;
            rClick.press = true;
            rClick.pos = Point<double>(40.0, 14.0);

            const bool handled = btn.onMouse(rClick);
            assert(handled);
            (void)handled;
            assert(bCb.contextMenuCount == 1);
            assert(bCb.lastX == 40);
            assert(bCb.lastY == 14);
            assert(sStaticContextMenuCount == 1);
        }

        std::cout << "Vector widgets right-click context menu tests passed successfully!" << std::endl;
    }

    std::cout << "All Phase 12 unit tests passed cleanly!" << std::endl;
    return 0;
}
