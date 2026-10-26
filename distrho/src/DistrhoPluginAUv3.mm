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
#import <CoreAudioKit/CoreAudioKit.h>

#include "DistrhoPluginInternal.hpp"
#include "../DistrhoPluginUtils.hpp"

#ifndef DISTRHO_PLUGIN_BRAND_ID
# error DISTRHO_PLUGIN_BRAND_ID undefined!
#endif

#ifndef DISTRHO_PLUGIN_UNIQUE_ID
# error DISTRHO_PLUGIN_UNIQUE_ID undefined!
#endif

@class DPFAudioUnit;

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// DPF Callbacks

static bool d_writeMidiCallback(void* const ptr, const MidiEvent& midiEvent);
static bool d_requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value);
static bool d_updateStateValueCallback(void* const ptr, const char* const key, const char* const value);

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// DPFAudioUnit Interface

@interface DPFAudioUnit : AUAudioUnit
{
    DISTRHO_NAMESPACE::PluginExporter* _plugin;
    AUAudioUnitBusArray* _inputBusArray;
    AUAudioUnitBusArray* _outputBusArray;
    AUParameterTree* _parameterTree;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    float* _inputBuffers[DISTRHO_PLUGIN_NUM_INPUTS];
    AudioBufferList* _inputBufferList;
#endif
}

@property (nonatomic, readonly) DISTRHO_NAMESPACE::PluginExporter* pluginExporter;

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError **)outError;

+ (AudioComponentDescription)componentDescription;
+ (void)registerAudioUnit;

@end

// --------------------------------------------------------------------------------------------------------------------
// DPFAudioUnit Implementation

@implementation DPFAudioUnit

@synthesize pluginExporter = _plugin;

+ (AudioComponentDescription)componentDescription
{
    AudioComponentDescription desc;
    std::memset(&desc, 0, sizeof(desc));

    constexpr const char typeStr[] = STRINGIFY(DISTRHO_PLUGIN_AU_TYPE);
    constexpr const char subTypeStr[] = STRINGIFY(DISTRHO_PLUGIN_UNIQUE_ID);
    constexpr const char manufStr[] = STRINGIFY(DISTRHO_PLUGIN_BRAND_ID);

    desc.componentType = (static_cast<OSType>(typeStr[0]) << 24) |
                         (static_cast<OSType>(typeStr[1]) << 16) |
                         (static_cast<OSType>(typeStr[2]) << 8)  |
                         static_cast<OSType>(typeStr[3]);

    desc.componentSubType = (static_cast<OSType>(subTypeStr[0]) << 24) |
                            (static_cast<OSType>(subTypeStr[1]) << 16) |
                            (static_cast<OSType>(subTypeStr[2]) << 8)  |
                            static_cast<OSType>(subTypeStr[3]);

    desc.componentManufacturer = (static_cast<OSType>(manufStr[0]) << 24) |
                                 (static_cast<OSType>(manufStr[1]) << 16) |
                                 (static_cast<OSType>(manufStr[2]) << 8)  |
                                 static_cast<OSType>(manufStr[3]);

    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    return desc;
}

+ (void)registerAudioUnit
{
    const AudioComponentDescription desc = [self componentDescription];
    NSString* const name = [NSString stringWithFormat:@"%s: %s",
                            STRINGIFY(DISTRHO_PLUGIN_BRAND_ID),
                            DISTRHO_PLUGIN_NAME];

#ifdef DISTRHO_PLUGIN_VERSION
    const uint32_t version = DISTRHO_PLUGIN_VERSION;
#else
    const uint32_t version = 0x00010000;
#endif

    [AUAudioUnit registerSubclass:[self class]
           asComponentDescription:desc
                             name:name
                          version:version];
}

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError **)outError
{
    self = [super initWithComponentDescription:componentDescription options:options error:outError];
    if (self == nil)
        return nil;

    if (DISTRHO_NAMESPACE::d_nextBufferSize == 0)
        DISTRHO_NAMESPACE::d_nextBufferSize = 512;
    if (d_isZero(DISTRHO_NAMESPACE::d_nextSampleRate))
        DISTRHO_NAMESPACE::d_nextSampleRate = 44100.0;

    _plugin = new DISTRHO_NAMESPACE::PluginExporter(
        (__bridge void*)self,
        DISTRHO_NAMESPACE::d_writeMidiCallback,
        DISTRHO_NAMESPACE::d_requestParameterValueChangeCallback,
        DISTRHO_NAMESPACE::d_updateStateValueCallback
    );

    // Audio Formats (Standard 44.1kHz non-interleaved float32)
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    AVAudioFormat* const inFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0
                                                                                   channels:DISTRHO_PLUGIN_NUM_INPUTS];
    AUAudioUnitBus* const inBus = [[AUAudioUnitBus alloc] initWithFormat:inFormat error:outError];
    _inputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                           busType:AUAudioUnitBusTypeInput
                                                           busses:@[inBus]];
    _inputBufferList = nullptr;
    for (uint32_t i = 0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        _inputBuffers[i] = nullptr;
#else
    _inputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                           busType:AUAudioUnitBusTypeInput
                                                           busses:@[]];
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    AVAudioFormat* const outFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0
                                                                                    channels:DISTRHO_PLUGIN_NUM_OUTPUTS];
    AUAudioUnitBus* const outBus = [[AUAudioUnitBus alloc] initWithFormat:outFormat error:outError];
    _outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                            busType:AUAudioUnitBusTypeOutput
                                                            busses:@[outBus]];
#else
    _outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                                            busType:AUAudioUnitBusTypeOutput
                                                            busses:@[]];
#endif

    [self setupParameterTree];

    return self;
}

- (void)dealloc
{
    if (_plugin != nullptr)
    {
        delete _plugin;
        _plugin = nullptr;
    }
    [super dealloc];
}

- (void)setupParameterTree
{
    if (_plugin == nullptr)
        return;

    const uint32_t paramCount = _plugin->getParameterCount();
    if (paramCount == 0)
        return;

    NSMutableArray* const params = [NSMutableArray arrayWithCapacity:paramCount];

    for (uint32_t i = 0; i < paramCount; ++i)
    {
        const DISTRHO_NAMESPACE::ParameterRanges& ranges = _plugin->getParameterRanges(i);
        const uint32_t hints = _plugin->getParameterHints(i);

        NSString* const identifier = [NSString stringWithUTF8String:_plugin->getParameterSymbol(i)];
        NSString* const name = [NSString stringWithUTF8String:_plugin->getParameterName(i)];
        const char* const unitStr = _plugin->getParameterUnit(i);
        NSString* const unitName = (unitStr != nullptr && unitStr[0] != '\0') ? [NSString stringWithUTF8String:unitStr] : nil;

        AudioUnitParameterOptions flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable;
        if ((hints & DISTRHO_NAMESPACE::kParameterIsAutomable) != 0)
            flags |= kAudioUnitParameterFlag_CanRamp;
        if ((hints & DISTRHO_NAMESPACE::kParameterIsBoolean) != 0 || (hints & DISTRHO_NAMESPACE::kParameterIsInteger) != 0)
            flags |= kAudioUnitParameterFlag_ValuesHaveStrings;

        AUParameter* const p = [AUParameterTree createParameterWithIdentifier:identifier
                                                                         name:name
                                                                      address:(AUParameterAddress)i
                                                                          min:(AUValue)ranges.min
                                                                          max:(AUValue)ranges.max
                                                                         unit:kAudioUnitParameterUnit_Generic
                                                                     unitName:unitName
                                                                        flags:flags
                                                                 valueStrings:nil
                                                          dependentParameters:nil];
        p.value = _plugin->getParameterValue(i);
        [params addObject:p];
    }

    _parameterTree = [AUParameterTree createTreeWithChildren:params];

    __unsafe_unretained DPFAudioUnit* const weakSelf = self;
    _parameterTree.implementorValueObserver = ^(AUParameter *param, AUValue value) {
        if (weakSelf != nil && weakSelf.pluginExporter != nullptr)
        {
            weakSelf.pluginExporter->setParameterValue((uint32_t)param.address, value);
        }
    };

    _parameterTree.implementorValueProvider = ^AUValue(AUParameter *param) {
        if (weakSelf != nil && weakSelf.pluginExporter != nullptr)
        {
            return weakSelf.pluginExporter->getParameterValue((uint32_t)param.address);
        }
        return 0.0f;
    };

    self.parameterTree = _parameterTree;
}

- (AUAudioUnitBusArray *)inputBusses
{
    return _inputBusArray;
}

- (AUAudioUnitBusArray *)outputBusses
{
    return _outputBusArray;
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)outError
{
    if (![super allocateRenderResourcesAndReturnError:outError])
        return NO;

    double sampleRate = 44100.0;
    if (_outputBusArray.count > 0 && _outputBusArray[0].format != nil)
        sampleRate = _outputBusArray[0].format.sampleRate;
    else if (_inputBusArray.count > 0 && _inputBusArray[0].format != nil)
        sampleRate = _inputBusArray[0].format.sampleRate;

    const uint32_t maxFrames = self.maximumFramesToRender > 0 ? self.maximumFramesToRender : 512;

    _plugin->setSampleRate(sampleRate, true);
    _plugin->setBufferSize(maxFrames, true);
    _plugin->activate();

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    const size_t bufferListSize = sizeof(AudioBufferList) + (DISTRHO_PLUGIN_NUM_INPUTS - 1) * sizeof(AudioBuffer);
    _inputBufferList = (AudioBufferList*)std::calloc(1, bufferListSize);
    _inputBufferList->mNumberBuffers = DISTRHO_PLUGIN_NUM_INPUTS;

    for (uint32_t i = 0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
    {
        _inputBuffers[i] = (float*)std::calloc(maxFrames, sizeof(float));
        _inputBufferList->mBuffers[i].mNumberChannels = 1;
        _inputBufferList->mBuffers[i].mDataByteSize = maxFrames * sizeof(float);
        _inputBufferList->mBuffers[i].mData = _inputBuffers[i];
    }
#endif

    return YES;
}

- (void)deallocateRenderResources
{
    if (_plugin != nullptr)
        _plugin->deactivate();

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    if (_inputBufferList != nullptr)
    {
        for (uint32_t i = 0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            if (_inputBuffers[i] != nullptr)
            {
                std::free(_inputBuffers[i]);
                _inputBuffers[i] = nullptr;
            }
        }
        std::free(_inputBufferList);
        _inputBufferList = nullptr;
    }
#endif

    [super deallocateRenderResources];
}

- (AUInternalRenderBlock)internalRenderBlock
{
    DISTRHO_NAMESPACE::PluginExporter* const plugin = _plugin;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
    AudioBufferList* const inBufferList = _inputBufferList;
    float** const inScratch = _inputBuffers;
#endif

#if DISTRHO_PLUGIN_WANT_TIMEPOS
    AUHostMusicalContextBlock const musicalContext = self.musicalContextBlock;
    AUHostTransportStateBlock const transportState = self.transportStateBlock;
#endif

    AUInternalRenderBlock const block = ^AUAudioUnitStatus(
        AudioUnitRenderActionFlags *actionFlags,
        const AudioTimeStamp *timestamp,
        AUAudioFrameCount frameCount,
        NSInteger outputBusNumber,
        AudioBufferList *outputData,
        const AURenderEvent *realtimeEventListHead,
        AURenderPullInputBlock pullInputBlock)
    {
        (void)actionFlags;
        (void)outputBusNumber;

        // 1. Process Realtime Events (Parameters & MIDI)
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        DISTRHO_NAMESPACE::MidiEvent midiEvents[512];
        uint32_t midiEventCount = 0;
#endif

        for (const AURenderEvent* event = realtimeEventListHead; event != nullptr; event = event->head.next)
        {
            if (event->head.eventType == AURenderEventParameter || event->head.eventType == AURenderEventParameterRamp)
            {
                plugin->setParameterValue((uint32_t)event->parameter.parameterAddress, event->parameter.value);
            }
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            else if (event->head.eventType == AURenderEventMIDI)
            {
                if (midiEventCount < 512 && event->MIDI.length <= 4)
                {
                    DISTRHO_NAMESPACE::MidiEvent& me = midiEvents[midiEventCount++];
                    me.frame = (uint32_t)(event->head.eventSampleTime >= 0 ? event->head.eventSampleTime : 0);
                    me.size = event->MIDI.length;
                    std::memcpy(me.data, event->MIDI.data, event->MIDI.length);
                }
            }
#endif
        }

        // 2. Query Host Transport & Musical Context
#if DISTRHO_PLUGIN_WANT_TIMEPOS
        DISTRHO_NAMESPACE::TimePosition timePos;
        std::memset(&timePos, 0, sizeof(timePos));

        if (transportState != nil)
        {
            AUHostTransportStateFlags flags = 0;
            double currentSample = 0.0;
            if (transportState(&flags, &currentSample, nullptr, nullptr))
            {
                timePos.playing = (flags & AUHostTransportStateMoving) != 0;
                timePos.frame = (uint64_t)(currentSample >= 0.0 ? currentSample : 0.0);
            }
        }
        if (musicalContext != nil)
        {
            double tempo = 120.0;
            double num = 4.0;
            NSInteger den = 4;
            double beatPos = 0.0;
            if (musicalContext(&tempo, &num, &den, &beatPos, nullptr, nullptr))
            {
                timePos.bbt.valid = true;
                timePos.bbt.beatsPerMinute = tempo;
                timePos.bbt.beatsPerBar = (float)num;
                timePos.bbt.beatType = (float)den;
                timePos.bbt.beat = beatPos;
            }
        }
        plugin->setTimePosition(timePos);
#endif

        // 3. Audio Inputs
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        const float* inBuffers[DISTRHO_PLUGIN_NUM_INPUTS];

        if (pullInputBlock != nullptr && inBufferList != nullptr)
        {
            AudioUnitRenderActionFlags pullFlags = 0;
            const AUAudioUnitStatus pullStatus = pullInputBlock(&pullFlags, timestamp, frameCount, 0, inBufferList);
            if (pullStatus != noErr)
                return pullStatus;

            for (uint32_t i = 0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            {
                if (i < inBufferList->mNumberBuffers && inBufferList->mBuffers[i].mData != nullptr)
                    inBuffers[i] = (const float*)inBufferList->mBuffers[i].mData;
                else
                    inBuffers[i] = inScratch[i];
            }
        }
        else
        {
            for (uint32_t i = 0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
                inBuffers[i] = inScratch[i];
        }
#else
        const float** inBuffers = nullptr;
#endif

        // 4. Audio Outputs
#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        float* outBuffers[DISTRHO_PLUGIN_NUM_OUTPUTS];
        for (uint32_t i = 0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            if (outputData != nullptr && i < outputData->mNumberBuffers)
                outBuffers[i] = (float*)outputData->mBuffers[i].mData;
            else
                outBuffers[i] = nullptr;
        }
#else
        float** outBuffers = nullptr;
#endif

        // 5. Execute DSP Run
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        plugin->run(inBuffers, outBuffers, frameCount, midiEvents, midiEventCount);
#else
        plugin->run(inBuffers, outBuffers, frameCount);
#endif

        return noErr;
    };

    return [[block copy] autorelease];
}

- (NSDictionary<NSString *, id> *)fullState
{
    if (_plugin == nullptr)
        return @{};

    NSMutableDictionary* const dict = [NSMutableDictionary dictionary];
    dict[@"version"] = @(_plugin->getVersion());

    const uint32_t paramCount = _plugin->getParameterCount();
    if (paramCount > 0)
    {
        NSMutableDictionary* const params = [NSMutableDictionary dictionaryWithCapacity:paramCount];
        for (uint32_t i = 0; i < paramCount; ++i)
        {
            NSString* const sym = [NSString stringWithUTF8String:_plugin->getParameterSymbol(i)];
            params[sym] = @(_plugin->getParameterValue(i));
        }
        dict[@"parameters"] = params;
    }

#if DISTRHO_PLUGIN_WANT_STATE
    const uint32_t stateCount = _plugin->getStateCount();
    if (stateCount > 0)
    {
        NSMutableDictionary* const states = [NSMutableDictionary dictionaryWithCapacity:stateCount];
        for (uint32_t i = 0; i < stateCount; ++i)
        {
            const char* const key = _plugin->getStateKey(i).buffer();
# if DISTRHO_PLUGIN_WANT_FULL_STATE
            const char* const val = _plugin->getStateValue(key).buffer();
# else
            const char* const val = _plugin->getStateDefaultValue(i).buffer();
# endif
            if (key != nullptr && val != nullptr)
            {
                states[[NSString stringWithUTF8String:key]] = [NSString stringWithUTF8String:val];
            }
        }
        dict[@"states"] = states;
    }
#endif

    return dict;
}

- (void)setFullState:(NSDictionary<NSString *, id> *)state
{
    [super setFullState:state];
    if (state == nil || _plugin == nullptr)
        return;

    NSDictionary* const params = state[@"parameters"];
    if (params != nil && [params isKindOfClass:[NSDictionary class]])
    {
        const uint32_t paramCount = _plugin->getParameterCount();
        for (uint32_t i = 0; i < paramCount; ++i)
        {
            NSString* const sym = [NSString stringWithUTF8String:_plugin->getParameterSymbol(i)];
            NSNumber* const val = params[sym];
            if (val != nil && [val isKindOfClass:[NSNumber class]])
            {
                const float floatVal = [val floatValue];
                _plugin->setParameterValue(i, floatVal);
                if (_parameterTree != nil)
                {
                    AUParameter* const p = [_parameterTree parameterWithAddress:i];
                    if (p != nil)
                        [p setValue:floatVal originator:nil];
                }
            }
        }
    }

#if DISTRHO_PLUGIN_WANT_STATE
    NSDictionary* const states = state[@"states"];
    if (states != nil && [states isKindOfClass:[NSDictionary class]])
    {
        for (NSString* key in states)
        {
            id const val = states[key];
            if ([val isKindOfClass:[NSString class]])
            {
                _plugin->setState([key UTF8String], [((NSString*)val) UTF8String]);
            }
        }
    }
#endif
}

- (NSTimeInterval)latency
{
#if DISTRHO_PLUGIN_WANT_LATENCY
    if (_plugin != nullptr)
    {
        const double sr = (_outputBusArray.count > 0 && _outputBusArray[0].format != nil)
            ? _outputBusArray[0].format.sampleRate
            : 44100.0;
        return (NSTimeInterval)_plugin->getLatency() / sr;
    }
#endif
    return 0.0;
}

- (NSTimeInterval)tailTime
{
    return 0.0;
}

- (void)requestViewControllerWithCompletionHandler:(void (^)(AUViewControllerBase *viewController))completionHandler
{
#if DISTRHO_PLUGIN_HAS_UI
    dispatch_async(dispatch_get_main_queue(), ^{
        Class const vcClass = NSClassFromString(@"DPFAudioUnitViewController");
        if (vcClass != nil && [vcClass instancesRespondToSelector:@selector(initWithAudioUnit:)])
        {
            id const vc = [[vcClass alloc] performSelector:@selector(initWithAudioUnit:) withObject:self];
            completionHandler((AUViewControllerBase*)vc);
            return;
        }
        completionHandler(nil);
    });
#else
    completionHandler(nil);
#endif
}

@end

// --------------------------------------------------------------------------------------------------------------------
// DPF Callback Implementations

START_NAMESPACE_DISTRHO

static bool d_writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
{
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    DPFAudioUnit* const self = (__bridge DPFAudioUnit*)ptr;
    if (self.MIDIOutputEventBlock != nil && midiEvent.size > 0 && midiEvent.size <= 4)
    {
        self.MIDIOutputEventBlock(AUEventSampleTimeImmediate, 0, midiEvent.size, midiEvent.data);
        return true;
    }
#else
    (void)ptr;
    (void)midiEvent;
#endif
    return false;
}

static bool d_requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
{
    DPFAudioUnit* const self = (__bridge DPFAudioUnit*)ptr;
    if (self.parameterTree != nil)
    {
        AUParameter* const p = [self.parameterTree parameterWithAddress:index];
        if (p != nil)
        {
            [p setValue:value originator:nil];
            return true;
        }
    }
    return false;
}

static bool d_updateStateValueCallback(void* const ptr, const char* const key, const char* const value)
{
#if DISTRHO_PLUGIN_WANT_STATE
    DPFAudioUnit* const self = (__bridge DPFAudioUnit*)ptr;
    if (self.pluginExporter != nullptr)
    {
        self.pluginExporter->setState(key, value);
        return true;
    }
#else
    (void)ptr;
    (void)key;
    (void)value;
#endif
    return false;
}

END_NAMESPACE_DISTRHO
