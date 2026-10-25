#pragma once

#include "../color.h"
#include "../id.h"
#include "../plugin.h"

static CLAP_CONSTEXPR const char CLAP_EXT_PARAM_INDICATION[] = "clap.param-indication";

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clap_plugin_param_indication {
   void (CLAP_ABI *set_mapping)(const clap_plugin_t *plugin,
                                clap_id            param_id,
                                bool               has_mapping,
                                const clap_color_t *color,
                                const char         *label,
                                const char         *description);
   void (CLAP_ABI *set_automation)(const clap_plugin_t *plugin,
                                   clap_id            param_id,
                                   uint32_t           automation_state,
                                   const clap_color_t *color);
} clap_plugin_param_indication_t;

typedef struct clap_host_param_indication {
   void* reserved;
} clap_host_param_indication_t;

#ifdef __cplusplus
}
#endif
