#include "dist_clock.hpp"

#include <libCacheSim/admissionAlgo.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/cacheObj.h>

#include "additional_data.hpp"

namespace algorithm {
static void DistClockEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data =
        data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;

    while (obj_to_evict->clock.freq >= 1) {
        additional_cache_data.BeforeEvaluationTracking(obj_to_evict, req);
        bool wasted = obj_to_evict->obj_id >= additional_cache_data.dist_optimal_treshold;
        if (additional_cache_data.generate_datasets) {
            auto data = additional_cache_data.objs_metadata[obj_to_evict->obj_id];
            auto features =
                additional_cache_data.ObjectFeatures(data, cache, req, obj_to_evict);
            features["wasted"] = wasted;
            for (size_t i = 0; i < data::datasets_columns.size(); i++) {
                additional_cache_data.datasets
                    << features[data::datasets_columns[i]]
                    << (i == data::datasets_columns.size() - 1 ? '\n' : ',');
            }
        }
        additional_cache_data.BeforeEvictionTracking(obj_to_evict, req);
        if (wasted)
            break;
        additional_cache_data.OnPromotionTracking(obj_to_evict, req);

        obj_to_evict->clock.freq -= 1;
        params->n_obj_rewritten += 1;
        params->n_byte_rewritten += obj_to_evict->obj_size;
        move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
        obj_to_evict = params->q_tail;
    }
    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_t* DistClockInit(
    const common_cache_params_t ccache_params,
    const char* cache_specifiadditional_cache_data
) {
    auto cache = Clock_init(ccache_params, cache_specifiadditional_cache_data);

    cache->cache_init = DistClockInit;
    cache->evict = DistClockEvict;

    return cache;
}
}  // namespace algorithm
