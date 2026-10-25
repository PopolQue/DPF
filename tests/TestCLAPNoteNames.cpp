/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for CLAP Note Names Extension (CLAP_EXT_NOTE_NAME)
 */

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "distrho/src/clap/ext/note-name.h"
#include "distrho/DistrhoDetails.hpp"

USE_NAMESPACE_DISTRHO

class MockDrumSynth
{
public:
    std::vector<NoteName> noteNames;

    MockDrumSynth()
    {
        noteNames.push_back(NoteName("Kick", 36, 0, 0));
        noteNames.push_back(NoteName("Snare", 38, 0, 0));
        noteNames.push_back(NoteName("Closed Hat", 42, 0, 0));
        noteNames.push_back(NoteName("Open Hat", 46, 0, 0));
        noteNames.push_back(NoteName("Crash Cymbal", 49, 0, 0));
    }

    uint32_t getNoteNameCount() const noexcept
    {
        return static_cast<uint32_t>(noteNames.size());
    }

    bool getNoteName(const uint32_t index, NoteName& noteName) const noexcept
    {
        if (index < noteNames.size())
        {
            noteName = noteNames[index];
            return true;
        }
        return false;
    }
};

int main()
{
    std::cout << "--- Testing CLAP Note Name Extension Architecture ---" << std::endl;

    MockDrumSynth synth;

    // 1. Verify NoteName struct defaults and properties
    {
        NoteName empty;
        assert(empty.port == -1);
        assert(empty.channel == -1);
        assert(empty.key == -1);
        assert(empty.name[0] == '\0');

        NoteName custom("808 Sub Kick", 36, 9, 0);
        assert(std::strcmp(custom.name, "808 Sub Kick") == 0);
        assert(custom.key == 36);
        assert(custom.channel == 9);
        assert(custom.port == 0);
    }

    // 2. Verify Drum Map Note Names in Plugin
    {
        assert(synth.getNoteNameCount() == 5);

        NoteName n;
        assert(synth.getNoteName(0, n));
        assert(std::strcmp(n.name, "Kick") == 0);
        assert(n.key == 36);

        assert(synth.getNoteName(1, n));
        assert(std::strcmp(n.name, "Snare") == 0);
        assert(n.key == 38);

        assert(synth.getNoteName(2, n));
        assert(std::strcmp(n.name, "Closed Hat") == 0);
        assert(n.key == 42);

        assert(synth.getNoteName(3, n));
        assert(std::strcmp(n.name, "Open Hat") == 0);
        assert(n.key == 46);

        assert(synth.getNoteName(4, n));
        assert(std::strcmp(n.name, "Crash Cymbal") == 0);
        assert(n.key == 49);

        // Out of bounds query
        assert(!synth.getNoteName(5, n));

        std::cout << "  ✓ Drum map note name count and mappings verified" << std::endl;
    }

    // 3. Verify CLAP C-ABI bridge mapping
    {
        clap_note_name_t clapNote{};
        NoteName n;

        assert(synth.getNoteName(1, n));
        std::strncpy(clapNote.name, n.name, CLAP_NAME_SIZE - 1);
        clapNote.name[CLAP_NAME_SIZE - 1] = '\0';
        clapNote.port = n.port;
        clapNote.channel = n.channel;
        clapNote.key = n.key;

        assert(std::strcmp(clapNote.name, "Snare") == 0);
        assert(clapNote.key == 38);
        assert(clapNote.channel == 0);
        assert(clapNote.port == 0);

        std::cout << "  ✓ CLAP ABI translation to clap_note_name_t verified" << std::endl;
    }

    std::cout << "CLAP Note Name Extension Unit Tests passed successfully!" << std::endl;
    return 0;
}
