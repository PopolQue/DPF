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

#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/AUAudioUnitImplementation.h>
#import <AVFoundation/AVFoundation.h>

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

#define DISTRHO_PLUGIN_INFO_H_INCLUDED 1
#define DISTRHO_PLUGIN_NAME "AUv3 Test Plugin"
#define DISTRHO_PLUGIN_URI "https://distrho.kx.studio/tests/AUv3"
#define DISTRHO_PLUGIN_BRAND_ID Test
#define DISTRHO_PLUGIN_UNIQUE_ID Tst3
#define DISTRHO_PLUGIN_NUM_INPUTS 2
#define DISTRHO_PLUGIN_NUM_OUTPUTS 2
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 1
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 1
#define DISTRHO_PLUGIN_WANT_STATE 1
#define DISTRHO_PLUGIN_WANT_FULL_STATE 1
#define DISTRHO_PLUGIN_WANT_TIMEPOS 1
#define DISTRHO_PLUGIN_VERSION 0x010000
#define DISTRHO_PLUGIN_AU_TYPE aufx
#define DISTRHO_PLUGIN_TARGET_AUV3 1

#include "distrho/DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class MockAUv3Plugin : public Plugin
{
public:
    float fGain;
    float fPan;
    std::string fCustomState;
    uint32_t fProcessCount;

    MockAUv3Plugin()
        : Plugin(2, 0, 1),
          fGain(0.75f),
          fPan(0.0f),
          fCustomState("DefaultState"),
          fProcessCount(0)
    {
    }

    const char* getLabel() const noexcept override { return "AUv3Test"; }
    const char* getDescription() const noexcept override { return "AUv3 Unit Test Plugin"; }
    const char* getMaker() const noexcept override { return "DPF"; }
    const char* getHomePage() const noexcept override { return "https://github.com/DISTRHO/DPF"; }
    const char* getLicense() const noexcept override { return "ISC"; }
    uint32_t getVersion() const noexcept override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const noexcept override { return d_cconst('T', 's', 't', '3'); }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        switch (index)
        {
        case 0:
            parameter.name = "Gain";
            parameter.symbol = "gain";
            parameter.unit = "dB";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 2.0f;
            parameter.ranges.def = 0.75f;
            parameter.hints = kParameterIsAutomable;
            break;
        case 1:
            parameter.name = "Pan";
            parameter.symbol = "pan";
            parameter.ranges.min = -1.0f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 0.0f;
            parameter.hints = kParameterIsAutomable;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index)
        {
        case 0: return fGain;
        case 1: return fPan;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index)
        {
        case 0: fGain = value; break;
        case 1: fPan = value; break;
        }
    }

    void initState(uint32_t index, State& state) override
    {
        if (index == 0)
        {
            state.key = "presetData";
            state.defaultValue = "DefaultState";
        }
    }

    void setState(const char* key, const char* value) override
    {
        if (std::strcmp(key, "presetData") == 0)
            fCustomState = value;
    }

    String getState(const char* key) const override
    {
        if (std::strcmp(key, "presetData") == 0)
            return String(fCustomState.c_str());
        return String();
    }

    void run(const float** inputs, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        ++fProcessCount;

        for (uint32_t s = 0; s < frames; ++s)
        {
            outputs[0][s] = inputs[0][s] * fGain;
            outputs[1][s] = inputs[1][s] * fGain;
        }

        for (uint32_t m = 0; m < midiEventCount; ++m)
        {
            writeMidiEvent(midiEvents[m]);
        }
    }
};

Plugin* createPlugin()
{
    return new MockAUv3Plugin();
}

END_NAMESPACE_DISTRHO

// Include the AUv3 dispatcher implementation directly for this test translation unit
#include "distrho/DistrhoPluginMain.cpp"

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    @autoreleasepool {
        std::cout << "=== Testing AUv3 (Audio Unit v3) App Extension Architecture ===" << std::endl;

        // 1. Dynamic Subclass Registration
        std::cout << "--- Testing Subclass Registration & Component Description ---" << std::endl;
        [DPFAudioUnit registerAudioUnit];
        const AudioComponentDescription desc = [DPFAudioUnit componentDescription];
        assert(desc.componentType == 'aufx');
        assert(desc.componentSubType == 'Tst3');
        assert(desc.componentManufacturer == 'Test');
        std::cout << "  ✓ Registered AUv3 component: type='aufx' subtype='Tst3' manuf='Test'" << std::endl;

        // 2. Component Instantiation
        std::cout << "--- Testing Component Instantiation ---" << std::endl;
        NSError* error = nil;
        DPFAudioUnit* const au = [[DPFAudioUnit alloc] initWithComponentDescription:desc
                                                                           options:0
                                                                             error:&error];
        assert(au != nil);
        assert(error == nil);
        assert(au.pluginExporter != nullptr);
        std::cout << "  ✓ Instantiated DPFAudioUnit successfully" << std::endl;

        // 3. Bus Arrays & Format Verification
        std::cout << "--- Testing Audio Bus Configurations ---" << std::endl;
        assert(au.inputBusses != nil);
        assert(au.inputBusses.count == 1);
        assert(au.inputBusses[0].format.channelCount == 2);
        assert(au.inputBusses[0].format.sampleRate == 44100.0);

        assert(au.outputBusses != nil);
        assert(au.outputBusses.count == 1);
        assert(au.outputBusses[0].format.channelCount == 2);
        assert(au.outputBusses[0].format.sampleRate == 44100.0);
        std::cout << "  ✓ Verified input and output AUAudioUnitBusArray (Stereo 44.1kHz non-interleaved)" << std::endl;

        // 4. AUParameterTree & Parameter Binding
        std::cout << "--- Testing AUParameterTree & Bi-Directional Synchronization ---" << std::endl;
        AUParameterTree* const tree = au.parameterTree;
        assert(tree != nil);
        assert(tree.allParameters.count == 2);

        AUParameter* const gainParam = [tree parameterWithAddress:0];
        assert(gainParam != nil);
        assert([gainParam.identifier isEqualToString:@"gain"]);
        assert([gainParam.name isEqualToString:@"Gain"]);
        assert(gainParam.minValue == 0.0f);
        assert(gainParam.maxValue == 2.0f);
        assert(std::abs(gainParam.value - 0.75f) < 0.001f);

        AUParameter* const panParam = [tree parameterWithAddress:1];
        assert(panParam != nil);
        assert([panParam.identifier isEqualToString:@"pan"]);
        assert(panParam.minValue == -1.0f);
        assert(panParam.maxValue == 1.0f);
        assert(std::abs(panParam.value - 0.0f) < 0.001f);
        (void)panParam;

        // Mutate parameter via AUParameterTree
        gainParam.value = 1.5f;
        assert(std::abs(au.pluginExporter->getParameterValue(0) - 1.5f) < 0.001f);

        // Mutate parameter on PluginExporter directly
        au.pluginExporter->setParameterValue(0, 0.5f);
        assert(std::abs(gainParam.value - 0.5f) < 0.001f);
        std::cout << "  ✓ Verified AUParameterTree creation, addressing, and bi-directional parameter binding" << std::endl;

        // 5. Render Resource Allocation
        std::cout << "--- Testing Render Resource Allocation ---" << std::endl;
        au.maximumFramesToRender = 256;
        const BOOL allocated = [au allocateRenderResourcesAndReturnError:&error];
        assert(allocated == YES);
        assert(error == nil);
        (void)allocated;
        std::cout << "  ✓ allocateRenderResourcesAndReturnError succeeded" << std::endl;

        // 6. Realtime Audio Render Block Execution
        std::cout << "--- Testing AUInternalRenderBlock Audio & Parameter Processing ---" << std::endl;
        AUInternalRenderBlock const renderBlock = au.internalRenderBlock;
        assert(renderBlock != nil);

        constexpr const uint32_t frames = 64;
        float inL[frames], inR[frames];
        for (uint32_t i = 0; i < frames; ++i)
        {
            inL[i] = 1.0f;
            inR[i] = 0.5f;
        }

        struct TwoChannelBufferList {
            AudioBufferList list;
            AudioBuffer extraBuffer;
        };

        TwoChannelBufferList inStorage;
        AudioBufferList* const inList = &inStorage.list;
        inList->mNumberBuffers = 2;
        inList->mBuffers[0].mNumberChannels = 1;
        inList->mBuffers[0].mDataByteSize = frames * sizeof(float);
        inList->mBuffers[0].mData = inL;
        inList->mBuffers[1].mNumberChannels = 1;
        inList->mBuffers[1].mDataByteSize = frames * sizeof(float);
        inList->mBuffers[1].mData = inR;

        float outL[frames] = { 0 };
        float outR[frames] = { 0 };
        TwoChannelBufferList outStorage;
        AudioBufferList* const outList = &outStorage.list;
        outList->mNumberBuffers = 2;
        outList->mBuffers[0].mNumberChannels = 1;
        outList->mBuffers[0].mDataByteSize = frames * sizeof(float);
        outList->mBuffers[0].mData = outL;
        outList->mBuffers[1].mNumberChannels = 1;
        outList->mBuffers[1].mDataByteSize = frames * sizeof(float);
        outList->mBuffers[1].mData = outR;

        AURenderPullInputBlock pullBlock = ^AUAudioUnitStatus(
            AudioUnitRenderActionFlags *actionFlags,
            const AudioTimeStamp *timestamp,
            AUAudioFrameCount frameCount,
            NSInteger inputBusNumber,
            AudioBufferList *inputData)
        {
            (void)actionFlags;
            (void)timestamp;
            (void)inputBusNumber;
            for (uint32_t b = 0; b < inputData->mNumberBuffers && b < inList->mNumberBuffers; ++b)
            {
                std::memcpy(inputData->mBuffers[b].mData, inList->mBuffers[b].mData, frameCount * sizeof(float));
            }
            return noErr;
        };

        // Schedule a realtime parameter event (Gain = 1.25)
        AUParameterEvent paramEv;
        std::memset(&paramEv, 0, sizeof(paramEv));
        paramEv.eventType = AURenderEventParameter;
        paramEv.parameterAddress = 0;
        paramEv.value = 1.25f;

        AudioTimeStamp ts;
        std::memset(&ts, 0, sizeof(ts));
        AudioUnitRenderActionFlags renderFlags = 0;

        const AUAudioUnitStatus status = renderBlock(
            &renderFlags,
            &ts,
            frames,
            0,
            outList,
            (const AURenderEvent*)&paramEv,
            pullBlock
        );
        assert(status == noErr);
        (void)status;

        // Verify output audio was processed by Gain = 1.25
        for (uint32_t i = 0; i < frames; ++i)
        {
            assert(std::abs(outL[i] - 1.25f) < 0.001f);
            assert(std::abs(outR[i] - 0.625f) < 0.001f);
        }
        std::cout << "  ✓ Verified internalRenderBlock processing: inL (1.0) * 1.25 -> 1.25, inR (0.5) * 1.25 -> 0.625" << std::endl;

        // 7. State Archiving & Restoration
        std::cout << "--- Testing State Archiving & Restoration (fullState) ---" << std::endl;
        au.pluginExporter->setState("presetData", "StateAlpha123");
        NSDictionary* const savedState = au.fullState;
        assert(savedState != nil);
        assert(savedState[@"parameters"] != nil);
        assert(savedState[@"states"] != nil);
        assert([savedState[@"states"][@"presetData"] isEqualToString:@"StateAlpha123"]);

        // Mutate state
        au.pluginExporter->setState("presetData", "OverwrittenState");
        assert(std::strcmp(au.pluginExporter->getState("presetData"), "OverwrittenState") == 0);

        // Restore state
        au.fullState = savedState;
        assert(std::strcmp(au.pluginExporter->getState("presetData"), "StateAlpha123") == 0);
        std::cout << "  ✓ Verified fullState export and restoration" << std::endl;

        // 8. Render Resource Deallocation
        std::cout << "--- Testing Render Resource Deallocation ---" << std::endl;
        [au deallocateRenderResources];
        std::cout << "  ✓ deallocateRenderResources succeeded" << std::endl;

        std::cout << "=== All AUv3 Architecture Tests Passed Successfully! ===" << std::endl;
        return 0;
    }
}
