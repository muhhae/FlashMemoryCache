#pragma once
#include <libCacheSim/cache.h>

namespace lru {
// LRU
cache_t* LRUInit(const common_cache_params_t ccache_params, const char* cache_specific_params);
cache_obj_t* LRUFind(cache_t* cache, const request_t* req, const bool update_cache);
void LRUEvict(cache_t* cache, const request_t* req);
}  // namespace lru
