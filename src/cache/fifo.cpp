#include "fifo.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

cache_t* fifo::FIFOInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = FIFO_init(ccache_params, cache_specific_params);
    cache->cache_init = FIFOInit;
    return cache;
}
