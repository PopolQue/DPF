/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for VST3 UnitInfo (IUnitInfo) & Flexible Bus Negotiation
 */

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "distrho/src/travesty/base.h"
#include "distrho/src/travesty/unit.h"
#include "distrho/src/travesty/audio_processor.h"

// Speaker arrangement definitions matching VST3
static constexpr const v3_speaker_arrangement kArrStereo = V3_SPEAKER_L | V3_SPEAKER_R;
static constexpr const v3_speaker_arrangement kArrMono   = V3_SPEAKER_M;
static constexpr const v3_speaker_arrangement kArrEmpty  = 0;
static constexpr const v3_speaker_arrangement kArr71Surround = 0x00ff;

struct MockPortGroup {
    uint32_t groupId;
    const char* name;
};

// Simulation of DPF VST3 get_unit_count and get_unit_info
static int32_t mock_get_unit_count(const std::vector<MockPortGroup>& groups)
{
    return 1 + static_cast<int32_t>(groups.size());
}

static v3_result mock_get_unit_info(const std::vector<MockPortGroup>& groups,
                                    const int32_t unit_idx,
                                    v3_unit_info* const info)
{
    if (info == nullptr || unit_idx < 0)
        return V3_INVALID_ARG;

    const uint32_t groupCount = static_cast<uint32_t>(groups.size());
    if (unit_idx > static_cast<int32_t>(groupCount))
        return V3_INVALID_ARG;

    std::memset(info, 0, sizeof(v3_unit_info));

    if (unit_idx == 0)
    {
        info->id = 0; // Root unit id
        info->parent_unit_id = -1; // No parent
        info->program_list_id = -1;
        // Mock utf16 write for name "Root"
        info->name[0] = 'R';
        info->name[1] = 'o';
        info->name[2] = 'o';
        info->name[3] = 't';
        info->name[4] = 0;
        return V3_OK;
    }

    const MockPortGroup& group = groups[static_cast<uint32_t>(unit_idx - 1)];
    info->id = static_cast<int32_t>(group.groupId + 1);
    info->parent_unit_id = 0; // Child of root
    info->program_list_id = -1;
    for (size_t i = 0; i < std::strlen(group.name); ++i)
        info->name[i] = group.name[i];
    info->name[std::strlen(group.name)] = 0;
    return V3_OK;
}

// Simulation of DPF VST3 setAudioBusArrangement
static bool mock_set_audio_bus_arrangement(v3_speaker_arrangement requestedArr,
                                           v3_speaker_arrangement expectedArr,
                                           bool* enabledPorts,
                                           uint32_t portCount)
{
    // Disabling the bus
    if (requestedArr == 0)
    {
        for (uint32_t i = 0; i < portCount; ++i)
            enabledPorts[i] = false;
        return true;
    }

    // Exact match
    if (requestedArr == expectedArr)
    {
        for (uint32_t i = 0; i < portCount; ++i)
            enabledPorts[i] = true;
        return true;
    }

    // Mono fallback on stereo bus
    if (expectedArr == kArrStereo && requestedArr == kArrMono)
    {
        for (uint32_t i = 0; i < portCount; ++i)
            enabledPorts[i] = (i % 2 == 0);
        return true;
    }

    // Unsupported arrangement
    return false;
}

static v3_speaker_arrangement mock_get_audio_bus_arrangement(v3_speaker_arrangement defaultArr,
                                                            const bool* enabledPorts,
                                                            uint32_t portCount)
{
    uint32_t activePorts = 0;
    for (uint32_t i = 0; i < portCount; ++i)
    {
        if (enabledPorts[i])
            activePorts++;
    }

    if (activePorts == 0)
        return 0;

    if (activePorts == 1 && portCount > 1)
        return kArrMono;

    return defaultArr;
}

int main()
{
    std::cout << "--- Testing VST3 IUnitInfo Tree Hierarchy ---" << std::endl;
    {
        std::vector<MockPortGroup> groups = {
            { 0, "Oscillator" },
            { 1, "Filter" },
            { 2, "Modulation" }
        };

        int32_t unitCount = mock_get_unit_count(groups);
        assert(unitCount == 4); // Root + 3 groups
        (void)unitCount;
        std::cout << "  ✓ mock_get_unit_count returned 4 units (Root + 3 groups)" << std::endl;

        // Query Unit 0 (Root)
        v3_unit_info info;
        v3_result res = mock_get_unit_info(groups, 0, &info);
        assert(res == V3_OK);
        assert(info.id == 0);
        assert(info.parent_unit_id == -1);
        assert(info.program_list_id == -1);
        std::cout << "  ✓ Root Unit: id=0, parent=-1 confirmed" << std::endl;

        // Query Unit 1 (Oscillator)
        res = mock_get_unit_info(groups, 1, &info);
        assert(res == V3_OK);
        assert(info.id == 1); // groupId 0 + 1
        assert(info.parent_unit_id == 0); // child of root
        std::cout << "  ✓ Unit 1: id=1, parent=0 (Oscillator) confirmed" << std::endl;

        // Query Unit 2 (Filter)
        res = mock_get_unit_info(groups, 2, &info);
        assert(res == V3_OK);
        assert(info.id == 2); // groupId 1 + 1
        assert(info.parent_unit_id == 0);
        std::cout << "  ✓ Unit 2: id=2, parent=0 (Filter) confirmed" << std::endl;

        // Query Unit 3 (Modulation)
        res = mock_get_unit_info(groups, 3, &info);
        assert(res == V3_OK);
        assert(info.id == 3); // groupId 2 + 1
        assert(info.parent_unit_id == 0);
        std::cout << "  ✓ Unit 3: id=3, parent=0 (Modulation) confirmed" << std::endl;

        // Out of bounds query
        res = mock_get_unit_info(groups, 4, &info);
        assert(res == V3_INVALID_ARG);
        res = mock_get_unit_info(groups, -1, &info);
        assert(res == V3_INVALID_ARG);
        (void)res;
        std::cout << "  ✓ Out of bounds unit indices rejected with V3_INVALID_ARG" << std::endl;
    }

    std::cout << "--- Testing VST3 Parameter unit_id Mapping ---" << std::endl;
    {
        uint32_t groupFilter = 1;
        uint32_t groupNone = UINT32_MAX;

        int32_t unitIdFilter = (groupFilter != UINT32_MAX) ? static_cast<int32_t>(groupFilter + 1) : 0;
        int32_t unitIdUngrouped = (groupNone != UINT32_MAX) ? static_cast<int32_t>(groupNone + 1) : 0;

        assert(unitIdFilter == 2);
        assert(unitIdUngrouped == 0);
        (void)unitIdFilter;
        (void)unitIdUngrouped;
        std::cout << "  ✓ Grouped param maps to unit_id=2; ungrouped maps to unit_id=0 (Root)" << std::endl;
    }

    std::cout << "--- Testing VST3 Flexible Bus Negotiation ---" << std::endl;
    {
        bool ports[2] = { false, false };

        // 1. Exact match (Stereo)
        bool ok = mock_set_audio_bus_arrangement(kArrStereo, kArrStereo, ports, 2);
        assert(ok);
        assert(ports[0] == true && ports[1] == true);
        v3_speaker_arrangement queryArr = mock_get_audio_bus_arrangement(kArrStereo, ports, 2);
        assert(queryArr == kArrStereo);
        std::cout << "  ✓ Exact Stereo arrangement negotiation -> OK, 2 ports active" << std::endl;

        // 2. Disable bus (arr = 0)
        ok = mock_set_audio_bus_arrangement(kArrEmpty, kArrStereo, ports, 2);
        assert(ok);
        assert(ports[0] == false && ports[1] == false);
        queryArr = mock_get_audio_bus_arrangement(kArrStereo, ports, 2);
        assert(queryArr == 0);
        std::cout << "  ✓ Disabling bus (arr=0) -> OK, 0 ports active, query returns 0" << std::endl;

        // 3. Mono fallback on stereo bus
        ok = mock_set_audio_bus_arrangement(kArrMono, kArrStereo, ports, 2);
        assert(ok);
        assert(ports[0] == true && ports[1] == false);
        queryArr = mock_get_audio_bus_arrangement(kArrStereo, ports, 2);
        assert(queryArr == kArrMono);
        (void)ok;
        (void)queryArr;
        std::cout << "  ✓ Mono fallback on stereo bus -> OK, port 0 active, query returns Mono" << std::endl;

        // 4. Unsupported 7.1 surround on stereo bus
        ok = mock_set_audio_bus_arrangement(kArr71Surround, kArrStereo, ports, 2);
        assert(!ok); // Rejected with V3_FALSE
        std::cout << "  ✓ Unsupported arrangement 7.1 surround correctly rejected" << std::endl;
    }

    std::cout << "VST3 UnitInfo and Bus Negotiation Unit Tests passed successfully!" << std::endl;
    return 0;
}
