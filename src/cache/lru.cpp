#include "cache/lru.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

#include "cache/additional_data.hpp"

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
    auto& additional_cache_data =
        AdditionalData::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    additional_cache_data.InsertNext(obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_obj_t* lru::LRUFind(cache_t* cache, const request_t* req, const bool update_cache) {
    auto& additional_cache_data =
        AdditionalData::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    LRU_params_t* params = (LRU_params_t*)cache->eviction_params;
    cache_obj_t* cache_obj = cache_find_base(cache, req, update_cache);
    if (cache_obj && likely(update_cache)) {
        move_obj_to_head(&params->q_head, &params->q_tail, cache_obj);
        additional_cache_data.n_promoted++;
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
    return cache;
}
