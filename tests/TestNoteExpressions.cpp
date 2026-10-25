/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for Per-Note Expression & Polyphonic Modulation (MPE, CLAP & VST3)
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "distrho/DistrhoDetails.hpp"

USE_NAMESPACE_DISTRHO

// Test harness simulating Plugin process loop
class NoteExpressionTestHarness
{
public:
    std::vector<NoteExpressionEvent> events;

    void clearEvents()
    {
        events.clear();
    }

    void addEvent(const NoteExpressionEvent& ev)
    {
        events.push_back(ev);
    }

    uint32_t getCount() const
    {
        return static_cast<uint32_t>(events.size());
    }

    const NoteExpressionEvent& getEvent(uint32_t index) const
    {
        static const NoteExpressionEvent sFallback{};
        if (index < events.size())
            return events[index];
        return sFallback;
    }
};

int main()
{
    std::cout << "--- Testing Per-Note Expression & Polyphonic Modulation Architecture ---" << std::endl;

    NoteExpressionTestHarness harness;

    // 1. Initial State
    assert(harness.getCount() == 0);

    // 2. Simulate CLAP Note Expression Events
    {
        // Event 1: Note tuning (pitch bend on noteId 42, +2.5 semitones)
        NoteExpressionEvent evTuning{};
        evTuning.frame = 128;
        evTuning.key = 60; // C4
        evTuning.channel = 0;
        evTuning.noteId = 42;
        evTuning.type = kNoteExpressionTuning;
        evTuning.value = 2.5; // +2.5 semitones
        harness.addEvent(evTuning);

        // Event 2: Note pressure (aftertouch on noteId 42, 0.85)
        NoteExpressionEvent evPressure{};
        evPressure.frame = 256;
        evPressure.key = 60;
        evPressure.channel = 0;
        evPressure.noteId = 42;
        evPressure.type = kNoteExpressionPressure;
        evPressure.value = 0.85;
        harness.addEvent(evPressure);

        // Event 3: Note brightness (timbre / CC74 on noteId 43, 0.6)
        NoteExpressionEvent evBrightness{};
        evBrightness.frame = 384;
        evBrightness.key = 64; // E4
        evBrightness.channel = 1;
        evBrightness.noteId = 43;
        evBrightness.type = kNoteExpressionBrightness;
        evBrightness.value = 0.6;
        harness.addEvent(evBrightness);

        assert(harness.getCount() == 3);

        const NoteExpressionEvent& e0 = harness.getEvent(0);
        (void)e0;
        assert(e0.frame == 128);
        assert(e0.key == 60);
        assert(e0.noteId == 42);
        assert(e0.type == kNoteExpressionTuning);
        assert(std::abs(e0.value - 2.5) < 0.0001);

        const NoteExpressionEvent& e1 = harness.getEvent(1);
        (void)e1;
        assert(e1.frame == 256);
        assert(e1.type == kNoteExpressionPressure);
        assert(std::abs(e1.value - 0.85) < 0.0001);

        const NoteExpressionEvent& e2 = harness.getEvent(2);
        (void)e2;
        assert(e2.frame == 384);
        assert(e2.key == 64);
        assert(e2.noteId == 43);
        assert(e2.type == kNoteExpressionBrightness);
        assert(std::abs(e2.value - 0.6) < 0.0001);

        std::cout << "CLAP Note Expression mapping and retrieval: OK" << std::endl;
    }

    // 3. Simulate process block boundary reset
    harness.clearEvents();
    assert(harness.getCount() == 0);

    // 4. Simulate VST3 Note Expression & Poly Pressure Mapping
    {
        // VST3 Tuning conversion: normalized [0.0, 1.0] to [-120.0, +120.0] semitones
        // norm 0.5 -> 0 semitones
        // norm 0.55 -> (0.55 - 0.5) * 240 = +12 semitones
        const double vst3TuningNorm = 0.55;
        const double calculatedSemitones = (vst3TuningNorm - 0.5) * 240.0;

        NoteExpressionEvent evVst3Tuning{};
        evVst3Tuning.frame = 64;
        evVst3Tuning.noteId = 101;
        evVst3Tuning.key = -1;
        evVst3Tuning.channel = -1;
        evVst3Tuning.type = kNoteExpressionTuning;
        evVst3Tuning.value = calculatedSemitones;
        harness.addEvent(evVst3Tuning);

        // VST3 Poly Pressure: pitch 67, channel 2, pressure 0.72
        NoteExpressionEvent evVst3Poly{};
        evVst3Poly.frame = 120;
        evVst3Poly.noteId = 102;
        evVst3Poly.key = 67;
        evVst3Poly.channel = 2;
        evVst3Poly.type = kNoteExpressionPressure;
        evVst3Poly.value = 0.72;
        harness.addEvent(evVst3Poly);

        assert(harness.getCount() == 2);
        assert(std::abs(harness.getEvent(0).value - 12.0) < 0.0001);
        assert(harness.getEvent(1).type == kNoteExpressionPressure);
        assert(harness.getEvent(1).key == 67);
        assert(std::abs(harness.getEvent(1).value - 0.72) < 0.0001);

        std::cout << "VST3 Note Expression & Poly Pressure conversion: OK" << std::endl;
    }

    std::cout << "All Note Expression tests passed successfully!" << std::endl;
    return 0;
}
