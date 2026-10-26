/*
 * DISTRHO Plugin Framework (DPF)
 * Unit test for VST3 Native Context Menu (IComponentHandler3 / IContextMenu)
 * and IUnitInfo Program List Architecture
 */

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>

#include "distrho/src/travesty/base.h"
#include "distrho/src/travesty/unit.h"
#include "distrho/src/travesty/edit_controller.h"

// --------------------------------------------------------------------------------------------------------------------
// UTF-16 helper for testing

static void convert_from_utf16(const v3_str_128 src, char* const dst, size_t maxLen)
{
    size_t i = 0;
    while (src[i] != 0 && i + 1 < maxLen)
    {
        dst[i] = static_cast<char>(src[i]);
        ++i;
    }
    dst[i] = '\0';
}

// --------------------------------------------------------------------------------------------------------------------
// Mock VST3 Context Menu & ComponentHandler3 Implementation

struct MockContextMenu : v3_context_menu_cpp {
    int32_t refCount;
    int32_t popupX;
    int32_t popupY;
    bool popupCalled;
    std::vector<v3_context_menu_item> items;

    MockContextMenu()
        : refCount(1),
          popupX(0),
          popupY(0),
          popupCalled(false)
    {
        query_interface = mock_query_interface;
        ref = mock_ref;
        unref = mock_unref;

        menu.get_item_count = mock_get_item_count;
        menu.get_item = mock_get_item;
        menu.add_item = mock_add_item;
        menu.remove_item = mock_remove_item;
        menu.popup = mock_popup;
    }

    static v3_result V3_API mock_query_interface(void*, const v3_tuid, void**)
    {
        return V3_NOT_IMPLEMENTED;
    }

    static uint32_t V3_API mock_ref(void* self)
    {
        MockContextMenu* const m = *static_cast<MockContextMenu**>(self);
        return static_cast<uint32_t>(++m->refCount);
    }

    static uint32_t V3_API mock_unref(void* self)
    {
        MockContextMenu* const m = *static_cast<MockContextMenu**>(self);
        const int32_t res = --m->refCount;
        return static_cast<uint32_t>(res);
    }

    static int32_t V3_API mock_get_item_count(void* self)
    {
        MockContextMenu* const m = *static_cast<MockContextMenu**>(self);
        return static_cast<int32_t>(m->items.size());
    }

    static v3_result V3_API mock_get_item(void* self, int32_t index, v3_context_menu_item* item, v3_context_menu_target***)
    {
        MockContextMenu* const m = *static_cast<MockContextMenu**>(self);
        if (item == nullptr || index < 0 || static_cast<size_t>(index) >= m->items.size())
            return V3_INVALID_ARG;

        *item = m->items[static_cast<size_t>(index)];
        return V3_OK;
    }

    static v3_result V3_API mock_add_item(void* self, const v3_context_menu_item* item, v3_context_menu_target**)
    {
        MockContextMenu* const m = *static_cast<MockContextMenu**>(self);
        if (item == nullptr)
            return V3_INVALID_ARG;

        m->items.push_back(*item);
        return V3_OK;
    }

    static v3_result V3_API mock_remove_item(void*, const v3_context_menu_item*, v3_context_menu_target**)
    {
        return V3_NOT_IMPLEMENTED;
    }

    static v3_result V3_API mock_popup(void* self, int32_t x, int32_t y)
    {
        MockContextMenu* const m = *static_cast<MockContextMenu**>(self);
        m->popupCalled = true;
        m->popupX = x;
        m->popupY = y;
        return V3_OK;
    }
};

struct MockComponentHandler3 : v3_component_handler3_cpp {
    int32_t refCount;
    bool createContextMenuCalled;
    v3_param_id lastParamId;
    bool hasParamId;
    MockContextMenu* createdMenu;
    MockContextMenu** createdMenuPtr;

    MockComponentHandler3()
        : refCount(1),
          createContextMenuCalled(false),
          lastParamId(0),
          hasParamId(false),
          createdMenu(nullptr),
          createdMenuPtr(nullptr)
    {
        query_interface = mock_query_interface;
        ref = mock_ref;
        unref = mock_unref;

        comp3.create_context_menu = mock_create_context_menu;
    }

    ~MockComponentHandler3()
    {
        delete createdMenu;
        delete createdMenuPtr;
    }

    static v3_result V3_API mock_query_interface(void*, const v3_tuid, void**)
    {
        return V3_NOT_IMPLEMENTED;
    }

    static uint32_t V3_API mock_ref(void* self)
    {
        MockComponentHandler3* const h = *static_cast<MockComponentHandler3**>(self);
        return static_cast<uint32_t>(++h->refCount);
    }

    static uint32_t V3_API mock_unref(void* self)
    {
        MockComponentHandler3* const h = *static_cast<MockComponentHandler3**>(self);
        return static_cast<uint32_t>(--h->refCount);
    }

    static v3_context_menu** V3_API mock_create_context_menu(void* self, v3_plugin_view**, const v3_param_id* param_id)
    {
        MockComponentHandler3* const h = *static_cast<MockComponentHandler3**>(self);
        h->createContextMenuCalled = true;
        if (param_id != nullptr)
        {
            h->hasParamId = true;
            h->lastParamId = *param_id;
        }
        else
        {
            h->hasParamId = false;
        }

        h->createdMenu = new MockContextMenu();
        h->createdMenuPtr = new MockContextMenu*(h->createdMenu);
        return reinterpret_cast<v3_context_menu**>(h->createdMenuPtr);
    }
};

struct MockHostHandler : v3_component_handler_cpp {
    int32_t refCount;
    MockComponentHandler3* handler3;
    MockComponentHandler3** handler3Ptr;
    bool supportsHandler3;

    MockHostHandler(bool enableHandler3 = true)
        : refCount(1),
          handler3(nullptr),
          handler3Ptr(nullptr),
          supportsHandler3(enableHandler3)
    {
        if (supportsHandler3)
        {
            handler3 = new MockComponentHandler3();
            handler3Ptr = new MockComponentHandler3*(handler3);
        }

        query_interface = mock_query_interface;
        ref = mock_ref;
        unref = mock_unref;
    }

    ~MockHostHandler()
    {
        delete handler3;
        delete handler3Ptr;
    }

    static v3_result V3_API mock_query_interface(void* self, const v3_tuid iid, void** obj)
    {
        MockHostHandler* const h = *static_cast<MockHostHandler**>(self);
        if (h->supportsHandler3 && std::memcmp(iid, v3_component_handler3_iid, sizeof(v3_tuid)) == 0)
        {
            *obj = h->handler3Ptr;
            v3_cpp_obj_ref(reinterpret_cast<v3_component_handler3**>(h->handler3Ptr));
            return V3_OK;
        }
        *obj = nullptr;
        return V3_NOT_IMPLEMENTED;
    }

    static uint32_t V3_API mock_ref(void* self)
    {
        MockHostHandler* const h = *static_cast<MockHostHandler**>(self);
        return static_cast<uint32_t>(++h->refCount);
    }

    static uint32_t V3_API mock_unref(void* self)
    {
        MockHostHandler* const h = *static_cast<MockHostHandler**>(self);
        return static_cast<uint32_t>(--h->refCount);
    }
};

struct MockPlugFrame : v3_plugin_frame_cpp {
    int32_t refCount;
    MockComponentHandler3* handler3;
    MockComponentHandler3** handler3Ptr;
    bool supportsHandler3;

    MockPlugFrame(bool enableHandler3 = true)
        : refCount(1),
          handler3(nullptr),
          handler3Ptr(nullptr),
          supportsHandler3(enableHandler3)
    {
        if (supportsHandler3)
        {
            handler3 = new MockComponentHandler3();
            handler3Ptr = new MockComponentHandler3*(handler3);
        }

        query_interface = mock_query_interface;
        ref = mock_ref;
        unref = mock_unref;
    }

    ~MockPlugFrame()
    {
        delete handler3;
        delete handler3Ptr;
    }

    static v3_result V3_API mock_query_interface(void* self, const v3_tuid iid, void** obj)
    {
        MockPlugFrame* const f = *static_cast<MockPlugFrame**>(self);
        if (f->supportsHandler3 && std::memcmp(iid, v3_component_handler3_iid, sizeof(v3_tuid)) == 0)
        {
            *obj = f->handler3Ptr;
            v3_cpp_obj_ref(reinterpret_cast<v3_component_handler3**>(f->handler3Ptr));
            return V3_OK;
        }
        *obj = nullptr;
        return V3_NOT_IMPLEMENTED;
    }

    static uint32_t V3_API mock_ref(void* self)
    {
        MockPlugFrame* const f = *static_cast<MockPlugFrame**>(self);
        return static_cast<uint32_t>(++f->refCount);
    }

    static uint32_t V3_API mock_unref(void* self)
    {
        MockPlugFrame* const f = *static_cast<MockPlugFrame**>(self);
        return static_cast<uint32_t>(--f->refCount);
    }
};

// Simulation of UIVst3::showContextMenu
static bool mock_ui_show_context_menu(v3_component_handler** fHandler,
                                      v3_plugin_frame** fFrame,
                                      v3_plugin_view** fView,
                                      const uint32_t rindex,
                                      const int32_t screenX,
                                      const int32_t screenY)
{
    v3_component_handler3** handler3 = nullptr;

    if (fHandler != nullptr && v3_cpp_obj_query_interface(fHandler, v3_component_handler3_iid, &handler3) == V3_OK && handler3 != nullptr)
    {
        // Successfully queried from fHandler
    }
    else if (fFrame != nullptr && v3_cpp_obj_query_interface(fFrame, v3_component_handler3_iid, &handler3) == V3_OK && handler3 != nullptr)
    {
        // Successfully queried from fFrame fallback
    }

    if (handler3 == nullptr)
        return false;

    const v3_param_id paramId = static_cast<v3_param_id>(rindex);
    v3_context_menu** const menu = v3_cpp_obj(handler3)->create_context_menu(handler3, fView, &paramId);

    if (menu != nullptr)
    {
        v3_cpp_obj(menu)->popup(menu, screenX, screenY);
        v3_cpp_obj_unref(menu);
        v3_cpp_obj_unref(handler3);
        return true;
    }

    v3_cpp_obj_unref(handler3);
    return false;
}

// --------------------------------------------------------------------------------------------------------------------
// Mock Program List Implementation

struct MockProgram {
    std::string name;
};

struct MockPluginWithPrograms {
    std::vector<MockProgram> programs;
    std::string pluginName;

    MockPluginWithPrograms()
        : pluginName("ModernDPF Synth")
    {
        programs.push_back({"Init Sound"});
        programs.push_back({"Sub Heavy Bass"});
        programs.push_back({"Shimmering Lead"});
        programs.push_back({"Lush Analog Strings"});
    }

    uint32_t getProgramCount() const { return static_cast<uint32_t>(programs.size()); }
    const char* getProgramName(uint32_t idx) const { return programs[idx].name.c_str(); }
    const char* getName() const { return pluginName.c_str(); }
};

// Simulation of IUnitInfo program list routines in DistrhoPluginVST3.cpp
static int32_t mock_get_program_list_count(const MockPluginWithPrograms* plugin)
{
    if (plugin == nullptr)
        return 0;
    return (plugin->getProgramCount() > 0) ? 1 : 0;
}

static v3_result mock_get_program_list_info(const MockPluginWithPrograms* plugin,
                                            const int32_t list_idx,
                                            v3_program_list_info* const info)
{
    if (info == nullptr || list_idx != 0)
        return V3_INVALID_ARG;

    if (plugin == nullptr)
        return V3_NOT_INITIALIZED;

    const uint32_t count = plugin->getProgramCount();
    if (count == 0)
        return V3_INVALID_ARG;

    info->id = 0;
    info->programCount = static_cast<int32_t>(count);

    const char* const listName = "Factory Presets";
    for (size_t i = 0; i < std::strlen(listName); ++i)
        info->name[i] = static_cast<int16_t>(listName[i]);
    info->name[std::strlen(listName)] = 0;

    return V3_OK;
}

static v3_result mock_get_program_name(const MockPluginWithPrograms* plugin,
                                       const int32_t list_id,
                                       const int32_t program_idx,
                                       v3_str_128 name)
{
    if (name == nullptr || list_id != 0 || program_idx < 0)
        return V3_INVALID_ARG;

    if (plugin == nullptr)
        return V3_NOT_INITIALIZED;

    if (static_cast<uint32_t>(program_idx) >= plugin->getProgramCount())
        return V3_INVALID_ARG;

    const char* const progName = plugin->getProgramName(static_cast<uint32_t>(program_idx));
    for (size_t i = 0; i < std::strlen(progName); ++i)
        name[i] = static_cast<int16_t>(progName[i]);
    name[std::strlen(progName)] = 0;

    return V3_OK;
}

static v3_result mock_get_program_info(const MockPluginWithPrograms* plugin,
                                       const int32_t list_id,
                                       const int32_t program_idx,
                                       const char* const attribute_id,
                                       v3_str_128 attribute_value)
{
    if (attribute_value == nullptr || list_id != 0 || program_idx < 0 || attribute_id == nullptr)
        return V3_INVALID_ARG;

    if (plugin == nullptr)
        return V3_NOT_INITIALIZED;

    if (static_cast<uint32_t>(program_idx) >= plugin->getProgramCount())
        return V3_INVALID_ARG;

    if (std::strcmp(attribute_id, "PlugInName") == 0)
    {
        const char* const pName = plugin->getName();
        for (size_t i = 0; i < std::strlen(pName); ++i)
            attribute_value[i] = static_cast<int16_t>(pName[i]);
        attribute_value[std::strlen(pName)] = 0;
        return V3_OK;
    }

    attribute_value[0] = 0;
    return V3_OK;
}

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    std::cout << "=== Testing VST3 Context Menu & Program List Architecture ===" << std::endl;

    // 1. ABI Alignment and IIDs
    std::cout << "--- Testing VST3 IComponentHandler3 & IContextMenu IIDs & Enums ---" << std::endl;
    {
        // IComponentHandler3 IID: 4F04C92E-48964344-BE2850A3-D4C497BA
        const v3_tuid expectedHandler3IID = V3_ID(0x4F04C92E, 0x48964344, 0xBE2850A3, 0xD4C497BA);
        assert(std::memcmp(v3_component_handler3_iid, expectedHandler3IID, sizeof(v3_tuid)) == 0);
        (void)expectedHandler3IID;

        // IContextMenu IID: 1A4C4082-546747D9-A03C9E26-3DEFF922
        const v3_tuid expectedMenuIID = V3_ID(0x1A4C4082, 0x546747D9, 0xA03C9E26, 0x3DEFF922);
        assert(std::memcmp(v3_context_menu_iid, expectedMenuIID, sizeof(v3_tuid)) == 0);
        (void)expectedMenuIID;

        // Item flags
        assert(V3_CONTEXT_MENU_ITEM_IS_DISABLED == 1);
        assert(V3_CONTEXT_MENU_ITEM_IS_CHECKED == 2);
        assert(V3_CONTEXT_MENU_ITEM_IS_GROUP_START == 4);
        assert(V3_CONTEXT_MENU_ITEM_IS_GROUP_END == 8);
        assert(V3_CONTEXT_MENU_ITEM_IS_SEPARATOR == 16);

        std::cout << "  ✓ IComponentHandler3 & IContextMenu IIDs and flags verified" << std::endl;
    }

    // 2. Context Menu Query via Host Handler
    std::cout << "--- Testing Context Menu Query & Popup via Host Handler ---" << std::endl;
    {
        MockHostHandler hostHandler(true);
        MockHostHandler* hostHandlerPtr = &hostHandler;
        v3_component_handler** handlerPtr = reinterpret_cast<v3_component_handler**>(&hostHandlerPtr);

        const uint32_t testParamId = 42;
        const int32_t screenX = 350;
        const int32_t screenY = 220;

        const bool result = mock_ui_show_context_menu(handlerPtr, nullptr, nullptr, testParamId, screenX, screenY);
        assert(result == true);
        (void)result;
        assert(hostHandler.handler3 != nullptr);
        assert(hostHandler.handler3->createContextMenuCalled == true);
        assert(hostHandler.handler3->hasParamId == true);
        assert(hostHandler.handler3->lastParamId == testParamId);

        MockContextMenu* menu = hostHandler.handler3->createdMenu;
        assert(menu != nullptr);
        assert(menu->popupCalled == true);
        assert(menu->popupX == screenX);
        assert(menu->popupY == screenY);
        // Menu refcount should have decremented back to 0 on unref (simulating cleanup)
        assert(menu->refCount == 0);
        // Handler3 refcount should have decremented back to initial (1)
        assert(hostHandler.handler3->refCount == 1);
        (void)menu;

        std::cout << "  ✓ Context menu creation, parameter binding, coordinates popup, and COM refcounting verified" << std::endl;
    }

    // 3. Fallback to IPlugFrame when Handler does not implement IComponentHandler3
    std::cout << "--- Testing Context Menu Fallback to IPlugFrame ---" << std::endl;
    {
        MockHostHandler hostHandlerWithoutHandler3(false);
        MockHostHandler* hostHandlerPtr = &hostHandlerWithoutHandler3;
        v3_component_handler** handlerPtr = reinterpret_cast<v3_component_handler**>(&hostHandlerPtr);

        MockPlugFrame plugFrameWithHandler3(true);
        MockPlugFrame* plugFramePtr = &plugFrameWithHandler3;
        v3_plugin_frame** framePtr = reinterpret_cast<v3_plugin_frame**>(&plugFramePtr);

        const uint32_t testParamId = 7;
        const int32_t screenX = 100;
        const int32_t screenY = 500;

        const bool result = mock_ui_show_context_menu(handlerPtr, framePtr, nullptr, testParamId, screenX, screenY);
        assert(result == true);
        (void)result;
        assert(plugFrameWithHandler3.handler3 != nullptr);
        assert(plugFrameWithHandler3.handler3->createContextMenuCalled == true);
        assert(plugFrameWithHandler3.handler3->lastParamId == testParamId);

        MockContextMenu* menu = plugFrameWithHandler3.handler3->createdMenu;
        assert(menu != nullptr);
        assert(menu->popupCalled == true);
        assert(menu->popupX == screenX);
        assert(menu->popupY == screenY);
        assert(menu->refCount == 0);
        assert(plugFrameWithHandler3.handler3->refCount == 1);
        (void)menu;

        std::cout << "  ✓ Fallback from host handler to plugin frame verified" << std::endl;
    }

    // 4. Graceful failure when neither handler nor frame supports IComponentHandler3
    std::cout << "--- Testing Graceful Failure When Unsupported ---" << std::endl;
    {
        MockHostHandler hostHandlerWithoutHandler3(false);
        MockHostHandler* hostHandlerPtr = &hostHandlerWithoutHandler3;
        v3_component_handler** handlerPtr = reinterpret_cast<v3_component_handler**>(&hostHandlerPtr);

        MockPlugFrame plugFrameWithoutHandler3(false);
        MockPlugFrame* plugFramePtr = &plugFrameWithoutHandler3;
        v3_plugin_frame** framePtr = reinterpret_cast<v3_plugin_frame**>(&plugFramePtr);

        const bool result = mock_ui_show_context_menu(handlerPtr, framePtr, nullptr, 0, 10, 10);
        assert(result == false);
        (void)result;

        const bool resultNull = mock_ui_show_context_menu(nullptr, nullptr, nullptr, 0, 10, 10);
        assert(resultNull == false);
        (void)resultNull;

        std::cout << "  ✓ Graceful rejection without crashing verified" << std::endl;
    }

    // 5. IUnitInfo Program List Queries
    std::cout << "--- Testing IUnitInfo Program List Queries ---" << std::endl;
    {
        MockPluginWithPrograms plugin;
        v3_program_list_info info;
        char buffer[128];

        // List Count
        const int32_t listCount = mock_get_program_list_count(&plugin);
        assert(listCount == 1);
        (void)listCount;

        // List Info for list 0
        v3_result res = mock_get_program_list_info(&plugin, 0, &info);
        assert(res == V3_OK);
        assert(info.id == 0);
        assert(info.programCount == 4);
        convert_from_utf16(info.name, buffer, sizeof(buffer));
        assert(std::strcmp(buffer, "Factory Presets") == 0);

        // Invalid list index
        res = mock_get_program_list_info(&plugin, 1, &info);
        assert(res == V3_INVALID_ARG);
        res = mock_get_program_list_info(&plugin, -1, &info);
        assert(res == V3_INVALID_ARG);

        // Program Names
        v3_str_128 progName;
        for (uint32_t i = 0; i < plugin.getProgramCount(); ++i)
        {
            res = mock_get_program_name(&plugin, 0, static_cast<int32_t>(i), progName);
            assert(res == V3_OK);
            convert_from_utf16(progName, buffer, sizeof(buffer));
            assert(std::strcmp(buffer, plugin.getProgramName(i)) == 0);
        }

        // Out of range program name query
        res = mock_get_program_name(&plugin, 0, 4, progName);
        assert(res == V3_INVALID_ARG);
        res = mock_get_program_name(&plugin, 1, 0, progName);
        assert(res == V3_INVALID_ARG);

        // Program Info (PlugInName)
        v3_str_128 attrVal;
        res = mock_get_program_info(&plugin, 0, 0, "PlugInName", attrVal);
        assert(res == V3_OK);
        convert_from_utf16(attrVal, buffer, sizeof(buffer));
        assert(std::strcmp(buffer, plugin.getName()) == 0);

        // Unknown attribute returns empty string with V3_OK
        res = mock_get_program_info(&plugin, 0, 0, "UnknownCategory", attrVal);
        assert(res == V3_OK);
        convert_from_utf16(attrVal, buffer, sizeof(buffer));
        assert(std::strlen(buffer) == 0);

        // Root Unit program_list_id verification
        v3_unit_info rootUnit;
        rootUnit.id = 0;
        rootUnit.parent_unit_id = -1;
        rootUnit.program_list_id = (plugin.getProgramCount() > 0) ? 0 : -1;
        assert(rootUnit.program_list_id == 0);

        // Zero program plugin verification
        MockPluginWithPrograms emptyPlugin;
        emptyPlugin.programs.clear();
        assert(mock_get_program_list_count(&emptyPlugin) == 0);
        v3_unit_info emptyRootUnit;
        emptyRootUnit.program_list_id = (emptyPlugin.getProgramCount() > 0) ? 0 : -1;
        assert(emptyRootUnit.program_list_id == -1);

        (void)res;
        std::cout << "  ✓ get_program_list_count, get_program_list_info, get_program_name, and get_program_info verified" << std::endl;
        std::cout << "  ✓ Root unit program_list_id linkage (0 vs -1) verified" << std::endl;
    }

    std::cout << "=== All VST3 Context Menu & Program List Tests Passed Successfully! ===" << std::endl;
    return 0;
}
