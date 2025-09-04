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
#include <list>
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
    QAutoData(uint64_t ghost_q_size, float precision = 16)
        : ghost_q_size(std::max((uint64_t)1, ghost_q_size)) {
        time_quantiles.reserve(precision + 1);
        freq_quantiles.reserve(precision + 1);
        float p = 1 / precision;
        for (size_t i = 0; i <= precision; i++) {
            time_quantiles.emplace_back(1 - p * i);
            freq_quantiles.emplace_back(p * i);
        }
        index = precision / 2;
    }
    void Track(uint64_t new_time, uint64_t new_freq) {
        for (size_t i = 0; i < time_quantiles.size(); i++) {
            time_quantiles[i].add(new_time);
            freq_quantiles[i].add(new_freq);
        }
    }
    bool IsPromoted(obj_id_t obj_id, const request_t* req) {
        auto obj_last_access = metadatas.at(obj_id).last_access_time;
        auto time = req->clock_time - obj_last_access;
        auto freq = metadatas.at(obj_id).freq;

        Track(time, freq);
        bool promoted = time < time_quantiles[index].get() || freq > freq_quantiles[index].get();

        if (!promoted) {
            if (ghost_q.size() >= ghost_q_size) {
                ghost_map.erase(ghost_q.back());
                ghost_q.pop_back();
                index += (index < time_quantiles.size() - 1);
            }
            ghost_q.push_front(obj_id);
            ghost_map[obj_id] = ghost_q.begin();
        }
        return promoted;
    }
    void OnMiss(const request_t* req) {
        auto it = ghost_map.find(req->obj_id);
        if (it != ghost_map.end()) {
            ghost_q.erase(it->second);
            ghost_map.erase(it);
            index -= index > 0;
        }
    }
    std::unordered_map<obj_id_t, QAutoMetadata> metadatas;
    std::list<obj_id_t> ghost_q;
    std::unordered_map<obj_id_t, std::list<obj_id_t>::iterator> ghost_map;
    uint64_t index;

   private:
    std::vector<P2Quantile> time_quantiles;
    std::vector<P2Quantile> freq_quantiles;
    uint64_t ghost_q_size;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, QAutoMetadata());
    cache_data->OnMiss(req);
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id)) {
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
        cache_data->metadatas.at(req->obj_id).freq++;
    }
}
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<QAutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    assert(params.contains("cache_size"));
    uint64_t cache_size = std::stof(params.at("cache_size"));
    uint64_t precision = 16;
    if (params.contains("precision")) {
        precision = std::stof(params.at("precision"));
    }
    float ghost_q_size = 0.1;
    if (params.contains("ghost_size")) {
        ghost_q_size = std::stof(params.at("ghost_size"));
    }
    data.CacheSpecificData.emplace<QAuto::QAutoData>(ghost_q_size * cache_size, precision);
}

void QAutoEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    auto* cache_data = std::any_cast<QAutoData>(&additional_cache_data.CacheSpecificData);

    while (obj_to_evict->clock.freq >= 1) {
        if (!cache_data->IsPromoted(obj_to_evict->obj_id, req)) {
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
