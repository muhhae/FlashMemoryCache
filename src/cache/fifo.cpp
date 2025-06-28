#include "fifo.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

#include "cache/common.hpp"

cache_t* fifo::FIFOInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = FIFO_init(ccache_params, cache_specific_params);

    cache->cache_init = FIFOInit;

    common::CustomParams* params =
        new common::CustomParams(*(Clock_params_t*)cache->eviction_params);
    free(cache->eviction_params);

    cache->eviction_params = params;
    return cache;
}
