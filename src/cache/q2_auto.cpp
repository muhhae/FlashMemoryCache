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
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace Q2Auto {

struct Q2AutoMetadata {
    uint64_t last_access_time = 0;
    uint64_t freq = 0;

    bool freq_promoted = false;
    bool time_promoted = false;
};
class Q2AutoData {
   public:
    Q2AutoData(uint64_t ghost_q_size, float precision = 16)
        : ghost_q_size(std::max((uint64_t)1, ghost_q_size)) {
        time_quantiles.reserve(precision + 1);
        freq_quantiles.reserve(precision + 1);
        float p = 1 / precision;
        for (size_t i = 0; i <= precision; i++) {
            time_quantiles.emplace_back(1 - p * i);
            freq_quantiles.emplace_back(p * i);
        }
        time_index = precision / 2;
        freq_index = precision / 2;
    }
    void Track(uint64_t new_time, uint64_t new_freq) {
        for (size_t i = 0; i < time_quantiles.size(); i++) {
            time_quantiles[i].add(new_time);
            freq_quantiles[i].add(new_freq);
        }
    }
    bool IsPromoted(const cache_obj_t* obj, const request_t* req) {
        auto& metadata = metadatas.at(obj->obj_id);
        if (obj->clock.freq <= 0) {
            if (metadata.freq_promoted) {
                freq_index += (freq_index < freq_quantiles.size() - 1);
            }
            if (metadata.time_promoted) {
                time_index += (time_index < time_quantiles.size() - 1);
            }
            return false;
        }

        auto obj_last_access = metadata.last_access_time;
        auto time = req->clock_time - obj_last_access;
        auto freq = metadatas.at(obj->obj_id).freq;

        Track(time, freq);
        metadata.time_promoted = time < time_quantiles[time_index].get();
        metadata.freq_promoted = freq > freq_quantiles[freq_index].get();

        bool promoted = metadata.time_promoted || metadata.freq_promoted;
        if (!promoted) {
            if (ghost_q.size() >= ghost_q_size) {
                ghost_map.erase(ghost_q.back());
                ghost_q.pop_back();
            }
            ghost_q.push_front(obj->obj_id);
            ghost_map[obj->obj_id] = ghost_q.begin();
        }
        return promoted;
    }
    void OnMiss(const request_t* req) {
        auto it = ghost_map.find(req->obj_id);
        if (it != ghost_map.end()) {
            ghost_q.erase(it->second);
            ghost_map.erase(it);

            freq_index -= freq_index > 0;
            time_index -= time_index > 0;
        }
    }
    std::unordered_map<obj_id_t, Q2AutoMetadata> metadatas;
    std::list<obj_id_t> ghost_q;
    std::unordered_map<obj_id_t, std::list<obj_id_t>::iterator> ghost_map;

   private:
    std::vector<P2Quantile> time_quantiles;
    std::vector<P2Quantile> freq_quantiles;

    uint64_t time_index;
    uint64_t freq_index;
    uint64_t ghost_q_size;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<Q2AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, Q2AutoMetadata());
    cache_data->OnMiss(req);
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<Q2AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<Q2AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id)) {
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
        cache_data->metadatas.at(req->obj_id).freq++;
    }
}
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<Q2AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(
    data::AdditionalCacheData& data, std::unordered_map<std::string, std::string>& params
) {
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
    data.CacheSpecificData.emplace<Q2Auto::Q2AutoData>(ghost_q_size * cache_size, precision);
}

void Q2AutoEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    auto* cache_data = std::any_cast<Q2AutoData>(&additional_cache_data.CacheSpecificData);

    while (cache_data->IsPromoted(obj_to_evict, req)) {
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
}  // namespace Q2Auto

cache_t* Q2AutoInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = Q2AutoInit;
    cache->evict = Q2Auto::Q2AutoEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = Q2Auto::OnAccess;
    data.OnEvictionCallback = Q2Auto::OnEviction;
    data.OnInsertCallback = Q2Auto::OnInsert;
    data.OnIterationEndCallback = Q2Auto::OnIterationEnd;
    data.SetParamsCallback = Q2Auto::SetParams;

    return cache;
}
}  // namespace algorithm
