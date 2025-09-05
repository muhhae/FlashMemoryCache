
#pragma once

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

namespace algorithm {
cache_t* S3FClock_v2Init(
    const common_cache_params_t ccache_params, const char* cache_specific_params
);
}  // namespace algorithm
