#pragma once

#include <libCacheSim/cache.h>

namespace cclock {
cache_t* OfflineClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
);
void OfflineClockEvict(cache_t* cache, const request_t* req);
}  // namespace cclock
