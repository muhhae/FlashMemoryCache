#pragma once

#include <libCacheSim/cache.h>

namespace algorithm {
cache_t* OfflineClockV2Init(
    const common_cache_params_t ccache_params, const char* cache_specific_params
);
void OfflineClockV2Evict(cache_t* cache, const request_t* req);
}  // namespace algorithm
