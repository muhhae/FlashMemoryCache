#include "offline_clock.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

#include <cstdlib>
#include <iostream>

#include "cache/additional_data.hpp"

namespace algorithm {
void OfflineClockEvict(cache_t* cache, const request_t* req) {
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    auto& additional_cache_data =
        data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    cache_obj_t* obj_to_evict = params->q_tail;
    while (obj_to_evict->clock.freq >= 1) {
        auto& data = additional_cache_data.objs_metadata[obj_to_evict->obj_id];
        data.last_promotion = data.lifetime_freq;

        additional_cache_data.BeforeEvaluationTracking(obj_to_evict, req);
        bool wasted = data.wasted_promotions.contains(data.last_promotion);

        if (additional_cache_data.generate_datasets) {
            auto features =
                additional_cache_data.ObjectFeatures(data, cache, req, obj_to_evict);
            features["wasted"] = wasted;
            for (size_t i = 0; i < data::datasets_columns.size(); i++) {
                additional_cache_data.datasets
                    << features[data::datasets_columns[i]]
                    << (i == data::datasets_columns.size() - 1 ? "\n" : ",");
            }
        }

        additional_cache_data.BeforeEvictionTracking(obj_to_evict, req);
        if (wasted) {
            break;
        }
        additional_cache_data.OnPromotionTracking(obj_to_evict, req);

        obj_to_evict->clock.freq -= 1;
        params->n_obj_rewritten += 1;
        params->n_byte_rewritten += obj_to_evict->obj_size;
        move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
        obj_to_evict = params->q_tail;
    }
    auto& data = additional_cache_data.objs_metadata[obj_to_evict->obj_id];
    data.wasted_promotions.insert(data.last_promotion);
    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_t* OfflineClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);

    cache->cache_init = OfflineClockInit;
    cache->evict = OfflineClockEvict;

    return cache;
}
}  // namespace algorithm
