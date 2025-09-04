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
namespace QANDv2Clock {

struct QANDv2ClockMetadata {
    uint64_t last_access_time = 0;
    uint64_t freq = 0;
};
class QANDv2ClockData {
   public:
    QANDv2ClockData(float p = 0.1) : time_quantile(1 - p), freq_quantile(p) {
    }
    void Track(uint64_t new_time, uint64_t new_freq) {
        time_quantile.add(new_time);
        freq_quantile.add(new_freq);
    }
    bool NotPromoted(uint64_t time, uint64_t freq) {
        return time > time_quantile.get() && freq < freq_quantile.get();
    }
    std::unordered_map<obj_id_t, QANDv2ClockMetadata> metadatas;

   private:
    P2Quantile time_quantile;
    P2Quantile freq_quantile;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QANDv2ClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, QANDv2ClockMetadata());
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<QANDv2ClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QANDv2ClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id)) {
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
        cache_data->metadatas.at(req->obj_id).freq++;
    }
}
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<QANDv2ClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    float p = 0.1;
    if (params.contains("p")) {
        p = std::stof(params.at("p"));
    }
    data.CacheSpecificData.emplace<QANDv2Clock::QANDv2ClockData>(p);
}

bool NotPromoted(data::AdditionalCacheData& data, obj_id_t id, uint64_t current_time) {
    auto* cache_data = std::any_cast<QANDv2ClockData>(&data.CacheSpecificData);
    assert(cache_data);
    auto& metadata = cache_data->metadatas.at(id);
    auto obj_last_access = metadata.last_access_time;
    auto time = current_time - obj_last_access;
    auto freq = metadata.freq;
    cache_data->Track(time, freq);
    metadata.freq /= 2;
    return cache_data->NotPromoted(time, freq);
}

void QANDv2ClockEvict(cache_t* cache, const request_t* req) {
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
}  // namespace QANDv2Clock

cache_t* QANDv2ClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = QANDv2ClockInit;
    cache->evict = QANDv2Clock::QANDv2ClockEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = QANDv2Clock::OnAccess;
    data.OnEvictionCallback = QANDv2Clock::OnEviction;
    data.OnInsertCallback = QANDv2Clock::OnInsert;
    data.OnIterationEndCallback = QANDv2Clock::OnIterationEnd;
    data.SetParamsCallback = QANDv2Clock::SetParams;

    return cache;
}
}  // namespace algorithm
