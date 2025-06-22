#include "offline_clock.hpp"
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <cstdlib>
#include <iostream>
#include "common.hpp"

static void OfflineClockEvict(cache_t* cache, const request_t* req) {
	Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
	common::CustomClockParams* custom_params = (common::CustomClockParams*)cache->eviction_params;

	cache_obj_t* obj_to_evict = params->q_tail;
	while (obj_to_evict->clock.freq >= 1) {
		auto& data = custom_params->objs_metadata[obj_to_evict->obj_id];
		data.last_promotion = data.lifetime_freq;
		common::BeforeEvaluationTracking(obj_to_evict, custom_params, req);
		bool wasted =
			data.wasted_promotions.find(data.last_promotion) != data.wasted_promotions.end();
		if (custom_params->generate_datasets) {
			auto features =
				common::CandidateMetadata(data, custom_params, cache, req, obj_to_evict);
			features["wasted"] = wasted;
			for (size_t i = 0; i < common::datasets_columns.size(); i++) {
				custom_params->datasets << features[common::datasets_columns[i]]
										<< (i == common::datasets_columns.size() - 1 ? "\n" : ",");
			}
		}
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
	auto& data = custom_params->objs_metadata[obj_to_evict->obj_id];
	data.wasted_promotions.insert(data.last_promotion);
	remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
	cache_evict_base(cache, obj_to_evict, true);
}

cache_t* cclock::OfflineClockInit(
	const common_cache_params_t ccache_params, const char* cache_specific_params
) {
	auto custom_clock = Clock_init(ccache_params, cache_specific_params);

	custom_clock->cache_init = OfflineClockInit;
	custom_clock->evict = OfflineClockEvict;

	common::CustomClockParams* params =
		new common::CustomClockParams(*(Clock_params_t*)custom_clock->eviction_params);
	free(custom_clock->eviction_params);

	custom_clock->eviction_params = params;
	return custom_clock;
}
