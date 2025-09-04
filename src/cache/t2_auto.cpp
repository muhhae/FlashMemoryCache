#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cassert>
#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace T2Auto {

struct T2AutoMetadata {
    uint64_t last_access_time = 0;
    uint64_t freq = 0;
    bool reinserted = false;
    uint64_t freq_reinserted = 0;
    uint64_t time_reinserted = 0;
};
class T2AutoData {
   public:
    T2AutoData(uint64_t ghost_q_size, uint64_t step = 1)
        : ghost_q_size(std::max((uint64_t)1, ghost_q_size)) {
    }
    bool IsPromoted(obj_id_t obj_id, const request_t* req) {
        auto obj_last_access = metadatas.at(obj_id).last_access_time;

        auto time = req->clock_time - obj_last_access;
        auto freq = metadatas.at(obj_id).freq;

        bool time_promoted = time < time_threshold;
        bool freq_promoted = freq > freq_threshold;

        if (!freq_promoted) {
            if (freq_ghost_q.size() >= ghost_q_size / 2) {
                freq_ghost_map.erase(freq_ghost_q.back());
                freq_ghost_q.pop_back();
            }
            freq_ghost_q.push_front(obj_id);
            freq_ghost_map[obj_id] = freq_ghost_q.begin();
        }
        if (!time_promoted) {
            if (time_ghost_q.size() >= ghost_q_size / 2) {
                time_ghost_map.erase(time_ghost_q.back());
                time_ghost_q.pop_back();
            }
            time_ghost_q.push_front(obj_id);
            time_ghost_map[obj_id] = time_ghost_q.begin();
        }
        bool promoted = time_promoted && freq_promoted;
        metadatas[obj_id].reinserted = promoted;
        metadatas[obj_id].freq_reinserted = freq;
        metadatas[obj_id].time_reinserted = time;
        return promoted;
    }
    void OnMiss(const request_t* req) {
        auto freq_it = freq_ghost_map.find(req->obj_id);
        if (freq_it != freq_ghost_map.end()) {
            freq_ghost_q.erase(freq_it->second);
            freq_ghost_map.erase(freq_it);
            freq_threshold -= freq_step * (freq_threshold >= freq_step);
        }
        auto time_it = time_ghost_map.find(req->obj_id);
        if (time_it != freq_ghost_map.end()) {
            time_ghost_q.erase(time_it->second);
            time_ghost_map.erase(time_it);
            time_threshold += time_step * (time_threshold <= UINT64_MAX - time_step);
        }
    }
    void OnEviction(const cache_obj_t* obj_evicted, const request_t* req) {
        auto metadata = metadatas.at(obj_evicted->obj_id);
        if (metadata.reinserted) {
            freq_threshold += freq_step * ((freq_threshold <= UINT64_MAX - freq_step) &&
                                           (metadata.freq_reinserted > freq_threshold));
            time_threshold -= time_step * ((time_threshold >= time_step) &&
                                           (metadata.time_reinserted < time_threshold));
        }
        metadatas.erase(obj_evicted->obj_id);
    }
    std::unordered_map<obj_id_t, T2AutoMetadata> metadatas;
    std::list<obj_id_t> freq_ghost_q;
    std::list<obj_id_t> time_ghost_q;
    std::unordered_map<obj_id_t, std::list<obj_id_t>::iterator> freq_ghost_map;
    std::unordered_map<obj_id_t, std::list<obj_id_t>::iterator> time_ghost_map;

   private:
    uint64_t freq_threshold = 0;
    uint64_t time_threshold = 60 * 5;
    uint64_t ghost_q_size;

    int64_t time_step = 60;
    int64_t freq_step = 1;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<T2AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, T2AutoMetadata());
    cache_data->OnMiss(req);
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<T2AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->OnEviction(obj, req);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<T2AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id)) {
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
        cache_data->metadatas.at(req->obj_id).freq++;
    }
}
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<T2AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    assert(params.contains("cache_size"));
    uint64_t cache_size = std::stof(params.at("cache_size"));
    uint64_t step = 1;
    if (params.contains("step")) {
        step = std::stof(params.at("step"));
    }
    float ghost_q_size = 0.1;
    if (params.contains("ghost_size")) {
        ghost_q_size = std::stof(params.at("ghost_size"));
    }
    data.CacheSpecificData.emplace<T2Auto::T2AutoData>(ghost_q_size * cache_size, step);
}

void T2AutoEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    auto* cache_data = std::any_cast<T2AutoData>(&additional_cache_data.CacheSpecificData);

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
}  // namespace T2Auto

cache_t* T2AutoInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = T2AutoInit;
    cache->evict = T2Auto::T2AutoEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = T2Auto::OnAccess;
    data.OnEvictionCallback = T2Auto::OnEviction;
    data.OnInsertCallback = T2Auto::OnInsert;
    data.OnIterationEndCallback = T2Auto::OnIterationEnd;
    data.SetParamsCallback = T2Auto::SetParams;

    return cache;
}
}  // namespace algorithm
