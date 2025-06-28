#pragma once
#include <libCacheSim/cache.h>

namespace bclock {
cache_t* ClockInit(const common_cache_params_t ccache_params, const char* cache_specific_params);
void ClockEvict(cache_t* cache, const request_t* req);
}  // namespace bclock
