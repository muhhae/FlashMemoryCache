#include "cache/clock.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <regex.h>
#include <sys/types.h>

#include "additional_data.hpp"

namespace Clock {
void ClockEvict(cache_t* cache, const request_t* req) {
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    while (obj_to_evict->clock.freq >= 1) {
        obj_to_evict->clock.freq -= 1;
        params->n_obj_rewritten += 1;
        params->n_byte_rewritten += obj_to_evict->obj_size;

        data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache).OnPromotion(
            obj_to_evict, req
        );

        move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
        obj_to_evict = params->q_tail;
    }

    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    auto param = static_cast<Clock_params_t*>(cache->eviction_params);
    param->max_freq = params.contains("n_bit") ? (1 << std::stoi(params.at("n_bit"))) - 1 : 1;
}
}  // namespace Clock

namespace algorithm {
cache_t* ClockInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = ClockInit;
    cache->evict = Clock::ClockEvict;
    data::AdditionalCacheDataStorage::GetStorage()
        .GetAdditionalCacheData(cache)
        .SetParamsCallback = Clock::SetParams;

    return cache;
}
}  // namespace algorithm
