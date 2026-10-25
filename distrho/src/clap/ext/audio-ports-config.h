/*
 * CLAP Audio Ports Config Extension
 * Part of the CLever Audio Plug-in specification
 */

#pragma once

#include "../plugin.h"
#include "../string-sizes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLAP_EXT_AUDIO_PORTS_CONFIG "clap.audio-ports-config"
#define CLAP_EXT_AUDIO_PORTS_CONFIG_COMPAT "clap.audio-ports-config/draft-2"

typedef struct clap_audio_ports_config {
   clap_id     id;
   char        name[CLAP_NAME_SIZE];
   uint32_t    input_port_count;
   uint32_t    output_port_count;
   const char *main_input_port_type;
   const char *main_output_port_type;
   bool        has_main_input;
   bool        has_main_output;
   const char *sidechain_input_port_type;
   const char *sidechain_output_port_type;
   bool        has_sidechain_input;
   bool        has_sidechain_output;
} clap_audio_ports_config_t;

typedef struct clap_plugin_audio_ports_config {
   // Number of available configurations.
   // [main-thread]
   uint32_t(CLAP_ABI *count)(const clap_plugin_t *plugin);

   // Gets configuration info by index.
   // [main-thread]
   bool(CLAP_ABI *get)(const clap_plugin_t *plugin, uint32_t index, clap_audio_ports_config_t *config);

   // Selects the configuration.
   // Returns true on success, false otherwise.
   // [main-thread]
   bool(CLAP_ABI *select)(const clap_plugin_t *plugin, clap_id config_id);
} clap_plugin_audio_ports_config_t;

typedef struct clap_host_audio_ports_config {
   // Rescans the audio port configs.
   // [main-thread]
   void(CLAP_ABI *rescan)(const clap_host_t *host);
} clap_host_audio_ports_config_t;

#ifdef __cplusplus
}
#endif
