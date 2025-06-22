#include "base.hpp"
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include "common.hpp"

static void BaseClockEvict(cache_t* cache, const request_t* req) {
	Clock_params_t* params = (Clock_params_t*)cache->eviction_params;

	cache_obj_t* obj_to_evict = params->q_tail;
	auto custom_params = (common::CustomClockParams*)params;
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

cache_t* base::BaseClockInit(
	const common_cache_params_t ccache_params, const char* cache_specific_params
) {
	auto cache = Clock_init(ccache_params, cache_specific_params);

	cache->cache_init = BaseClockInit;
	cache->evict = BaseClockEvict;

	common::CustomClockParams* params =
		new common::CustomClockParams(*(Clock_params_t*)cache->eviction_params);
	free(cache->eviction_params);

	cache->eviction_params = params;
	return cache;
}

static cache_obj_t* LRUFind(cache_t* cache, const request_t* req, const bool update_cache) {
	LRU_params_t* params = (LRU_params_t*)cache->eviction_params;
	cache_obj_t* cache_obj = cache_find_base(cache, req, update_cache);
	if (cache_obj && likely(update_cache)) {
		move_obj_to_head(&params->q_head, &params->q_tail, cache_obj);
		((common::CustomClockParams*)params)->n_promoted++;
	}
	return cache_obj;
}

cache_t* base::LRUInit(
	const common_cache_params_t ccache_params, const char* cache_specific_params
) {
	auto cache = LRU_init(ccache_params, cache_specific_params);

	cache->cache_init = LRUInit;
	cache->find = LRUFind;

	common::CustomClockParams* params =
		new common::CustomClockParams(*(Clock_params_t*)cache->eviction_params);
	free(cache->eviction_params);

	cache->eviction_params = params;
	return cache;
}

cache_t* base::FIFOInit(
	const common_cache_params_t ccache_params, const char* cache_specific_params
) {
	auto cache = FIFO_init(ccache_params, cache_specific_params);

	cache->cache_init = FIFOInit;

	common::CustomClockParams* params =
		new common::CustomClockParams(*(Clock_params_t*)cache->eviction_params);
	free(cache->eviction_params);

	cache->eviction_params = params;
	return cache;
}
