#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace CMClock {
struct CMClockMetadata {
    uint64_t last_access_cm = 0;
};
class CMClockData {
   public:
    CMClockData(uint64_t cache_size, float threshold_ratio) {
        cm_threshold = cache_size * (1 - threshold_ratio);
    }
    std::unordered_map<obj_id_t, CMClockMetadata> metadatas;
    uint64_t cm_threshold = 0;
    uint64_t current_cm = 0;
    bool full = false;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<CMClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, CMClockMetadata());
    cache_data->current_cm++;
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<CMClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<CMClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id))
        cache_data->metadatas.at(req->obj_id).last_access_cm = cache_data->current_cm;
};
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<CMClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    assert(params.contains("cache_size"));
    float hand_position = params.contains("h_position") ? std::stof(params.at("h_position")) : 0.1;
    uint64_t cache_size = std::stoull(params.at("cache_size"));
    data.CacheSpecificData.emplace<CMClockData>(cache_size, hand_position);
}

bool NotPromoted(data::AdditionalCacheData& data, obj_id_t id) {
    auto* cache_data = std::any_cast<CMClockData>(&data.CacheSpecificData);
    assert(cache_data);
    uint64_t last_access_cm = cache_data->current_cm - cache_data->metadatas.at(id).last_access_cm;
    // return false;
    return last_access_cm >= cache_data->cm_threshold;
}

void CMClockEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    auto* cache_data = std::any_cast<CMClockData>(&additional_cache_data.CacheSpecificData);
    cache_data->full = true;

    while (obj_to_evict->clock.freq >= 1) {
        if (NotPromoted(additional_cache_data, obj_to_evict->obj_id)) {
            break;
        }
        additional_cache_data.OnPromotion(obj_to_evict, req);
        obj_to_evict->clock.freq -= 1;
        params->n_obj_rewritten += 1;
        params->n_byte_rewritten += obj_to_evict->obj_size;
        move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
        obj_to_evict = params->q_tail;
        cache_data->current_cm++;
    }
    additional_cache_data.OnEviction(obj_to_evict, req);
    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}
}  // namespace CMClock

cache_t* CMClockInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = CMClockInit;
    cache->evict = CMClock::CMClockEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = CMClock::OnAccess;
    data.OnEvictionCallback = CMClock::OnEviction;
    data.OnInsertCallback = CMClock::OnInsert;
    data.OnIterationEndCallback = CMClock::OnIterationEnd;
    data.SetParamsCallback = CMClock::SetParams;

    return cache;
}
}  // namespace algorithm
