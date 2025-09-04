#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "additional_data.hpp"

namespace algorithm {
struct OfflineQClockMetadata {
    uint64_t freq = 0;
};
class OfflineQClockData {
   public:
    void add(uint64_t freq) {
        if (!settled)
            freqs.push_back(freq);
    }
    void settle() {
        settled = true;
        assert(settled);
        assert(!freqs.empty());
        size_t offset = std::round(p * (freqs.size() - 1));
        auto it = freqs.begin() + offset;
        std::nth_element(freqs.begin(), it, freqs.end());
        threshold = *it;
    }
    uint64_t GetQuantile() {
        return threshold;
    }
    bool isSettled() {
        return settled;
    }
    float p = 0.1;
    std::unordered_map<obj_id_t, OfflineQClockMetadata> metadatas;

   private:
    bool settled = false;
    uint64_t threshold = 0;
    std::vector<uint64_t> freqs;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<OfflineQClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, OfflineQClockMetadata());
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<OfflineQClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<OfflineQClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id))
        cache_data->metadatas.at(req->obj_id).freq++;
};
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<OfflineQClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
    cache_data->settle();
}
void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    auto* cache_data = std::any_cast<OfflineQClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (params.contains("p")) {
        cache_data->p = std::stof(params.at("p"));
    }
}

bool NotPromoted(data::AdditionalCacheData& data, obj_id_t id) {
    auto* cache_data = std::any_cast<OfflineQClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (!cache_data->isSettled()) {
        cache_data->add(cache_data->metadatas.at(id).freq);
        return false;
    }
    auto threshold = cache_data->GetQuantile();
    return cache_data->metadatas.at(id).freq < threshold;
}

void OfflineQClockEvict(cache_t* cache, const request_t* req) {
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(cache);
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    cache_obj_t* obj_to_evict = params->q_tail;
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
    }
    additional_cache_data.OnEviction(obj_to_evict, req);
    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

cache_t* OfflineQClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = OfflineQClockInit;
    cache->evict = OfflineQClockEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    data.CacheSpecificData.emplace<OfflineQClockData>();

    data.OnAccessCallback = OnAccess;
    data.OnEvictionCallback = OnEviction;
    data.OnInsertCallback = OnInsert;
    data.OnIterationEndCallback = OnIterationEnd;
    data.SetParamsCallback = SetParams;

    return cache;
}
}  // namespace algorithm
