/*
 * CLAP Note Name Extension
 * Part of the CLever Audio Plug-in specification
 */

#pragma once

#include "../plugin.h"
#include "../string-sizes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLAP_EXT_NOTE_NAME "clap.note-name"
#define CLAP_EXT_NOTE_NAME_COMPAT "clap.note-name/draft-2"

typedef struct clap_note_name {
   char    name[CLAP_NAME_SIZE];
   int16_t port;    // -1 for any
   int16_t key;     // 0..127, -1 for any
   int16_t channel; // 0..15, -1 for any
} clap_note_name_t;

typedef struct clap_plugin_note_name {
   // Return the number of available note names.
   // [main-thread]
   uint32_t(CLAP_ABI *count)(const clap_plugin_t *plugin);

   // Fills the note name object with the given index.
   // Returns true on success, false otherwise.
   // [main-thread]
   bool(CLAP_ABI *get)(const clap_plugin_t *plugin, uint32_t index, clap_note_name_t *note_name);
} clap_plugin_note_name_t;

typedef struct clap_host_note_name {
   // Informs the host that the note names have changed.
   // [main-thread]
   void(CLAP_ABI *changed)(const clap_host_t *host);
} clap_host_note_name_t;

#ifdef __cplusplus
}
#endif
