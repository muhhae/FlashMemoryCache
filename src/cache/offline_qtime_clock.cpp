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
namespace OfflineQTimeClock {

struct OfflineQTimeClockMetadata {
    uint64_t last_access_time = 0;
};
class OfflineQTimeClockData {
   public:
    void add(uint64_t last_access_time) {
        if (!settled)
            last_access_times.push_back(last_access_time);
    }
    void settle() {
        assert(!last_access_times.empty());
        settled = true;
        p = 1 - p;
        size_t offset = std::round(p * (last_access_times.size() - 1));
        auto it = last_access_times.begin() + offset;
        std::nth_element(last_access_times.begin(), it, last_access_times.end());
        threshold = *it;
    }
    uint64_t GetQuantile() { return threshold; }
    bool isSettled() { return settled; }
    float p = 0.1;
    std::unordered_map<obj_id_t, OfflineQTimeClockMetadata> metadatas;

   private:
    bool settled = false;
    uint64_t threshold = 0;
    std::vector<uint64_t> last_access_times;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<OfflineQTimeClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.insert({req->obj_id, {}});
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<OfflineQTimeClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<OfflineQTimeClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id))
        cache_data->metadatas.at(req->obj_id).last_access_time = req->clock_time;
};
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<OfflineQTimeClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->settle();
}
void SetParams(
    data::AdditionalCacheData& data, std::unordered_map<std::string, std::string>& params
) {
    auto* cache_data = std::any_cast<OfflineQTimeClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (params.contains("p")) {
        cache_data->p = std::stof(params.at("p"));
    }
}

bool NotPromoted(data::AdditionalCacheData& data, obj_id_t id, uint64_t current_time) {
    auto* cache_data = std::any_cast<OfflineQTimeClockData>(&data.CacheSpecificData);
    assert(cache_data);
    auto obj_last_access = cache_data->metadatas.at(id).last_access_time;
    auto time = current_time - obj_last_access;
    if (!cache_data->isSettled()) {
        cache_data->add(time);
        return false;
    }
    auto threshold = cache_data->GetQuantile();
    return time > threshold;
}

void OfflineQTimeClockEvict(cache_t* cache, const request_t* req) {
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
}  // namespace OfflineQTimeClock

cache_t* OfflineQTimeClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = OfflineQTimeClockInit;
    cache->evict = OfflineQTimeClock::OfflineQTimeClockEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    data.CacheSpecificData.emplace<OfflineQTimeClock::OfflineQTimeClockData>();

    data.OnAccessCallback = OfflineQTimeClock::OnAccess;
    data.OnEvictionCallback = OfflineQTimeClock::OnEviction;
    data.OnInsertCallback = OfflineQTimeClock::OnInsert;
    data.OnIterationEndCallback = OfflineQTimeClock::OnIterationEnd;
    data.SetParamsCallback = OfflineQTimeClock::SetParams;

    return cache;
}
}  // namespace algorithm
