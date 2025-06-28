#include "cache/lru.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

#include "common.hpp"

void lru::LRUEvict(cache_t* cache, const request_t* req) {
    LRU_params_t* params = (LRU_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    DEBUG_ASSERT(params->q_tail != NULL);

    // we can simply call remove_obj_from_list here, but for the best performance,
    // we chose to do it manually
    // remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);

    params->q_tail = params->q_tail->queue.prev;
    if (likely(params->q_tail != NULL)) {
        params->q_tail->queue.next = NULL;
    } else {
        /* cache->n_obj has not been updated */
        DEBUG_ASSERT(cache->n_obj == 1);
        params->q_head = NULL;
    }

    ((common::CustomParams*)cache->eviction_params)->InsertNext(obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_obj_t* lru::LRUFind(cache_t* cache, const request_t* req, const bool update_cache) {
    LRU_params_t* params = (LRU_params_t*)cache->eviction_params;
    cache_obj_t* cache_obj = cache_find_base(cache, req, update_cache);
    if (cache_obj && likely(update_cache)) {
        move_obj_to_head(&params->q_head, &params->q_tail, cache_obj);
        ((common::CustomParams*)params)->n_promoted++;
    }
    return cache_obj;
}

cache_t* lru::LRUInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = LRU_init(ccache_params, cache_specific_params);

    cache->cache_init = LRUInit;
    cache->evict = LRUEvict;
    cache->find = LRUFind;

    common::CustomParams* params =
        new common::CustomParams(*(Clock_params_t*)cache->eviction_params);
    free(cache->eviction_params);

    cache->eviction_params = params;
    return cache;
}
