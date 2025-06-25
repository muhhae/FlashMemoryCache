#include "base.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

#include "common.hpp"

void base::ClockEvict(cache_t* cache, const request_t* req) {
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;

    cache_obj_t* obj_to_evict = params->q_tail;
    auto custom_params = (common::CustomParams*)params;
    while (obj_to_evict->clock.freq >= 1) {
        obj_to_evict->clock.freq -= 1;
        params->n_obj_rewritten += 1;
        params->n_byte_rewritten += obj_to_evict->obj_size;
        move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
        obj_to_evict = params->q_tail;
        custom_params->n_promoted++;
    }

    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_t* base::ClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);

    cache->cache_init = ClockInit;
    cache->evict = ClockEvict;

    common::CustomParams* params =
        new common::CustomParams(*(Clock_params_t*)cache->eviction_params);
    free(cache->eviction_params);

    cache->eviction_params = params;
    return cache;
}

void base::LRUEvict(cache_t* cache, const request_t* req) {
    LRU_params_t* params = (LRU_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    DEBUG_ASSERT(params->q_tail != NULL);

    // we can simply call remove_obj_from_list here, but for the best performance,
    // we chose to do it manually
    // remove_obj_from_list(&params->q_head, &params->q_tail, obj)

    params->q_tail = params->q_tail->queue.prev;
    if (likely(params->q_tail != NULL)) {
        params->q_tail->queue.next = NULL;
    } else {
        /* cache->n_obj has not been updated */
        DEBUG_ASSERT(cache->n_obj == 1);
        params->q_head = NULL;
    }

#if defined(TRACK_DEMOTION)
    if (cache->track_demotion)
        printf(
            "%ld demote %ld %ld\n",
            cache->n_req,
            obj_to_evict->create_time,
            obj_to_evict->misc.next_access_vtime
        );
#endif
    ((common::CustomParams*)cache->eviction_params)->InsertNext(obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_obj_t* base::LRUFind(cache_t* cache, const request_t* req, const bool update_cache) {
    LRU_params_t* params = (LRU_params_t*)cache->eviction_params;
    cache_obj_t* cache_obj = cache_find_base(cache, req, update_cache);
    if (cache_obj && likely(update_cache)) {
        move_obj_to_head(&params->q_head, &params->q_tail, cache_obj);
        ((common::CustomParams*)params)->n_promoted++;
    }
    return cache_obj;
}

cache_t* base::LRUInit(
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

cache_t* base::FIFOInit(
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
