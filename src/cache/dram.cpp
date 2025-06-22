#include "dram.hpp"
#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <iostream>
#include "common.hpp"

void dram::DramParam::InsertToMain(obj_id_t obj_id) {
	if (main_cache) {
		auto req = req_map[obj_id];
		main_cache->get(main_cache, req);
	}
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

static void LRUEvict(cache_t* cache, const request_t* req) {
	LRU_params_t* params = (LRU_params_t*)cache->eviction_params;
	cache_obj_t* obj_to_evict = params->q_tail;
	DEBUG_ASSERT(params->q_tail != NULL);

	params->q_tail = params->q_tail->queue.prev;
	if (likely(params->q_tail != NULL)) {
		params->q_tail->queue.next = NULL;
	} else {
		/* cache->n_obj has not been updated */
		DEBUG_ASSERT(cache->n_obj == 1);
		params->q_head = NULL;
	}

	auto dram_params = (dram::DramParam*)params;
	dram_params->InsertToMain(obj_to_evict->obj_id);

	cache_evict_base(cache, obj_to_evict, true);
}

cache_t* dram::LRUInit(
	const common_cache_params_t ccache_params, const char* cache_specific_params
) {
	auto cache = LRU_init(ccache_params, cache_specific_params);

	cache->cache_init = LRUInit;
	cache->find = LRUFind;
	cache->evict = LRUEvict;

	DramParam* params = new DramParam(*(Clock_params_t*)cache->eviction_params);
	free(cache->eviction_params);

	cache->eviction_params = params;
	return cache;
}
