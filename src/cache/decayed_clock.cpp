#include "cache/decayed_clock.hpp"
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include "common.hpp"

void DecayedClockEvict(cache_t* cache, const request_t* req) {
	Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
	common::CustomParams* custom_params = (common::CustomParams*)cache->eviction_params;
	cache_obj_t* obj_to_evict = params->q_tail;
	while (obj_to_evict->clock.freq >= 1) {
		auto& data = custom_params->objs_metadata[obj_to_evict->obj_id];
		common::BeforeEvaluationTracking(obj_to_evict, custom_params, req);
		bool wasted = data.clock_freq_decayed_rtime <= 0.1;
		common::BeforeEvictionTracking(obj_to_evict, custom_params, req);
		if (wasted) {
			break;
		}
		common::OnPromotionTracking(obj_to_evict, custom_params, req);
		obj_to_evict->clock.freq -= 1;
		params->n_obj_rewritten += 1;
		params->n_byte_rewritten += obj_to_evict->obj_size;
		move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
		custom_params->n_promoted++;
		obj_to_evict = params->q_tail;
	}
	remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
	cache_evict_base(cache, obj_to_evict, true);
}

cache_t* decayed::DecayedClockInit(
	const common_cache_params_t ccache_params, const char* cache_specific_params
) {
	auto cache = Clock_init(ccache_params, cache_specific_params);

	cache->cache_init = DecayedClockInit;
	cache->evict = DecayedClockEvict;

	common::CustomParams* params =
		new common::CustomParams(*(Clock_params_t*)cache->eviction_params);
	free(cache->eviction_params);

	cache->eviction_params = params;
	return cache;
}
