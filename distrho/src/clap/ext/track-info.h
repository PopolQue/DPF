#pragma once

#include "../color.h"
#include "../id.h"
#include "../plugin.h"
#include "../string-sizes.h"

static CLAP_CONSTEXPR const char CLAP_EXT_TRACK_INFO[] = "clap.track-info";

#ifdef __cplusplus
extern "C" {
#endif

enum {
   CLAP_TRACK_INFO_HAS_TRACK_NAME = 1 << 0,
   CLAP_TRACK_INFO_HAS_TRACK_COLOR = 1 << 1,
   CLAP_TRACK_INFO_HAS_AUDIO_CHANNEL = 1 << 2,
   CLAP_TRACK_INFO_IS_FOR_RETURN_TRACK = 1 << 3,
   CLAP_TRACK_INFO_IS_FOR_BUS = 1 << 4,
   CLAP_TRACK_INFO_IS_FOR_MASTER = 1 << 5,
};

typedef struct clap_track_info {
   clap_id id;
   int32_t index;
   char name[CLAP_NAME_SIZE];
   clap_color_t color;
   uint32_t audio_channel_count;
   const char *audio_port_type;
   uint32_t flags;
} clap_track_info_t;

typedef struct clap_plugin_track_info {
   void (CLAP_ABI *changed)(const clap_plugin_t *plugin);
} clap_plugin_track_info_t;

typedef struct clap_host_track_info {
   bool (CLAP_ABI *get)(const clap_host_t *host, clap_track_info_t *info);
} clap_host_track_info_t;

#ifdef __cplusplus
}
#endif
