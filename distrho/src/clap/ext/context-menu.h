#pragma once

#include "../plugin.h"

static CLAP_CONSTEXPR const char CLAP_EXT_CONTEXT_MENU[] = "clap.context-menu";
static CLAP_CONSTEXPR const char CLAP_EXT_CONTEXT_MENU_COMPAT[] = "clap.context-menu.draft/0";

#ifdef __cplusplus
extern "C" {
#endif

enum {
   CLAP_CONTEXT_MENU_TARGET_KIND_GLOBAL = 0,
   CLAP_CONTEXT_MENU_TARGET_KIND_PARAM  = 1,
};

typedef struct clap_context_menu_target {
   uint32_t kind;
   clap_id  id; ///< parameter id when kind == CLAP_CONTEXT_MENU_TARGET_KIND_PARAM
} clap_context_menu_target_t;

/// The context-menu extension allows the plugin to request the host to display
/// its native context menu (e.g. parameter automation, modulation, MIDI learn).
typedef struct clap_host_context_menu {
   /// Request the host to display a context menu for a specific target.
   /// @param screen_x Absolute or window-relative horizontal position.
   /// @param screen_y Absolute or window-relative vertical position.
   /// @return true if the host displayed or handled the menu request.
   bool(CLAP_ABI *popup)(const clap_host_t *host, const clap_context_menu_target_t *target, int32_t screen_x, int32_t screen_y);
} clap_host_context_menu_t;

typedef struct clap_plugin_context_menu {
   /// Optional callback for plugin-specific context menu handling.
   bool(CLAP_ABI *can_popup)(const clap_plugin_t *plugin);
} clap_plugin_context_menu_t;

#ifdef __cplusplus
}
#endif
