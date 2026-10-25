#pragma once

#include "../plugin.h"

static CLAP_CONSTEXPR const char CLAP_EXT_THREAD_POOL[] = "clap.thread-pool";

#ifdef __cplusplus
extern "C" {
#endif

/// The thread-pool extension allows a plugin to schedule tasks across the host's worker thread pool.
typedef struct clap_plugin_thread_pool {
   // Called by the host's thread pool to execute a specific task index in the range [0 .. num_tasks - 1].
   void(CLAP_ABI *exec)(const clap_plugin_t *plugin, uint32_t task_index);
} clap_plugin_thread_pool_t;

typedef struct clap_host_thread_pool {
   // Request the host to execute `num_tasks` in its thread pool.
   // This function can only be called from within clap_plugin.process().
   // Returns true if the host accepted the execution request and all tasks completed
   // before this function returns.
   bool(CLAP_ABI *request_exec)(const clap_host_t *host, uint32_t num_tasks);
} clap_host_thread_pool_t;

#ifdef __cplusplus
}
#endif
