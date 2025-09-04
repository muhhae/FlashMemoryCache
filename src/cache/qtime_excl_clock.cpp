#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace QTimeExclClock {

struct QTimeExclClockMetadata {
    uint64_t last_access_time = 0;
};
class QTimeExclClockData {
   public:
    QTimeExclClockData(float p = 0.1) : quantile(1 - p) {
    }
    uint64_t GetQuantile(uint64_t new_time) {
        quantile.add(new_time);
        return quantile.get();
    }
    std::unordered_map<obj_id_t, QTimeExclClockMetadata> metadatas;

   private:
    P2Quantile quantile;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QTimeExclClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, QTimeExclClockMetadata());
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<QTimeExclClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QTimeExclClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id))
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
};
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<QTimeExclClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    float p = 0.1;
    if (params.contains("p")) {
        p = std::stof(params.at("p"));
    }
    data.CacheSpecificData.emplace<QTimeExclClock::QTimeExclClockData>(p);
}

bool NotPromoted(data::AdditionalCacheData& data, obj_id_t id, uint64_t current_time) {
    auto* cache_data = std::any_cast<QTimeExclClockData>(&data.CacheSpecificData);
    assert(cache_data);
    auto obj_last_access = cache_data->metadatas.at(id).last_access_time;
    auto time = current_time - obj_last_access;
    auto threshold = cache_data->GetQuantile(time);
    return time >= threshold;
}

void QTimeExclClockEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    while (obj_to_evict->clock.freq >= 1) {
        if (NotPromoted(additional_cache_data, obj_to_evict->obj_id, req->clock_time)) {
            break;
        }
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
}  // namespace QTimeExclClock

cache_t* QTimeExclClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = QTimeExclClockInit;
    cache->evict = QTimeExclClock::QTimeExclClockEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = QTimeExclClock::OnAccess;
    data.OnEvictionCallback = QTimeExclClock::OnEviction;
    data.OnInsertCallback = QTimeExclClock::OnInsert;
    data.OnIterationEndCallback = QTimeExclClock::OnIterationEnd;
    data.SetParamsCallback = QTimeExclClock::SetParams;

    return cache;
}
}  // namespace algorithm
