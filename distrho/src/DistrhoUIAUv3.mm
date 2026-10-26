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

#define Point AudioUnitPoint
#define Size AudioUnitSize

#import <CoreAudioKit/CoreAudioKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Cocoa/Cocoa.h>

#undef Point
#undef Size

#include <mach/mach_time.h>
#include "DistrhoUIInternal.hpp"

#ifndef DISTRHO_PLUGIN_BRAND_ID
# error DISTRHO_PLUGIN_BRAND_ID undefined!
#endif

#ifndef DISTRHO_PLUGIN_UNIQUE_ID
# error DISTRHO_PLUGIN_UNIQUE_ID undefined!
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif

// unsupported in AUv3
static constexpr const fileRequestFunc fileRequestCallback = nullptr;

// --------------------------------------------------------------------------------------------------------------------
// DPF_UI_AUv3 Core Bridge

class DPF_UI_AUv3
{
public:
    DPF_UI_AUv3(AUAudioUnit* const audioUnit,
                NSView* const view,
                const double sampleRate)
        : fAudioUnit(audioUnit),
          fParentView(view),
          fTimerRef(nullptr),
          fObserverToken(nullptr),
          fUI(this,
              reinterpret_cast<uintptr_t>(view),
              sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              fileRequestCallback,
              nullptr,
              nullptr)
    {
        // Observe parameter changes from host/DSP
        if (fAudioUnit != nil && fAudioUnit.parameterTree != nil)
        {
            DPF_UI_AUv3* const weakSelf = this;
            fObserverToken = [fAudioUnit.parameterTree tokenByAddingParameterObserver:^(AUParameterAddress address, AUValue value) {
                if (weakSelf != nullptr)
                    weakSelf->fUI.parameterChanged((uint32_t)address, value);
            }];
        }

        // Setup periodic idle timer
        constexpr const CFTimeInterval interval = 60 * 0.0001;
        CFRunLoopTimerContext context = {};
        context.info = this;
        fTimerRef = CFRunLoopTimerCreate(kCFAllocatorDefault,
                                         CFAbsoluteTimeGetCurrent() + interval,
                                         interval, 0, 0,
                                         _idleCallback, &context);
        if (fTimerRef != nullptr)
            CFRunLoopAddTimer(CFRunLoopGetCurrent(), fTimerRef, kCFRunLoopCommonModes);
    }

    ~DPF_UI_AUv3()
    {
        if (fObserverToken != nullptr && fAudioUnit != nil && fAudioUnit.parameterTree != nil)
        {
            [fAudioUnit.parameterTree removeParameterObserver:fObserverToken];
            fObserverToken = nullptr;
        }

        if (fTimerRef != nullptr)
        {
            CFRunLoopTimerInvalidate(fTimerRef);
            CFRelease(fTimerRef);
            fTimerRef = nullptr;
        }
    }

    void idle()
    {
        fUI.idleFromNativeIdle();
    }

    void editParameter(const uint32_t index, const bool started) const
    {
        if (fAudioUnit != nil && fAudioUnit.parameterTree != nil)
        {
            AUParameter* const p = [fAudioUnit.parameterTree parameterWithAddress:index];
            if (p != nil)
            {
                const AUParameterAutomationEventType eventType = started
                    ? AUParameterAutomationEventTypeTouch
                    : AUParameterAutomationEventTypeRelease;
                [p setValue:p.value originator:fObserverToken atHostTime:mach_absolute_time() eventType:eventType];
            }
        }
    }

    void setParameter(const uint32_t index, const float value)
    {
        if (fAudioUnit != nil && fAudioUnit.parameterTree != nil)
        {
            AUParameter* const p = [fAudioUnit.parameterTree parameterWithAddress:index];
            if (p != nil)
                [p setValue:value originator:fObserverToken];
        }
    }

#if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        (void)key;
        (void)value;
    }
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        (void)channel;
        (void)note;
        (void)velocity;
    }
#endif

    void setSize(const uint width, const uint height)
    {
        if (fParentView != nil)
            [fParentView setFrameSize:NSMakeSize(width, height)];
    }

private:
    AUAudioUnit* const fAudioUnit;
    NSView* const fParentView;
    CFRunLoopTimerRef fTimerRef;
    AUParameterObserverToken fObserverToken;
    UIExporter fUI;

    static void _idleCallback(CFRunLoopTimerRef, void* const info)
    {
        static_cast<DPF_UI_AUv3*>(info)->idle();
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<DPF_UI_AUv3*>(ptr)->editParameter(rindex, started);
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<DPF_UI_AUv3*>(ptr)->setParameter(rindex, value);
    }

#if DISTRHO_PLUGIN_WANT_STATE
    static void setStateCallback(void* const ptr, const char* const key, const char* const value)
    {
        static_cast<DPF_UI_AUv3*>(ptr)->setState(key, value);
    }
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<DPF_UI_AUv3*>(ptr)->sendNote(channel, note, velocity);
    }
#endif

    static void setSizeCallback(void* const ptr, const uint width, const uint height)
    {
        static_cast<DPF_UI_AUv3*>(ptr)->setSize(width, height);
    }
};

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// DPFAudioUnitViewController

@interface DPFAudioUnitViewController : AUViewController <AUAudioUnitFactory>
{
    AUAudioUnit* _audioUnit;
    DISTRHO_NAMESPACE::DPF_UI_AUv3* _dpfUI;
}

- (instancetype)initWithAudioUnit:(AUAudioUnit*)audioUnit;

@end

@implementation DPFAudioUnitViewController

- (instancetype)initWithAudioUnit:(AUAudioUnit*)audioUnit
{
    if ((self = [super initWithNibName:nil bundle:nil]))
    {
        _audioUnit = audioUnit;
        _dpfUI = nullptr;
    }
    return self;
}

- (void)dealloc
{
    if (_dpfUI != nullptr)
    {
        delete _dpfUI;
        _dpfUI = nullptr;
    }
    [super dealloc];
}

- (void)loadView
{
#if defined(DISTRHO_UI_DEFAULT_WIDTH) && defined(DISTRHO_UI_DEFAULT_HEIGHT)
    const CGFloat width = (CGFloat)DISTRHO_UI_DEFAULT_WIDTH;
    const CGFloat height = (CGFloat)DISTRHO_UI_DEFAULT_HEIGHT;
#else
    const CGFloat width = 400.0;
    const CGFloat height = 300.0;
#endif
    self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    if (_dpfUI == nullptr && _audioUnit != nil)
    {
        double sampleRate = 44100.0;
        if (_audioUnit.outputBusses.count > 0 && _audioUnit.outputBusses[0].format != nil)
            sampleRate = _audioUnit.outputBusses[0].format.sampleRate;

        _dpfUI = new DISTRHO_NAMESPACE::DPF_UI_AUv3(_audioUnit, self.view, sampleRate);
    }
}

- (AUAudioUnit *)createAudioUnitWithComponentDescription:(AudioComponentDescription)desc error:(NSError **)error
{
    Class auClass = NSClassFromString(@"DPFAudioUnit");
    if (auClass != nil)
    {
        _audioUnit = [[auClass alloc] initWithComponentDescription:desc options:0 error:error];
        return _audioUnit;
    }
    return nil;
}

@end
