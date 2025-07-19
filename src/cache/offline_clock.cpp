#include "offline_clock.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "additional_data.hpp"

namespace algorithm {
void OfflineClockEvict(cache_t* cache, const request_t* req) {
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);

    cache_obj_t* obj_to_evict = params->q_tail;
    if (!additional_cache_data.object_offline_clock_metadatas ||
        !additional_cache_data.object_lifetime_metadatas) [[unlikely]] {
        throw std::runtime_error("Offline Clock and Lifetime metadata need to be initialized");
    }

    while (obj_to_evict->clock.freq >= 1) {
        auto& offline_clock_metadata = additional_cache_data.object_offline_clock_metadatas
                                           .value()[obj_to_evict->obj_id];
        auto& lifetime_metadata = additional_cache_data.object_lifetime_metadatas
                                      .value()[obj_to_evict->obj_id];

        lifetime_metadata.last_promotion = lifetime_metadata.lifetime_freq;
        bool wasted = offline_clock_metadata.wasted_promotions.contains(
            lifetime_metadata.last_promotion
        );

        if (additional_cache_data.generate_datasets) {
            auto features = additional_cache_data.ObjectFeatures(obj_to_evict, cache, req);
            features["wasted"] = wasted;
            for (size_t i = 0; i < data::datasets_columns.size(); i++) {
                additional_cache_data.datasets
                    << features[data::datasets_columns[i]]
                    << (i == data::datasets_columns.size() - 1 ? "\n" : ",");
            }
        }

        if (wasted) {
            break;
        }
        additional_cache_data.OnPromotion(obj_to_evict, req);
        obj_to_evict->clock.freq -= 1;
        params->n_obj_rewritten += 1;
        params->n_byte_rewritten += obj_to_evict->obj_size;
        move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
        obj_to_evict = params->q_tail;
    }
    auto& offline_clock_metadata = additional_cache_data.object_offline_clock_metadatas
                                       .value()[obj_to_evict->obj_id];
    auto& lifetime_metadata = additional_cache_data.object_lifetime_metadatas
                                  .value()[obj_to_evict->obj_id];
    offline_clock_metadata.wasted_promotions.insert(lifetime_metadata.last_promotion);
    additional_cache_data.OnEviction(obj_to_evict, req);

    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_t* OfflineClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);

    cache->cache_init = OfflineClockInit;
    cache->evict = OfflineClockEvict;

    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);

    additional_cache_data.object_lifetime_metadatas.emplace();
    additional_cache_data.object_offline_clock_metadatas.emplace();
    return cache;
}
}  // namespace algorithm
