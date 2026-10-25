/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for CLAP Extensions (Remote Controls, Param Indication, Track Info)
 */

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include "distrho/src/clap/color.h"
#include "distrho/src/clap/id.h"
#include "distrho/src/clap/ext/remote-controls.h"
#include "distrho/src/clap/ext/param-indication.h"
#include "distrho/src/clap/ext/track-info.h"

static inline uint32_t clapColorToRGBA(const clap_color_t& color) noexcept
{
    return (static_cast<uint32_t>(color.red) << 24)
         | (static_cast<uint32_t>(color.green) << 16)
         | (static_cast<uint32_t>(color.blue) << 8)
         | static_cast<uint32_t>(color.alpha);
}

struct DummyPortGroup {
    uint32_t groupId;
    const char* name;
};

struct DummyParam {
    uint32_t id;
    const char* name;
    uint32_t groupId;
    bool isOutput;
};

struct RemoteControlPage {
    clap_id page_id;
    clap_id section_name_id;
    char page_name[CLAP_NAME_SIZE];
    clap_id param_ids[CLAP_REMOTE_CONTROLS_COUNT];
};

static void generateRemoteControlPages(const std::vector<DummyPortGroup>& portGroups,
                                       const std::vector<DummyParam>& params,
                                       std::vector<RemoteControlPage>& pages)
{
    pages.clear();
    clap_id nextPageId = 0;

    const uint32_t portGroupCount = static_cast<uint32_t>(portGroups.size());
    const uint32_t paramCount = static_cast<uint32_t>(params.size());

    if (portGroupCount > 0)
    {
        for (uint32_t g = 0; g < portGroupCount; ++g)
        {
            const DummyPortGroup& group = portGroups[g];
            std::vector<uint32_t> groupParams;
            for (uint32_t p = 0; p < paramCount; ++p)
            {
                if (params[p].groupId == group.groupId && !params[p].isOutput)
                    groupParams.push_back(p);
            }

            if (groupParams.empty())
                continue;

            const uint32_t numChunks = (static_cast<uint32_t>(groupParams.size()) + CLAP_REMOTE_CONTROLS_COUNT - 1) / CLAP_REMOTE_CONTROLS_COUNT;
            for (uint32_t c = 0; c < numChunks; ++c)
            {
                RemoteControlPage page;
                std::memset(&page, 0, sizeof(page));
                page.page_id = nextPageId++;
                page.section_name_id = group.groupId + 1;

                if (numChunks == 1)
                {
                    std::strncpy(page.page_name, group.name, CLAP_NAME_SIZE);
                }
                else
                {
                    char nameBuf[CLAP_NAME_SIZE];
                    std::snprintf(nameBuf, sizeof(nameBuf), "%s %u", group.name, c + 1);
                    std::strncpy(page.page_name, nameBuf, CLAP_NAME_SIZE);
                }

                for (uint32_t i = 0; i < CLAP_REMOTE_CONTROLS_COUNT; ++i)
                {
                    const uint32_t paramIndex = c * CLAP_REMOTE_CONTROLS_COUNT + i;
                    if (paramIndex < groupParams.size())
                        page.param_ids[i] = groupParams[paramIndex];
                    else
                        page.param_ids[i] = CLAP_INVALID_ID;
                }

                pages.push_back(page);
            }
        }

        std::vector<uint32_t> ungroupedParams;
        for (uint32_t p = 0; p < paramCount; ++p)
        {
            if (params[p].groupId == UINT32_MAX && !params[p].isOutput)
                ungroupedParams.push_back(p);
        }

        if (!ungroupedParams.empty())
        {
            const uint32_t numChunks = (static_cast<uint32_t>(ungroupedParams.size()) + CLAP_REMOTE_CONTROLS_COUNT - 1) / CLAP_REMOTE_CONTROLS_COUNT;
            for (uint32_t c = 0; c < numChunks; ++c)
            {
                RemoteControlPage page;
                std::memset(&page, 0, sizeof(page));
                page.page_id = nextPageId++;
                page.section_name_id = CLAP_INVALID_ID;

                if (numChunks == 1)
                {
                    std::strncpy(page.page_name, "General", CLAP_NAME_SIZE);
                }
                else
                {
                    char nameBuf[CLAP_NAME_SIZE];
                    std::snprintf(nameBuf, sizeof(nameBuf), "General %u", c + 1);
                    std::strncpy(page.page_name, nameBuf, CLAP_NAME_SIZE);
                }

                for (uint32_t i = 0; i < CLAP_REMOTE_CONTROLS_COUNT; ++i)
                {
                    const uint32_t paramIndex = c * CLAP_REMOTE_CONTROLS_COUNT + i;
                    if (paramIndex < ungroupedParams.size())
                        page.param_ids[i] = ungroupedParams[paramIndex];
                    else
                        page.param_ids[i] = CLAP_INVALID_ID;
                }

                pages.push_back(page);
            }
        }
    }
    else if (paramCount > 0)
    {
        std::vector<uint32_t> validParams;
        for (uint32_t p = 0; p < paramCount; ++p)
        {
            if (!params[p].isOutput)
                validParams.push_back(p);
        }

        if (!validParams.empty())
        {
            const uint32_t numChunks = (static_cast<uint32_t>(validParams.size()) + CLAP_REMOTE_CONTROLS_COUNT - 1) / CLAP_REMOTE_CONTROLS_COUNT;
            for (uint32_t c = 0; c < numChunks; ++c)
            {
                RemoteControlPage page;
                std::memset(&page, 0, sizeof(page));
                page.page_id = nextPageId++;
                page.section_name_id = CLAP_INVALID_ID;

                char nameBuf[CLAP_NAME_SIZE];
                std::snprintf(nameBuf, sizeof(nameBuf), "Page %u", c + 1);
                std::strncpy(page.page_name, nameBuf, CLAP_NAME_SIZE);

                for (uint32_t i = 0; i < CLAP_REMOTE_CONTROLS_COUNT; ++i)
                {
                    const uint32_t paramIndex = c * CLAP_REMOTE_CONTROLS_COUNT + i;
                    if (paramIndex < validParams.size())
                        page.param_ids[i] = validParams[paramIndex];
                    else
                        page.param_ids[i] = CLAP_INVALID_ID;
                }

                pages.push_back(page);
            }
        }
    }
}

int main()
{
    std::cout << "--- Testing CLAP Color Conversion ---" << std::endl;
    {
        clap_color_t color;
        color.red = 0x12;
        color.green = 0x34;
        color.blue = 0x56;
        color.alpha = 0x78;

        uint32_t rgba = clapColorToRGBA(color);
        assert(rgba == 0x12345678);
        (void)rgba;
        std::cout << "  ✓ clapColorToRGBA: 0x12345678 matches" << std::endl;
    }

    std::cout << "--- Testing CLAP Remote Controls Paging ---" << std::endl;
    {
        std::vector<DummyPortGroup> groups = {
            { 0, "Filter" },
            { 1, "Oscillator" }
        };

        std::vector<DummyParam> params;
        // Group 0: Filter (3 params)
        params.push_back({ 0, "Cutoff", 0, false });
        params.push_back({ 1, "Resonance", 0, false });
        params.push_back({ 2, "Env Amount", 0, false });

        // Group 1: Oscillator (10 params -> will split into 2 pages: 8 + 2)
        for (uint32_t i = 0; i < 10; ++i)
        {
            params.push_back({ 3 + i, "Osc Param", 1, false });
        }

        // Ungrouped: 2 params
        params.push_back({ 13, "Master Volume", UINT32_MAX, false });
        params.push_back({ 14, "Master Pan", UINT32_MAX, false });

        std::vector<RemoteControlPage> pages;
        generateRemoteControlPages(groups, params, pages);

        // Expecting 4 pages:
        // Page 0: Filter (3 params + 5 CLAP_INVALID_ID)
        // Page 1: Oscillator 1 (8 params)
        // Page 2: Oscillator 2 (2 params + 6 CLAP_INVALID_ID)
        // Page 3: General (2 params + 6 CLAP_INVALID_ID)
        assert(pages.size() == 4);
        std::cout << "  ✓ Page count: " << pages.size() << " pages generated" << std::endl;

        // Check Page 0
        assert(pages[0].page_id == 0);
        assert(pages[0].section_name_id == 1); // groupId 0 + 1
        assert(std::strcmp(pages[0].page_name, "Filter") == 0);
        assert(pages[0].param_ids[0] == 0);
        assert(pages[0].param_ids[1] == 1);
        assert(pages[0].param_ids[2] == 2);
        for (int i = 3; i < 8; ++i)
            assert(pages[0].param_ids[i] == CLAP_INVALID_ID);
        std::cout << "  ✓ Page 0: Filter page correctly structured with 3 active and 5 invalid slots" << std::endl;

        // Check Page 1
        assert(pages[1].page_id == 1);
        assert(pages[1].section_name_id == 2); // groupId 1 + 1
        assert(std::strcmp(pages[1].page_name, "Oscillator 1") == 0);
        for (int i = 0; i < 8; ++i)
            assert(pages[1].param_ids[i] == static_cast<clap_id>(3 + i));
        std::cout << "  ✓ Page 1: Oscillator 1 has 8 full parameter slots" << std::endl;

        // Check Page 2
        assert(pages[2].page_id == 2);
        assert(pages[2].section_name_id == 2);
        assert(std::strcmp(pages[2].page_name, "Oscillator 2") == 0);
        assert(pages[2].param_ids[0] == 11);
        assert(pages[2].param_ids[1] == 12);
        for (int i = 2; i < 8; ++i)
            assert(pages[2].param_ids[i] == CLAP_INVALID_ID);
        std::cout << "  ✓ Page 2: Oscillator 2 overflow page has 2 active and 6 invalid slots" << std::endl;

        // Check Page 3 (General ungrouped)
        assert(pages[3].page_id == 3);
        assert(pages[3].section_name_id == CLAP_INVALID_ID);
        assert(std::strcmp(pages[3].page_name, "General") == 0);
        assert(pages[3].param_ids[0] == 13);
        assert(pages[3].param_ids[1] == 14);
        for (int i = 2; i < 8; ++i)
            assert(pages[3].param_ids[i] == CLAP_INVALID_ID);
        std::cout << "  ✓ Page 3: General ungrouped page accurately populated" << std::endl;
    }

    std::cout << "--- Testing CLAP Remote Controls (No Groups) ---" << std::endl;
    {
        std::vector<DummyPortGroup> groups; // empty
        std::vector<DummyParam> params;
        for (uint32_t i = 0; i < 12; ++i)
            params.push_back({ i, "Param", UINT32_MAX, false });

        std::vector<RemoteControlPage> pages;
        generateRemoteControlPages(groups, params, pages);

        // 12 params -> 2 pages: "Page 1" (8 params), "Page 2" (4 params + 4 invalid)
        assert(pages.size() == 2);
        assert(std::strcmp(pages[0].page_name, "Page 1") == 0);
        assert(std::strcmp(pages[1].page_name, "Page 2") == 0);
        for (int i = 0; i < 8; ++i)
            assert(pages[0].param_ids[i] == static_cast<clap_id>(i));
        for (int i = 0; i < 4; ++i)
            assert(pages[1].param_ids[i] == static_cast<clap_id>(8 + i));
        for (int i = 4; i < 8; ++i)
            assert(pages[1].param_ids[i] == CLAP_INVALID_ID);

        std::cout << "  ✓ Default chunking without groups verified (Page 1 & Page 2)" << std::endl;
    }

    std::cout << "--- Testing CLAP Track Info Flags & Mock Dispatch ---" << std::endl;
    {
        clap_track_info_t trackInfo;
        std::memset(&trackInfo, 0, sizeof(trackInfo));

        trackInfo.flags = CLAP_TRACK_INFO_HAS_TRACK_NAME | CLAP_TRACK_INFO_HAS_TRACK_COLOR;
        std::strncpy(trackInfo.name, "Lead Synth", CLAP_NAME_SIZE);
        trackInfo.color.red = 0x33;
        trackInfo.color.green = 0x99;
        trackInfo.color.blue = 0xFF;
        trackInfo.color.alpha = 0xFF;

        assert(trackInfo.flags & CLAP_TRACK_INFO_HAS_TRACK_NAME);
        assert(trackInfo.flags & CLAP_TRACK_INFO_HAS_TRACK_COLOR);
        assert(std::strcmp(trackInfo.name, "Lead Synth") == 0);
        uint32_t colorRGBA = clapColorToRGBA(trackInfo.color);
        assert(colorRGBA == 0x3399FFFF);
        (void)colorRGBA;
        std::cout << "  ✓ Track Info extraction and color packing (0x3399FFFF) verified" << std::endl;
    }

    std::cout << "CLAP Extensions Unit Tests passed successfully!" << std::endl;
    return 0;
}
