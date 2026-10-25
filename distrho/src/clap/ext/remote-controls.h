#pragma once

#include "../id.h"
#include "../plugin.h"
#include "../string-sizes.h"

static CLAP_CONSTEXPR const char CLAP_EXT_REMOTE_CONTROLS[] = "clap.remote-controls";

#define CLAP_REMOTE_CONTROLS_COUNT 8

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clap_remote_controls_page {
   clap_id section_name_id;
   char page_name[CLAP_NAME_SIZE];
   clap_id page_id;
   clap_id param_ids[CLAP_REMOTE_CONTROLS_COUNT];
   bool is_for_context;
} clap_remote_controls_page_t;

typedef struct clap_plugin_remote_controls {
   uint32_t (CLAP_ABI *count)(const clap_plugin_t *plugin);
   bool (CLAP_ABI *get)(const clap_plugin_t *plugin, uint32_t page_index, clap_remote_controls_page_t *page);
} clap_plugin_remote_controls_t;

typedef struct clap_host_remote_controls {
   void (CLAP_ABI *changed)(const clap_host_t *host);
   void (CLAP_ABI *suggest_page)(const clap_host_t *host, clap_id page_id);
} clap_host_remote_controls_t;

#ifdef __cplusplus
}
#endif
