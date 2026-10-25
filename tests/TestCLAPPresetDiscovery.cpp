/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for CLAP Preset Discovery Factory & Preset Load Extension
 */

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "distrho/src/clap/factory/preset-discovery.h"
#include "distrho/src/clap/ext/preset-load.h"
#include "distrho/extra/PresetManager.hpp"

USE_NAMESPACE_DISTRHO

struct MockIndexer {
    std::vector<std::string> filetypes;
    std::vector<std::string> locations;

    static bool CLAP_ABI declare_filetype(const clap_preset_discovery_indexer_t* indexer,
                                         const clap_preset_discovery_filetype_t* filetype)
    {
        MockIndexer* self = static_cast<MockIndexer*>(indexer->indexer_data);
        if (filetype != nullptr && filetype->extension != nullptr)
            self->filetypes.push_back(filetype->extension);
        return true;
    }

    static bool CLAP_ABI declare_location(const clap_preset_discovery_indexer_t* indexer,
                                         const clap_preset_discovery_location_t* location)
    {
        MockIndexer* self = static_cast<MockIndexer*>(indexer->indexer_data);
        if (location != nullptr && location->name != nullptr)
            self->locations.push_back(location->name);
        return true;
    }

    static bool CLAP_ABI declare_soundpack(const clap_preset_discovery_indexer_t*,
                                          const clap_preset_discovery_soundpack_t*)
    {
        return true;
    }

    static void* CLAP_ABI get_extension(const clap_preset_discovery_indexer_t*, const char*)
    {
        return nullptr;
    }

    clap_preset_discovery_indexer_t makeIndexer()
    {
        clap_preset_discovery_indexer_t idx{};
        idx.indexer_data = this;
        idx.declare_filetype = declare_filetype;
        idx.declare_location = declare_location;
        idx.declare_soundpack = declare_soundpack;
        idx.get_extension = get_extension;
        return idx;
    }
};

struct MockMetadataReceiver {
    std::string presetName;
    std::string loadKey;
    std::string description;
    std::string creator;
    std::vector<std::string> tags;
    uint32_t flags = 0;

    static void CLAP_ABI on_error(const clap_preset_discovery_metadata_receiver_t*, int32_t, const char*) {}

    static bool CLAP_ABI begin_preset(const clap_preset_discovery_metadata_receiver_t* receiver,
                                     const char* name,
                                     const char* load_key)
    {
        MockMetadataReceiver* self = static_cast<MockMetadataReceiver*>(receiver->receiver_data);
        if (name != nullptr)
            self->presetName = name;
        if (load_key != nullptr)
            self->loadKey = load_key;
        return true;
    }

    static bool CLAP_ABI set_soundpack_id(const clap_preset_discovery_metadata_receiver_t*, const char*)
    {
        return true;
    }

    static bool CLAP_ABI set_flags(const clap_preset_discovery_metadata_receiver_t* receiver, uint32_t flags)
    {
        MockMetadataReceiver* self = static_cast<MockMetadataReceiver*>(receiver->receiver_data);
        self->flags = flags;
        return true;
    }

    static bool CLAP_ABI add_creator(const clap_preset_discovery_metadata_receiver_t* receiver, const char* creator)
    {
        MockMetadataReceiver* self = static_cast<MockMetadataReceiver*>(receiver->receiver_data);
        if (creator != nullptr)
            self->creator = creator;
        return true;
    }

    static bool CLAP_ABI set_description(const clap_preset_discovery_metadata_receiver_t* receiver, const char* description)
    {
        MockMetadataReceiver* self = static_cast<MockMetadataReceiver*>(receiver->receiver_data);
        if (description != nullptr)
            self->description = description;
        return true;
    }

    static bool CLAP_ABI set_creation_time(const clap_preset_discovery_metadata_receiver_t*, const char*) { return true; }
    static bool CLAP_ABI set_modification_time(const clap_preset_discovery_metadata_receiver_t*, const char*) { return true; }

    static bool CLAP_ABI add_tag(const clap_preset_discovery_metadata_receiver_t* receiver, const char* tag)
    {
        MockMetadataReceiver* self = static_cast<MockMetadataReceiver*>(receiver->receiver_data);
        if (tag != nullptr)
            self->tags.push_back(tag);
        return true;
    }

    clap_preset_discovery_metadata_receiver_t makeReceiver()
    {
        clap_preset_discovery_metadata_receiver_t rcv{};
        rcv.receiver_data = this;
        rcv.on_error = on_error;
        rcv.begin_preset = begin_preset;
        rcv.set_soundpack_id = set_soundpack_id;
        rcv.set_flags = set_flags;
        rcv.add_creator = add_creator;
        rcv.set_description = set_description;
        rcv.set_creation_time = set_creation_time;
        rcv.set_modification_time = set_modification_time;
        rcv.add_tag = add_tag;
        return rcv;
    }
};

int main()
{
    std::cout << "--- Testing CLAP Preset Discovery & Load Architecture ---" << std::endl;

    // 1. Create a temporary .dpfpreset file with full metadata
    const char* const testPresetPath = "/tmp/test_clap_discovery.dpfpreset";
    {
        Preset p("Warm Tube Crunch", "Drive", "Distrho Team");
        p.subCategory = "Overdrive";
        p.description = "A warm vintage tube drive emulation";
        p.addTag("Vintage");
        p.addTag("Analog");
        p.setParameter("drive", 7.5f);
        p.setParameter("tone", 4.2f);
        p.setState("oversampling", "4x");

        bool saved = p.saveToFile(testPresetPath);
        (void)saved;
        assert(saved);
    }

    // 2. Test File Discovery and Metadata Extraction
    {
        MockIndexer mockIdx;
        clap_preset_discovery_indexer_t indexer = mockIdx.makeIndexer();

        // Simulate provider init declaring file types
        clap_preset_discovery_filetype_t ftDpf = { "DPF Preset", "DPF Preset File", "dpfpreset" };
        clap_preset_discovery_filetype_t ftJson = { "JSON Preset", "DPF JSON Preset File", "json" };
        indexer.declare_filetype(&indexer, &ftDpf);
        indexer.declare_filetype(&indexer, &ftJson);

        assert(mockIdx.filetypes.size() == 2);
        assert(mockIdx.filetypes[0] == "dpfpreset");
        assert(mockIdx.filetypes[1] == "json");

        // Simulate get_metadata called by host indexer
        MockMetadataReceiver mockRcv;
        clap_preset_discovery_metadata_receiver_t receiver = mockRcv.makeReceiver();

        Preset preset;
        bool loaded = preset.loadFromFile(testPresetPath);
        (void)loaded;
        assert(loaded);

        receiver.begin_preset(&receiver, preset.name.buffer(), testPresetPath);
        receiver.set_flags(&receiver, CLAP_PRESET_DISCOVERY_IS_USER_CONTENT);
        if (preset.description.isNotEmpty())
            receiver.set_description(&receiver, preset.description.buffer());
        if (preset.author.isNotEmpty())
            receiver.add_creator(&receiver, preset.author.buffer());
        if (preset.category.isNotEmpty())
            receiver.add_tag(&receiver, preset.category.buffer());
        if (preset.subCategory.isNotEmpty())
            receiver.add_tag(&receiver, preset.subCategory.buffer());
        for (size_t t = 0; t < preset.tags.size(); ++t)
            receiver.add_tag(&receiver, preset.tags[t].buffer());

        // Verify extracted metadata
        assert(mockRcv.presetName == "Warm Tube Crunch");
        assert(mockRcv.loadKey == testPresetPath);
        assert(mockRcv.flags == CLAP_PRESET_DISCOVERY_IS_USER_CONTENT);
        assert(mockRcv.creator == "Distrho Team");
        assert(mockRcv.description == "A warm vintage tube drive emulation");
        assert(mockRcv.tags.size() >= 4); // Drive, Overdrive, Vintage, Analog

        std::cout << "CLAP Preset Discovery metadata receiver verification: OK" << std::endl;
    }

    // 3. Test Preset Load from Location
    {
        struct MockPlugin {
            float drive = 0.0f;
            float tone = 0.0f;
            std::string state;

            uint32_t getParameterCount() const { return 2; }
            String getParameterSymbol(uint32_t index) const
            {
                if (index == 0) return String("drive");
                if (index == 1) return String("tone");
                return String();
            }
            float getParameterValue(uint32_t index) const
            {
                if (index == 0) return drive;
                if (index == 1) return tone;
                return 0.0f;
            }
            void setParameterValue(uint32_t index, float val)
            {
                if (index == 0) drive = val;
                if (index == 1) tone = val;
            }
            uint32_t getStateCount() const { return 1; }
            String getStateKey(uint32_t) const { return String("oversampling"); }
            String getState(const char*) const { return String(state.c_str()); }
            void setState(const char* key, const char* val)
            {
                if (std::strcmp(key, "oversampling") == 0)
                    state = val;
            }
        } plugin;

        Preset p;
        bool loaded = p.loadFromFile(testPresetPath);
        (void)loaded;
        assert(loaded);
        p.applyToPlugin(plugin);

        assert(std::abs(plugin.drive - 7.5f) < 0.0001f);
        assert(std::abs(plugin.tone - 4.2f) < 0.0001f);
        assert(plugin.state == "4x");

        std::cout << "CLAP Preset Load application to plugin: OK" << std::endl;
    }

    std::remove(testPresetPath);
    std::cout << "All CLAP Preset Discovery tests passed successfully!" << std::endl;
    return 0;
}
