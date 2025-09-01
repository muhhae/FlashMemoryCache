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
namespace Q3Auto {

struct Q3AutoMetadata {
    uint64_t last_access_time = 0;
    uint64_t freq = 0;
};
class Q3AutoData {
   public:
    Q3AutoData(uint64_t ghost_q_size, float precision = 16)
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
    void TrackTime(uint64_t new_time) {
        for (size_t i = 0; i < time_quantiles.size(); i++) {
            time_quantiles[i].add(new_time);
        }
    }
    void TrackFreq(uint64_t new_freq) {
        for (size_t i = 0; i < time_quantiles.size(); i++) {
            freq_quantiles[i].add(new_freq);
        }
    }
    bool IsPromoted(const cache_obj_t* obj, const request_t* req) {
        auto& metadata = metadatas.at(obj->obj_id);
        auto obj_last_access = metadata.last_access_time;
        auto time = req->clock_time - obj_last_access;
        auto freq = metadata.freq;

        if (obj->clock.freq <= 0) {
            TrackTime(time);
            uint64_t time_threshold = time_quantiles[time_index].get();

            if (time >= time_threshold) {
                if (time_ghost_q.size() >= ghost_q_size / 2) {
                    time_ghost_map.erase(time_ghost_q.back());
                    time_ghost_q.pop_back();
                }
                time_ghost_q.push_front(obj->obj_id);
                time_ghost_map[obj->obj_id] = time_ghost_q.begin();

                return false;
            }

            return true;
        }

        TrackFreq(freq);
        uint64_t freq_threshold = freq_quantiles[freq_index].get();

        if (freq <= freq_quantiles[freq_index].get()) {
            if (freq_ghost_q.size() >= ghost_q_size / 2) {
                freq_ghost_map.erase(freq_ghost_q.back());
                freq_ghost_q.pop_back();
            }
            freq_ghost_q.push_front(obj->obj_id);
            freq_ghost_map[obj->obj_id] = freq_ghost_q.begin();

            return false;
        }

        return true;
    }
    void OnMiss(const request_t* req) {
        auto time_it = time_ghost_map.find(req->obj_id);
        if (time_it != time_ghost_map.end()) {
            time_ghost_q.erase(time_it->second);
            time_ghost_map.erase(time_it);

            freq_index += freq_index < freq_quantiles.size() - 1;
            time_index -= time_index > 0;
        }
        auto freq_it = freq_ghost_map.find(req->obj_id);
        if (freq_it != freq_ghost_map.end()) {
            freq_ghost_q.erase(freq_it->second);
            freq_ghost_map.erase(freq_it);

            time_index += time_index < time_quantiles.size() - 1;
            freq_index -= freq_index > 0;
        }
    }
    std::unordered_map<obj_id_t, Q3AutoMetadata> metadatas;
    std::list<obj_id_t> freq_ghost_q;
    std::unordered_map<obj_id_t, std::list<obj_id_t>::iterator> freq_ghost_map;
    std::list<obj_id_t> time_ghost_q;
    std::unordered_map<obj_id_t, std::list<obj_id_t>::iterator> time_ghost_map;

   private:
    std::vector<P2Quantile> time_quantiles;
    std::vector<P2Quantile> freq_quantiles;

    uint64_t time_index;
    uint64_t freq_index;
    uint64_t ghost_q_size;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<Q3AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, Q3AutoMetadata());
    cache_data->OnMiss(req);
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<Q3AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<Q3AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id)) {
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
        cache_data->metadatas.at(req->obj_id).freq++;
    }
}
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<Q3AutoData>(&data.CacheSpecificData);
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
    data.CacheSpecificData.emplace<Q3Auto::Q3AutoData>(ghost_q_size * cache_size, precision);
}

void Q3AutoEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    auto* cache_data = std::any_cast<Q3AutoData>(&additional_cache_data.CacheSpecificData);

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
}  // namespace Q3Auto

cache_t* Q3AutoInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = Q3AutoInit;
    cache->evict = Q3Auto::Q3AutoEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = Q3Auto::OnAccess;
    data.OnEvictionCallback = Q3Auto::OnEviction;
    data.OnInsertCallback = Q3Auto::OnInsert;
    data.OnIterationEndCallback = Q3Auto::OnIterationEnd;
    data.SetParamsCallback = Q3Auto::SetParams;

    return cache;
}
}  // namespace algorithm
