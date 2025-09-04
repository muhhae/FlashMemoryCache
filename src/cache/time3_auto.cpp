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
namespace Time3Auto {

struct Time3AutoMetadata {
    bool reinserted = false;
    uint64_t last_access_time = 0;
};
class Time3AutoData {
   public:
    Time3AutoData(uint64_t ghost_q_size, uint64_t initial_minutes = 5, uint64_t step = 1)
        : ghost_size(std::max((uint64_t)1, ghost_q_size)),
          step(step * 30),
          time_threshold(initial_minutes * 60) {
    }
    bool IsPromoted(const cache_obj_t* obj, const request_t* req) {
        auto metadata = metadatas.at(obj->obj_id);
        if (obj->clock.freq == 0) {
            time_threshold -= step * (metadata.reinserted && time_threshold >= step);
            return false;
        }

        auto time_last_access = metadata.last_access_time;
        auto time = req->clock_time - time_last_access;
        bool promoted = time < time_threshold;

        if (!promoted) {
            if (ghost_q.size() >= ghost_size) {
                ghost_map.erase(ghost_q.back());
                ghost_q.pop_back();
            }
            ghost_q.push_front(obj->obj_id);
            ghost_map[obj->obj_id] = ghost_q.begin();
        }
        metadatas[obj->obj_id].reinserted = promoted;
        return promoted;
    }
    void OnMiss(const request_t* req) {
        auto it = ghost_map.find(req->obj_id);
        if (it != ghost_map.end()) {
            ghost_q.erase(it->second);
            ghost_map.erase(it);
            time_threshold += step * (time_threshold <= UINT64_MAX - step);
        }
    }
    void OnEviction(const cache_obj_t* obj_evicted, const request_t* req) {
        metadatas.erase(obj_evicted->obj_id);
    }
    std::unordered_map<obj_id_t, Time3AutoMetadata> metadatas;

    std::list<obj_id_t> ghost_q;
    std::unordered_map<obj_id_t, std::list<obj_id_t>::iterator> ghost_map;

   private:
    uint64_t time_threshold;
    uint64_t ghost_size;
    int64_t step = 30;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<Time3AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, Time3AutoMetadata());
    cache_data->OnMiss(req);
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<Time3AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->OnEviction(obj, req);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<Time3AutoData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
}
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<Time3AutoData>(&data.CacheSpecificData);
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
    uint64_t initial_minutes = 5;
    if (params.contains("initial_minutes")) {
        step = std::stof(params.at("initial_minutes"));
    }
    float ghost_q_size = 0.1;
    if (params.contains("ghost_size")) {
        ghost_q_size = std::stof(params.at("ghost_size"));
    }
    data.CacheSpecificData.emplace<Time3Auto::Time3AutoData>(
        ghost_q_size * cache_size, initial_minutes, step
    );
}

void Time3AutoEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
    auto* cache_data = std::any_cast<Time3AutoData>(&additional_cache_data.CacheSpecificData);

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
}  // namespace Time3Auto

cache_t* Time3AutoInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = Time3AutoInit;
    cache->evict = Time3Auto::Time3AutoEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = Time3Auto::OnAccess;
    data.OnEvictionCallback = Time3Auto::OnEviction;
    data.OnInsertCallback = Time3Auto::OnInsert;
    data.OnIterationEndCallback = Time3Auto::OnIterationEnd;
    data.SetParamsCallback = Time3Auto::SetParams;

    return cache;
}
}  // namespace algorithm
