/*
 * CLAP Preset Load Extension
 * Part of the CLever Audio Plug-in specification
 */

#pragma once

#include "../plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLAP_EXT_PRESET_LOAD "clap.preset-load/2"
#define CLAP_EXT_PRESET_LOAD_COMPAT "clap.preset-load/draft-2"

typedef struct clap_plugin_preset_load {
   bool(CLAP_ABI *from_location)(const clap_plugin_t *plugin,
                                 uint32_t location_kind,
                                 const char *location,
                                 const char *load_key);
} clap_plugin_preset_load_t;

#ifdef __cplusplus
}
#endif
