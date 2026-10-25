/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for CLAP Thread Pool Extension, VectorTextEditor & Accessibility Foundation
 */

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>

#include "distrho/src/clap/ext/thread-pool.h"
#include "dgl/VectorWidgets.hpp"
#include "dgl/Application.hpp"
#include "dgl/Window.hpp"
#include "dgl/TopLevelWidget.hpp"

USE_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Mock Plugin with Thread Pool Processing

class MockSynthVoices
{
public:
    std::vector<float> voiceOutputs;
    bool fallbackUsed;

    MockSynthVoices() : voiceOutputs(8, 0.0f), fallbackUsed(false) {}

    void threadPoolExec(const uint32_t taskIndex)
    {
        if (taskIndex < voiceOutputs.size())
        {
            // Simulate voice synthesis computation
            voiceOutputs[taskIndex] = static_cast<float>(taskIndex + 1) * 10.0f;
        }
    }

    bool requestHostThreadPool(const uint32_t numTasks, const clap_host_thread_pool_t* const hostPool, const clap_host_t* const host)
    {
        if (numTasks == 0)
            return true;

        if (hostPool != nullptr && hostPool->request_exec != nullptr)
        {
            fallbackUsed = false;
            return hostPool->request_exec(host, numTasks);
        }

        // Sequential fallback
        fallbackUsed = true;
        for (uint32_t i = 0; i < numTasks; ++i)
            threadPoolExec(i);

        return false;
    }
};

static MockSynthVoices* sMockPlugin = nullptr;

static void CLAP_ABI mock_clap_exec(const clap_plugin_t*, const uint32_t task_index)
{
    if (sMockPlugin != nullptr)
        sMockPlugin->threadPoolExec(task_index);
}

static bool CLAP_ABI mock_host_request_exec(const clap_host_t*, const uint32_t num_tasks)
{
    // Simulate host thread pool distributing jobs to worker threads
    for (uint32_t i = 0; i < num_tasks; ++i)
        mock_clap_exec(nullptr, i);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Mock Text Editor Callback

class MockTextCallback : public VectorTextEditor::Callback
{
public:
    int changedCount = 0;
    int submittedCount = 0;
    int cancelledCount = 0;
    char lastText[64] = { 0 };

    void textEditorChanged(VectorTextEditor*, const char* newText) override
    {
        changedCount++;
        std::strncpy(lastText, newText, sizeof(lastText) - 1);
    }

    void textEditorSubmitted(VectorTextEditor*, const char* text) override
    {
        submittedCount++;
        std::strncpy(lastText, text, sizeof(lastText) - 1);
    }

    void textEditorCancelled(VectorTextEditor*) override
    {
        cancelledCount++;
    }
};

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    std::cout << "--- Testing CLAP Thread Pool Extension Architecture ---" << std::endl;
    {
        MockSynthVoices synth;
        sMockPlugin = &synth;

        // 1. Thread Pool Extension ABI
        clap_plugin_thread_pool_t pluginPool = { mock_clap_exec };
        (void)pluginPool;
        clap_host_thread_pool_t hostPool = { mock_host_request_exec };
        clap_host_t dummyHost = { { 1, 1, 0 }, nullptr, "Mock DAW", "Mock", "", "", nullptr, nullptr };

        // 2. Parallel Host Execution
        bool parallelOk = synth.requestHostThreadPool(8, &hostPool, &dummyHost);
        (void)parallelOk;
        assert(parallelOk);
        assert(!synth.fallbackUsed);
        for (uint32_t i = 0; i < 8; ++i)
            assert(std::fabs(synth.voiceOutputs[i] - static_cast<float>(i + 1) * 10.0f) < 0.001f);

        // Reset
        std::fill(synth.voiceOutputs.begin(), synth.voiceOutputs.end(), 0.0f);

        // 3. Sequential Fallback when host has no thread pool (e.g. VST3, AU, or simple DAWs)
        bool fallbackOk = synth.requestHostThreadPool(8, nullptr, nullptr);
        (void)fallbackOk;
        assert(!fallbackOk); // false signifies sequential fallback was executed
        assert(synth.fallbackUsed);
        for (uint32_t i = 0; i < 8; ++i)
            assert(std::fabs(synth.voiceOutputs[i] - static_cast<float>(i + 1) * 10.0f) < 0.001f);

        std::cout << "  ✓ CLAP thread-pool execution and automatic sequential fallback verified" << std::endl;
    }

    Application app(true);
    Window win(app);
    TopLevelWidget container(win);

    std::cout << "--- Testing Semantic Accessibility (A11y) Architecture ---" << std::endl;
    {
        // 1. Standalone AccessibleInfo Struct
        AccessibleInfo info;
        assert(info.role == kAccessibleRoleUnknown);
        assert(info.isReadOnly == false);

        info.setRole(kAccessibleRoleSlider);
        info.setLabel("Master Volume");
        info.setDescription("Controls main audio output amplitude");
        info.setRange(-60.0f, +6.0f, 0.1f);
        info.setValue(0.0f, "0.0 dB");

        assert(info.role == kAccessibleRoleSlider);
        assert(std::strcmp(info.label, "Master Volume") == 0);
        assert(std::strcmp(info.description, "Controls main audio output amplitude") == 0);
        assert(std::strcmp(info.valueText, "0.0 dB") == 0);
        assert(info.minValue == -60.0f);
        assert(info.maxValue == +6.0f);

        // 2. VectorKnob Accessibility Integration
        VectorKnob knob(&container);
        assert(knob.isAccessible());
        assert(knob.getAccessibleInfo().role == kAccessibleRoleDial);
        knob.setValue(0.75f);
        assert(std::fabs(knob.getAccessibleInfo().currentValue - 0.75f) < 0.01f);

        // 3. VectorSlider Accessibility Integration
        VectorSlider slider(&container, VectorSlider::Horizontal);
        assert(slider.isAccessible());
        assert(slider.getAccessibleInfo().role == kAccessibleRoleSlider);
        slider.setValue(0.42f);
        assert(std::fabs(slider.getAccessibleInfo().currentValue - 0.42f) < 0.01f);

        // 4. VectorButton Accessibility Integration
        VectorButton button(&container);
        assert(button.isAccessible());
        assert(button.getAccessibleInfo().role == kAccessibleRoleButton);

        // 5. VectorComboBox Accessibility Integration
        VectorComboBox combo(&container);
        combo.addItem("Sine");
        combo.addItem("Sawtooth");
        combo.addItem("Square");
        assert(combo.isAccessible());
        assert(combo.getAccessibleInfo().role == kAccessibleRoleComboBox);
        combo.setCurrentIndex(1);
        assert(std::strcmp(combo.getAccessibleInfo().valueText, "Sawtooth") == 0);

        std::cout << "  ✓ Semantic A11y roles, parameters, value formatting, and widget traits verified" << std::endl;
    }

    std::cout << "--- Testing VectorTextEditor Widget & Numeric Entry ---" << std::endl;
    {
        VectorTextEditor editor(&container);
        editor.setSize(120, 28);
        MockTextCallback cb;
        editor.setCallback(&cb);

        // 1. Initial State
        assert(editor.isAccessible());
        assert(editor.getAccessibleInfo().role == kAccessibleRoleTextEditor);
        assert(!editor.isFocused());
        assert(editor.getText().isEmpty());

        editor.setPlaceholder("Enter text...");
        assert(std::strcmp(editor.getPlaceholder().buffer(), "Enter text...") == 0);

        // 2. Direct Text Setting
        editor.setText("440.0", true);
        assert(std::strcmp(editor.getText().buffer(), "440.0") == 0);
        assert(cb.changedCount == 1);
        assert(std::strcmp(cb.lastText, "440.0") == 0);

        // 3. Numeric Mode & Clamping
        editor.setNumericMode(true, 20.0f, 20000.0f, 1);
        assert(editor.isNumericMode());
        assert(std::fabs(editor.getValueAsFloat() - 440.0f) < 0.01f);

        editor.setValueAsFloat(1000.0f);
        assert(std::fabs(editor.getValueAsFloat() - 1000.0f) < 0.01f);

        // 4. Focus & Edit Simulation
        editor.setFocused(true);
        assert(editor.isFocused());
        assert(editor.hasSelection());

        // Keyboard Escape -> Revert & Cancel
        Widget::KeyboardEvent escEv;
        escEv.press = true;
        escEv.key = kKeyEscape;
        bool handledEsc = editor.onKeyboard(escEv);
        (void)handledEsc;
        assert(handledEsc);
        assert(!editor.isFocused());
        assert(cb.cancelledCount == 1);

        // Re-focus and type characters
        editor.setFocused(true);

        Widget::CharacterInputEvent charEv;
        charEv.character = '8';
        editor.onCharacterInput(charEv);
        charEv.character = '8';
        editor.onCharacterInput(charEv);
        charEv.character = '0';
        editor.onCharacterInput(charEv);

        assert(std::strcmp(editor.getText().buffer(), "880") == 0);

        // Keyboard Enter -> Submit
        Widget::KeyboardEvent enterEv;
        enterEv.press = true;
        enterEv.key = kKeyEnter;
        bool handledEnter = editor.onKeyboard(enterEv);
        (void)handledEnter;
        assert(handledEnter);
        assert(!editor.isFocused());
        assert(cb.submittedCount == 1);
        assert(std::fabs(editor.getValueAsFloat() - 880.0f) < 0.01f);

        std::cout << "  ✓ VectorTextEditor typing, numeric filtering, selection, and submit/cancel verified" << std::endl;
    }

    std::cout << "All CLAP Thread Pool, VectorTextEditor, and Accessibility tests passed successfully!" << std::endl;
    return 0;
}
