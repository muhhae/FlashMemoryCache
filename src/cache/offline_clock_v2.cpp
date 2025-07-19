#include "offline_clock_v2.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>

#include "additional_data.hpp"

namespace algorithm {
void OfflineClockV2Evict(cache_t* cache, const request_t* req) {
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);

    cache_obj_t* obj_to_evict = params->q_tail;
    if (!additional_cache_data.object_offline_clock_v2_metadatas ||
        !additional_cache_data.object_lifetime_metadatas) [[unlikely]] {
        throw std::runtime_error("Offline Clock V2 and Lifetime metadata need to be initialized");
    }
    while (obj_to_evict->clock.freq >= 1) {
        auto& offline_metadata = additional_cache_data.object_offline_clock_v2_metadatas
                                     .value()[obj_to_evict->obj_id];
        auto& lifetime_metadata = additional_cache_data.object_lifetime_metadatas
                                      .value()[obj_to_evict->obj_id];

        if (offline_metadata.current_access_after_promotion.contains(
                lifetime_metadata.lifetime_freq
            ) &&
            offline_metadata.current_access_after_promotion[lifetime_metadata.lifetime_freq] >
                offline_metadata.final_access_after_promotion[lifetime_metadata.lifetime_freq]) {
            offline_metadata.final_access_after_promotion
                [lifetime_metadata.lifetime_freq] = offline_metadata.current_access_after_promotion
                                                        [lifetime_metadata.lifetime_freq];
        }

        // if (data.final_access_after_promotion.contains(data.last_promotion)) {
        //     std::println(
        //         "access: {}", data.final_access_after_promotion[data.last_promotion]
        //     );
        // }

        // bool wasted = data.final_access_after_promotion[data.lifetime_freq] < 1;
        // bool wasted = data.final_access_after_promotion.contains(data.lifetime_freq) &&
        //               data.final_access_after_promotion[data.lifetime_freq] < 1;
        bool wasted = false;
        if (offline_metadata.final_access_after_promotion.contains(
                lifetime_metadata.lifetime_freq
            )) {
            uint64_t
                future_freq = offline_metadata
                                  .final_access_after_promotion[lifetime_metadata.lifetime_freq];
            bool wasted = future_freq < 1;
        }

        // if (additional_cache_data.generate_datasets) {
        //     auto features =
        //         additional_cache_data.ObjectFeatures(data, cache, req, obj_to_evict);
        //     features["wasted"] = wasted;
        //     features["access_after_promotion"] =
        //         data.final_access_after_promotion[data.last_promotion];
        //     size_t counter = 0;
        //     for (const auto& x : features) {
        //         counter++;
        //         additional_cache_data.datasets
        //             << x.first << (counter == features.size() - 1 ? "\n" : ",");
        //     }
        // }

        lifetime_metadata.last_promotion = lifetime_metadata.lifetime_freq;
        if (wasted) {
            break;
        }
        offline_metadata.current_access_after_promotion[lifetime_metadata.lifetime_freq] = 0;
        additional_cache_data.OnPromotion(obj_to_evict, req);
        obj_to_evict->clock.freq -= 1;
        params->n_obj_rewritten += 1;
        params->n_byte_rewritten += obj_to_evict->obj_size;
        move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
        obj_to_evict = params->q_tail;
    }
    additional_cache_data.OnEviction(obj_to_evict, req);
    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_t* OfflineClockV2Init(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);

    cache->cache_init = OfflineClockV2Init;
    cache->evict = OfflineClockV2Evict;

    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);

    additional_cache_data.object_lifetime_metadatas.emplace();
    additional_cache_data.object_offline_clock_v2_metadatas.emplace();
    return cache;
}
}  // namespace algorithm
