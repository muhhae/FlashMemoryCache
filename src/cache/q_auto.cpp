#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace QAuto {

struct QAutoMetadata {
    uint64_t last_access_time = 0;
    uint64_t freq = 0;
};
class QAutoData {
   public:
    QAutoData(uint64_t cache_size, float precision = 16, float threshold = 0.1)
        : miss_threshold(threshold) {
        float p = 1 / precision;
        time_quantiles.reserve(precision);
        freq_quantiles.reserve(precision);
        for (size_t i = 0; i < precision; i++) {
            time_quantiles.emplace_back(1 - p * i);
            freq_quantiles.emplace_back(p * i);
        }
        index = precision / 2;
        interval = cache_size / 10;
    }
    void Adjust() {
        if (current_promotion < interval / 10 && current_req < interval) {
            return;
        }
        float current_miss_ratio = (float)current_miss / current_req;
        float relative_miss_ratio = current_miss_ratio / prev_miss_ratio - 1;
        if (prev_miss_ratio == 0) {
            relative_miss_ratio = 0;
        }
        if (abs(relative_miss_ratio) > miss_threshold) {
            direction = relative_miss_ratio / abs(relative_miss_ratio) * -1;
            index += direction;
            index = std::clamp(index, static_cast<size_t>(0), time_quantiles.size() - 1);
        }

        prev_miss_ratio = current_miss_ratio;
        current_promotion = 0;
        current_req = 0;
        current_miss = 0;
    }
    void Track(uint64_t new_time, uint64_t new_freq) {
        for (size_t i = 0; i < time_quantiles.size(); i++) {
            time_quantiles[i].add(new_time);
            freq_quantiles[i].add(new_freq);
        }
    }
    bool NotPromoted(uint64_t time, uint64_t freq) {
        return time > time_quantiles[index].get() && freq < freq_quantiles[index].get();
    }
    std::unordered_map<obj_id_t, QAutoMetadata> metadatas;
    uint64_t current_promotion = 0;
    uint64_t current_miss = 0;
    uint64_t current_req = 0;

   private:
    std::vector<P2Quantile> time_quantiles;
    std::vector<P2Quantile> freq_quantiles;

    float miss_threshold = 0;

    uint64_t index = 0;
    int8_t direction = 1;

    uint64_t interval = 0;
    float prev_miss_ratio = 1;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, QAutoMetadata());
    cache_data->current_miss++;
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->current_req++;
    if (cache_data->metadatas.contains(req->obj_id)) {
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
        cache_data->metadatas.at(req->obj_id).freq++;
    }
    cache_data->Adjust();
}
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(
    data::AdditionalCacheData& data, std::unordered_map<std::string, std::string>& params
) {
    uint64_t precision = 16;
    if (params.contains("precision")) {
        precision = std::stof(params.at("precision"));
    }
    float threshold = 0.1;
    if (params.contains("threshold")) {
        threshold = std::stof(params.at("threshold"));
    }
    assert(params.contains("cache_size"));
    uint64_t cache_size = std::stof(params.at("cache_size"));
    data.CacheSpecificData.emplace<QAuto::QAutoData>(cache_size, precision, threshold);
}

bool NotPromoted(data::AdditionalCacheData& data, obj_id_t id, uint64_t current_time) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);

    cache_data->current_promotion += 1;
    cache_data->Adjust();

    auto obj_last_access = cache_data->metadatas.at(id).last_access_time;
    auto time = current_time - obj_last_access;
    auto freq = cache_data->metadatas.at(id).freq;
    cache_data->Track(time, freq);
    return cache_data->NotPromoted(time, freq);
}

void QAutoEvict(cache_t* cache, const request_t* req) {
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
}  // namespace QAuto

cache_t* QAutoInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = QAutoInit;
    cache->evict = QAuto::QAutoEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = QAuto::OnAccess;
    data.OnEvictionCallback = QAuto::OnEviction;
    data.OnInsertCallback = QAuto::OnInsert;
    data.OnIterationEndCallback = QAuto::OnIterationEnd;
    data.SetParamsCallback = QAuto::SetParams;

    return cache;
}
}  // namespace algorithm
