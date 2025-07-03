#pragma once
#include <libCacheSim/cache.h>

namespace algorithm {
cache_t* FIFOInit(const common_cache_params_t ccache_params, const char* cache_specific_params);
}  // namespace algorithm
