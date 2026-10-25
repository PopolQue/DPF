/*
 * CLAP Preset Discovery Factory
 * Part of the CLever Audio Plug-in specification
 */

#pragma once

#include "../version.h"
#include "../plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLAP_PRESET_DISCOVERY_FACTORY_ID "clap.preset-discovery-factory/2"
#define CLAP_PRESET_DISCOVERY_FACTORY_COMPAT_ID "clap.preset-discovery-factory/draft-2"

enum {
   CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT = 1 << 0,
   CLAP_PRESET_DISCOVERY_IS_USER_CONTENT    = 1 << 1,
   CLAP_PRESET_DISCOVERY_IS_DEMO_CONTENT    = 1 << 2,
   CLAP_PRESET_DISCOVERY_IS_FAVORITE        = 1 << 3,
};

enum {
   CLAP_PRESET_DISCOVERY_LOCATION_FILE   = 0,
   CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN = 1,
};

typedef struct clap_preset_discovery_filetype {
   const char *name;
   const char *description;
   const char *extension;
} clap_preset_discovery_filetype_t;

typedef struct clap_preset_discovery_location {
   uint32_t    flags;
   const char *name;
   uint32_t    kind;
   const char *location;
} clap_preset_discovery_location_t;

typedef struct clap_preset_discovery_soundpack {
   uint32_t    flags;
   const char *name;
   const char *description;
   const char *homepage_url;
   const char *user_manual_url;
   const char *vendor;
   const char *image_path;
   const char *release_timestamp;
   const char *id;
} clap_preset_discovery_soundpack_t;

typedef struct clap_preset_discovery_metadata_receiver {
   void *receiver_data;

   void(CLAP_ABI *on_error)(const struct clap_preset_discovery_metadata_receiver *receiver,
                            int32_t os_error,
                            const char *error_message);

   bool(CLAP_ABI *begin_preset)(const struct clap_preset_discovery_metadata_receiver *receiver,
                                const char *name,
                                const char *load_key);

   bool(CLAP_ABI *set_soundpack_id)(const struct clap_preset_discovery_metadata_receiver *receiver,
                                    const char *soundpack_id);

   bool(CLAP_ABI *set_flags)(const struct clap_preset_discovery_metadata_receiver *receiver,
                             uint32_t flags);

   bool(CLAP_ABI *add_creator)(const struct clap_preset_discovery_metadata_receiver *receiver,
                               const char *creator);

   bool(CLAP_ABI *set_description)(const struct clap_preset_discovery_metadata_receiver *receiver,
                                   const char *description);

   bool(CLAP_ABI *set_creation_time)(const struct clap_preset_discovery_metadata_receiver *receiver,
                                     const char *creation_time);

   bool(CLAP_ABI *set_modification_time)(const struct clap_preset_discovery_metadata_receiver *receiver,
                                         const char *modification_time);

   bool(CLAP_ABI *add_tag)(const struct clap_preset_discovery_metadata_receiver *receiver,
                           const char *tag);
} clap_preset_discovery_metadata_receiver_t;

typedef struct clap_preset_discovery_indexer {
   clap_version_t clap_version;
   const char *name;
   const char *vendor;
   const char *url;
   const char *version;
   void *indexer_data;

   bool(CLAP_ABI *declare_filetype)(const struct clap_preset_discovery_indexer *indexer,
                                    const clap_preset_discovery_filetype_t *filetype);

   bool(CLAP_ABI *declare_location)(const struct clap_preset_discovery_indexer *indexer,
                                    const clap_preset_discovery_location_t *location);

   bool(CLAP_ABI *declare_soundpack)(const struct clap_preset_discovery_indexer *indexer,
                                     const clap_preset_discovery_soundpack_t *soundpack);

   void *(CLAP_ABI *get_extension)(const struct clap_preset_discovery_indexer *indexer,
                                   const char *extension_id);
} clap_preset_discovery_indexer_t;

typedef struct clap_preset_discovery_provider_descriptor {
   clap_version_t clap_version;
   const char *id;
   const char *name;
   const char *vendor;
} clap_preset_discovery_provider_descriptor_t;

typedef struct clap_preset_discovery_provider {
   const clap_preset_discovery_provider_descriptor_t *desc;
   void *provider_data;

   bool(CLAP_ABI *init)(const struct clap_preset_discovery_provider *provider,
                        const clap_preset_discovery_indexer_t *indexer);

   void(CLAP_ABI *destroy)(const struct clap_preset_discovery_provider *provider);

   bool(CLAP_ABI *get_metadata)(const struct clap_preset_discovery_provider *provider,
                                uint32_t location_kind,
                                const char *location,
                                const clap_preset_discovery_metadata_receiver_t *receiver);

   void *(CLAP_ABI *get_extension)(const struct clap_preset_discovery_provider *provider,
                                   const char *extension_id);
} clap_preset_discovery_provider_t;

typedef struct clap_preset_discovery_factory {
   uint32_t(CLAP_ABI *count)(const struct clap_preset_discovery_factory *factory);

   const clap_preset_discovery_provider_descriptor_t *(CLAP_ABI *get_descriptor)(
      const struct clap_preset_discovery_factory *factory, uint32_t index);

   const clap_preset_discovery_provider_t *(CLAP_ABI *create)(
      const struct clap_preset_discovery_factory *factory,
      const clap_preset_discovery_indexer_t *indexer,
      const char *provider_id);
} clap_preset_discovery_factory_t;

#ifdef __cplusplus
}
#endif
