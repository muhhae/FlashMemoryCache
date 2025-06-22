#pragma once

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

namespace myclock {
cache_t* MyClockInit(const common_cache_params_t ccache_params, const char* cache_specific_params);
}  // namespace myclock
