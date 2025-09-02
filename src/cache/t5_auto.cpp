#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace T5Auto {

struct T5AutoMetadata {
    uint64_t last_access_time = 0;
    uint64_t freq = 0;
    bool freq_promoted = 0;
    bool time_promoted = 0;
};
class T5AutoData {
   public:
    T5AutoData(uint64_t ghost_q_size, float time_step, float freq_step)
        : ghost_q_size(std::max((uint64_t)1, ghost_q_size)),
          time_step(time_step),
          freq_step(freq_step) {}
    bool IsPromoted(cache_obj_t* obj, const request_t* req) {
        auto& metadata = metadatas.at(obj->obj_id);
        if (obj->clock.freq <= 0) {
            if (metadata.freq_promoted) {
                freq_threshold = freq_threshold * freq_step;
            }
            if (metadata.time_promoted) {
                time_threshold = time_threshold / time_step;
                time_threshold = std::max(1.0, time_threshold);
            }
            return false;
        }

        auto obj_last_access = metadata.last_access_time;
        auto time = req->clock_time - obj_last_access;
        auto freq = metadata.freq;

        // std::cout << "time: " << time << "\n";
        // std::cout << "time_threshold: " << time_threshold << "\n";
        // std::cout << "freq: " << freq << "\n";
        // std::cout << "freq_threshold: " << freq_threshold << "\n";

        metadata.time_promoted = time < time_threshold;
        metadata.freq_promoted = freq > freq_threshold;
        bool promoted = metadata.time_promoted || metadata.freq_promoted;
        if (!promoted) {
            if (ghost_q.size() >= ghost_q_size) {
                ghost_map.erase(ghost_q.back());
                ghost_q.pop_back();
            }
            ghost_q.push_front(obj->obj_id);
            ghost_map[obj->obj_id] = ghost_q.begin();
        }
        metadata.freq /= 2;
        return promoted;
    }
    void OnMiss(const request_t* req) {
        auto it = ghost_map.find(req->obj_id);
        if (it != ghost_map.end()) {
            freq_threshold = freq_threshold / freq_step;
            time_threshold = time_threshold * time_step;
            freq_threshold = std::max(1.0, freq_threshold);
        }
    }
    void OnEviction(const cache_obj_t* obj_evicted, const request_t* req) {
        metadatas.erase(obj_evicted->obj_id);
    }
    std::unordered_map<obj_id_t, T5AutoMetadata> metadatas;
    std::list<obj_id_t> ghost_q;
    std::unordered_map<obj_id_t, std::list<obj_id_t>::iterator> ghost_map;

   private:
    uint64_t ghost_q_size;

    double freq_threshold = 1;
    double time_threshold = 60;
    double time_step;
    double freq_step;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<T5AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, T5AutoMetadata());
    cache_data->OnMiss(req);
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<T5AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->OnEviction(obj, req);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<T5AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id)) {
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
        cache_data->metadatas.at(req->obj_id).freq++;
    }
}
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<T5AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(
    data::AdditionalCacheData& data, std::unordered_map<std::string, std::string>& params
) {
    assert(params.contains("cache_size"));
    uint64_t cache_size = std::stof(params.at("cache_size"));
    float time_step = params.contains("time_step") ? std::stof(params.at("time_step")) : 1.5;
    float freq_step = params.contains("freq_step") ? std::stof(params.at("freq_step")) : 1.5;
    float ghost_q_size = params.contains("ghost_size") ? std::stof(params.at("ghost_size")) : 0.1f;
    data.CacheSpecificData.emplace<T5Auto::T5AutoData>(
        ghost_q_size * cache_size, time_step, freq_step
    );
}

void T5AutoEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    auto* cache_data = std::any_cast<T5AutoData>(&additional_cache_data.CacheSpecificData);

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
}  // namespace T5Auto

cache_t* T5AutoInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = T5AutoInit;
    cache->evict = T5Auto::T5AutoEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = T5Auto::OnAccess;
    data.OnEvictionCallback = T5Auto::OnEviction;
    data.OnInsertCallback = T5Auto::OnInsert;
    data.OnIterationEndCallback = T5Auto::OnIterationEnd;
    data.SetParamsCallback = T5Auto::SetParams;

    return cache;
}
}  // namespace algorithm
