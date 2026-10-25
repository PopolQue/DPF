/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for Preset System & PresetManager
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "distrho/extra/PresetManager.hpp"

USE_NAMESPACE_DISTRHO

// Mock Plugin to test PresetManager applyPreset and capturePreset
class MockPlugin
{
public:
    float fCutoff = 1000.0f;
    float fResonance = 0.5f;
    float fDrive = 0.0f;
    std::string fWaveform = "saw";
    std::string fFilterMode = "lp24";

    uint32_t getParameterCount() const { return 3; }

    String getParameterSymbol(uint32_t index) const
    {
        switch (index)
        {
        case 0: return String("cutoff");
        case 1: return String("resonance");
        case 2: return String("drive");
        default: return String("");
        }
    }

    float getParameterValue(uint32_t index) const
    {
        switch (index)
        {
        case 0: return fCutoff;
        case 1: return fResonance;
        case 2: return fDrive;
        default: return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value)
    {
        switch (index)
        {
        case 0: fCutoff = value; break;
        case 1: fResonance = value; break;
        case 2: fDrive = value; break;
        }
    }

    uint32_t getStateCount() const { return 2; }

    String getStateKey(uint32_t index) const
    {
        switch (index)
        {
        case 0: return String("waveform");
        case 1: return String("filter_mode");
        default: return String("");
        }
    }

    String getState(const char* key) const
    {
        if (std::strcmp(key, "waveform") == 0)
            return String(fWaveform.c_str());
        if (std::strcmp(key, "filter_mode") == 0)
            return String(fFilterMode.c_str());
        return String("");
    }

    void setState(const char* key, const char* value)
    {
        if (std::strcmp(key, "waveform") == 0)
            fWaveform = value;
        else if (std::strcmp(key, "filter_mode") == 0)
            fFilterMode = value;
    }
};

int main()
{
    std::cout << "--- Testing Preset Creation & Accessors ---" << std::endl;
    {
        Preset p("Deep Bass", "Bass", "Sound Designer");
        p.description = "A warm, analog sub bass patch";
        p.addTag("Warm");
        p.addTag("Sub");
        p.addTag("Analog");

        p.setParameter("cutoff", 250.0f);
        p.setParameter("resonance", 0.75f);
        p.setParameter("drive", 3.5f);

        p.setState("waveform", "square");
        p.setState("filter_mode", "lp12");

        assert(std::strcmp(p.name.buffer(), "Deep Bass") == 0);
        assert(std::strcmp(p.category.buffer(), "Bass") == 0);
        assert(p.hasTag("Sub"));
        assert(!p.hasTag("Lead"));
        assert(std::abs(p.getParameter("cutoff") - 250.0f) < 0.001f);
        assert(std::abs(p.getParameter("drive") - 3.5f) < 0.001f);
        assert(std::strcmp(p.getState("waveform").buffer(), "square") == 0);
        assert(std::strcmp(p.getState("filter_mode").buffer(), "lp12") == 0);

        std::cout << "  ✓ Preset metadata, parameters, states, and tags verified" << std::endl;

        // JSON roundtrip
        String json = p.toJSON();
        assert(json.isNotEmpty());

        Preset loaded;
        bool ok = loaded.fromJSON(json.buffer());
        assert(ok);
        assert(std::strcmp(loaded.name.buffer(), "Deep Bass") == 0);
        assert(std::strcmp(loaded.category.buffer(), "Bass") == 0);
        assert(loaded.hasTag("Warm"));
        assert(loaded.hasTag("Analog"));
        assert(std::abs(loaded.getParameter("cutoff") - 250.0f) < 0.001f);
        assert(std::strcmp(loaded.getState("waveform").buffer(), "square") == 0);

        std::cout << "  ✓ Preset JSON serialization and deserialization verified" << std::endl;
    }

    std::cout << "--- Testing PresetManager Bank & Search ---" << std::endl;
    {
        PresetManager manager;

        Preset p1("Acid Lead", "Lead", "Alice");
        p1.description = "Resonant squelchy acid line";
        p1.addTag("Acid");
        p1.addTag("303");
        p1.setParameter("cutoff", 1800.0f);
        p1.setParameter("resonance", 0.95f);
        manager.addPreset(p1);

        Preset p2("Subby Bass", "Bass", "Bob");
        p2.description = "Low end foundation";
        p2.addTag("Sub");
        p2.addTag("Clean");
        p2.setParameter("cutoff", 120.0f);
        p2.setParameter("resonance", 0.2f);
        manager.addPreset(p2);

        Preset p3("Ambient Pad", "Pad", "Alice");
        p3.description = "Lush evolving pad";
        p3.addTag("Warm");
        p3.addTag("Evolving");
        p3.setParameter("cutoff", 3500.0f);
        p3.setParameter("resonance", 0.3f);
        manager.addPreset(p3);

        assert(manager.getPresetCount() == 3);
        std::cout << "  ✓ 3 presets added to PresetManager" << std::endl;

        // Categories
        std::vector<String> cats = manager.getCategories();
        assert(cats.size() == 3); // Lead, Bass, Pad
        std::cout << "  ✓ getCategories() returned Lead, Bass, Pad" << std::endl;

        // Find by category
        std::vector<uint32_t> bassPresets = manager.findByCategory("Bass");
        assert(bassPresets.size() == 1);
        assert(bassPresets[0] == 1);
        std::cout << "  ✓ findByCategory('Bass') found preset index 1" << std::endl;

        // Find by tag
        std::vector<uint32_t> acidPresets = manager.findByTag("Acid");
        assert(acidPresets.size() == 1);
        assert(acidPresets[0] == 0);
        std::cout << "  ✓ findByTag('Acid') found preset index 0" << std::endl;

        // Search text query
        std::vector<uint32_t> searchEvolving = manager.search("evolving");
        assert(searchEvolving.size() == 1);
        assert(searchEvolving[0] == 2);

        std::vector<uint32_t> searchAcid = manager.search("ACID");
        assert(searchAcid.size() == 1);
        assert(searchAcid[0] == 0);
        std::cout << "  ✓ Case-insensitive search query verified" << std::endl;

        // Bank Export & Import
        String bankJSON = manager.exportBankToJSON();
        assert(bankJSON.isNotEmpty());

        PresetManager importedManager;
        bool ok = importedManager.importBankFromJSON(bankJSON.buffer());
        assert(ok);
        assert(importedManager.getPresetCount() == 3);
        assert(std::strcmp(importedManager.getPreset(0).name.buffer(), "Acid Lead") == 0);
        assert(std::strcmp(importedManager.getPreset(2).name.buffer(), "Ambient Pad") == 0);
        std::cout << "  ✓ Bank JSON export and import verified" << std::endl;
    }

    std::cout << "--- Testing Plugin State Capture & Application ---" << std::endl;
    {
        MockPlugin plugin;
        PresetManager manager;

        // Initialize plugin parameters
        plugin.setParameterValue(0, 440.0f);
        plugin.setParameterValue(1, 0.8f);
        plugin.setParameterValue(2, 5.0f);
        plugin.setState("waveform", "triangle");
        plugin.setState("filter_mode", "hp12");

        // Capture preset from plugin
        Preset captured = manager.capturePreset(plugin, "Captured Init", "Custom", "TestRunner");
        assert(std::abs(captured.getParameter("cutoff") - 440.0f) < 0.001f);
        assert(std::abs(captured.getParameter("resonance") - 0.8f) < 0.001f);
        assert(std::abs(captured.getParameter("drive") - 5.0f) < 0.001f);
        assert(std::strcmp(captured.getState("waveform").buffer(), "triangle") == 0);
        assert(std::strcmp(captured.getState("filter_mode").buffer(), "hp12") == 0);
        manager.addPreset(captured);
        std::cout << "  ✓ capturePreset accurately mirrored live plugin parameters & states" << std::endl;

        // Now modify plugin values
        plugin.setParameterValue(0, 10000.0f);
        plugin.setParameterValue(1, 0.1f);
        plugin.setParameterValue(2, 0.0f);
        plugin.setState("waveform", "sine");

        // Apply captured preset back to plugin
        bool applied = manager.applyPreset(plugin, 0);
        assert(applied);
        assert(std::abs(plugin.fCutoff - 440.0f) < 0.001f);
        assert(std::abs(plugin.fResonance - 0.8f) < 0.001f);
        assert(std::abs(plugin.fDrive - 5.0f) < 0.001f);
        assert(plugin.fWaveform == "triangle");
        assert(plugin.fFilterMode == "hp12");
        std::cout << "  ✓ applyPreset restored all parameters and states to plugin" << std::endl;
    }

    std::cout << "Preset System Unit Tests passed successfully!" << std::endl;
    return 0;
}
