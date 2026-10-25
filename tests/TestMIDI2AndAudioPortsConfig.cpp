/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for MIDI 2.0 (UMP) Architecture & CLAP Audio Ports Config
 */

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "distrho/DistrhoDetails.hpp"
#include "distrho/src/clap/ext/audio-ports-config.h"

USE_NAMESPACE_DISTRHO

class MockSurroundPlugin
{
public:
    std::vector<AudioPortConfig> configs;
    uint32_t activeConfigId;

    MockSurroundPlugin() : activeConfigId(1)
    {
        // Config 1: Standard Stereo
        AudioPortConfig stereo(1, "Stereo", 2, 2, "stereo", "stereo");
        configs.push_back(stereo);

        // Config 2: 5.1 Surround
        AudioPortConfig surround(2, "5.1 Surround", 6, 6, "5.1", "5.1");
        configs.push_back(surround);

        // Config 3: Sidechain Comp
        AudioPortConfig sidechain(3, "Stereo + Sidechain", 4, 2, "stereo", "stereo");
        sidechain.hasSidechainInput = true;
        std::strncpy(sidechain.sidechainInputPortType, "stereo", sizeof(sidechain.sidechainInputPortType) - 1);
        configs.push_back(sidechain);
    }

    uint32_t getAudioPortConfigCount() const noexcept
    {
        return static_cast<uint32_t>(configs.size());
    }

    bool getAudioPortConfig(const uint32_t index, AudioPortConfig& config) const noexcept
    {
        if (index < configs.size())
        {
            config = configs[index];
            return true;
        }
        return false;
    }

    bool selectAudioPortConfig(const uint32_t configId) noexcept
    {
        for (const auto& cfg : configs)
        {
            if (cfg.id == configId)
            {
                activeConfigId = configId;
                return true;
            }
        }
        return false;
    }
};

int main()
{
    std::cout << "--- Testing MIDI 2.0 (UMP) Architecture ---" << std::endl;

    // 1. Midi2Event Default Constructor
    {
        Midi2Event ev;
        assert(ev.frame == 0);
        assert(ev.port == 0);
        assert(ev.wordCount == 1);
        assert(ev.messageType == 0);
        assert(ev.words[0] == 0);
        assert(ev.words[1] == 0);
        assert(ev.words[2] == 0);
        assert(ev.words[3] == 0);
    }

    // 2. Midi2Event 32-bit UMP (1-word packet: System Real-Time / Utility)
    {
        const uint32_t kWord0 = 0x10F80000; // Type 0x1 (System Common / Real-Time), Timing Clock (0xF8)
        Midi2Event ev(64, kWord0);
        ev.port = 2;
        assert(ev.frame == 64);
        assert(ev.port == 2);
        assert(ev.wordCount == 1);
        assert(ev.messageType == 0x1);
        assert(ev.words[0] == kWord0);
        assert(ev.words[1] == 0);
    }

    // 3. Midi2Event 64-bit UMP (2-word packet: MIDI 2.0 Channel Voice)
    {
        // Type 0x4 (MIDI 2.0 Voice), Channel 0, Note On (0x90), Key 60 (0x3C), 16-bit velocity 0xC000
        const uint32_t kWord0 = 0x40903C00;
        const uint32_t kWord1 = 0xC0000000;
        Midi2Event ev(128, kWord0, kWord1);
        ev.port = 1;
        assert(ev.frame == 128);
        assert(ev.port == 1);
        assert(ev.wordCount == 2);
        assert(ev.messageType == 0x4);
        assert(ev.words[0] == kWord0);
        assert(ev.words[1] == kWord1);
        assert(ev.words[2] == 0);
        assert(ev.words[3] == 0);
    }

    // 4. Midi2Event 128-bit UMP (4-word packet: Extended Data / SysEx8 / Flex Data)
    {
        const uint32_t kWord0 = 0xD0123456; // Type 0xD (Flex Data)
        const uint32_t kWord1 = 0x11223344;
        const uint32_t kWord2 = 0x55667788;
        const uint32_t kWord3 = 0x99AABBCC;
        Midi2Event ev(256, kWord0, kWord1, kWord2, kWord3, 0);
        assert(ev.frame == 256);
        assert(ev.port == 0);
        assert(ev.wordCount == 4);
        assert(ev.messageType == 0xD);
        assert(ev.words[0] == kWord0);
        assert(ev.words[1] == kWord1);
        assert(ev.words[2] == kWord2);
        assert(ev.words[3] == kWord3);
    }

    // 5. Midi2Event Buffer / Multi-port Constructor
    {
        const uint32_t words[3] = { 0x30112233, 0x44556677, 0x8899AABB };
        Midi2Event ev(512, 3, words, 3);
        assert(ev.frame == 512);
        assert(ev.port == 3);
        assert(ev.wordCount == 3);
        assert(ev.messageType == 0x3);
        assert(ev.words[0] == words[0]);
        assert(ev.words[1] == words[1]);
        assert(ev.words[2] == words[2]);
        assert(ev.words[3] == 0);
    }

    std::cout << "  ✓ Midi2Event UMP (32, 64, 128-bit) construction, timing offsets, and packet decoding verified" << std::endl;

    std::cout << "--- Testing Audio Port Configurations (CLAP_EXT_AUDIO_PORTS_CONFIG) ---" << std::endl;

    MockSurroundPlugin plugin;

    // 1. Verify Configuration Count and Enumeration
    assert(plugin.getAudioPortConfigCount() == 3);

    AudioPortConfig cfg;
    bool ok0 = plugin.getAudioPortConfig(0, cfg);
    (void)ok0;
    assert(ok0);
    assert(cfg.id == 1);
    assert(std::strcmp(cfg.name, "Stereo") == 0);
    assert(cfg.inputPortCount == 2);
    assert(cfg.outputPortCount == 2);
    assert(cfg.hasMainInput == true);
    assert(cfg.hasMainOutput == true);
    assert(cfg.hasSidechainInput == false);

    bool ok1 = plugin.getAudioPortConfig(1, cfg);
    (void)ok1;
    assert(ok1);
    assert(cfg.id == 2);
    assert(std::strcmp(cfg.name, "5.1 Surround") == 0);
    assert(cfg.inputPortCount == 6);
    assert(cfg.outputPortCount == 6);
    assert(std::strcmp(cfg.mainOutputPortType, "5.1") == 0);

    bool ok2 = plugin.getAudioPortConfig(2, cfg);
    (void)ok2;
    assert(ok2);
    assert(cfg.id == 3);
    assert(std::strcmp(cfg.name, "Stereo + Sidechain") == 0);
    assert(cfg.inputPortCount == 4);
    assert(cfg.outputPortCount == 2);
    assert(cfg.hasSidechainInput == true);

    // Out of bounds
    bool okOut = plugin.getAudioPortConfig(3, cfg);
    (void)okOut;
    assert(!okOut);

    // 2. Switching Configurations
    assert(plugin.activeConfigId == 1);

    bool selectOk = plugin.selectAudioPortConfig(2);
    (void)selectOk;
    assert(selectOk);
    assert(plugin.activeConfigId == 2);

    bool selectSidechain = plugin.selectAudioPortConfig(3);
    (void)selectSidechain;
    assert(selectSidechain);
    assert(plugin.activeConfigId == 3);

    // Invalid config ID
    bool selectFail = plugin.selectAudioPortConfig(999);
    (void)selectFail;
    assert(!selectFail);
    assert(plugin.activeConfigId == 3);

    std::cout << "  ✓ AudioPortConfig enumeration, channel layouts, sidechains, and selection verified" << std::endl;

    std::cout << "All MIDI 2.0 (UMP) and Audio Ports Config tests passed successfully!" << std::endl;
    return 0;
}
