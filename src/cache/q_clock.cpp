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
namespace QClock {
struct QClockMetadata {
    uint64_t freq = 0;
};
class QClockData {
   public:
    QClockData(float p = 0.1) : quantile(p) {
    }
    uint64_t GetQuantile(uint64_t new_freq) {
        quantile.add(new_freq);
        return quantile.get();
    }
    std::unordered_map<obj_id_t, QClockMetadata> metadatas;

   private:
    P2Quantile quantile;
};
void OnInsert(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.emplace(req->obj_id, QClockMetadata());
}
void OnEviction(data::AdditionalCacheData& data, const request_t* req, const cache_obj_t* obj) {
    auto* cache_data = std::any_cast<QClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.erase(req->obj_id);
}
void OnAccess(data::AdditionalCacheData& data, const request_t* req) {
    auto* cache_data = std::any_cast<QClockData>(&data.CacheSpecificData);
    assert(cache_data);
    if (cache_data->metadatas.contains(req->obj_id))
        cache_data->metadatas.at(req->obj_id).freq++;
};
void OnIterationEnd(data::AdditionalCacheData& data) {
    auto* cache_data = std::any_cast<QClockData>(&data.CacheSpecificData);
    assert(cache_data);
    cache_data->metadatas.clear();
}
void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    float p = 0.1;
    if (params.contains("p")) {
        p = std::stof(params.at("p"));
    }
    data.CacheSpecificData.emplace<QClockData>(p);
}

bool NotPromoted(data::AdditionalCacheData& data, obj_id_t id) {
    auto* cache_data = std::any_cast<QClockData>(&data.CacheSpecificData);
    assert(cache_data);
    uint64_t freq = cache_data->metadatas.at(id).freq;
    auto threshold = cache_data->GetQuantile(freq);
    return freq < threshold;
}

void QClockEvict(cache_t* cache, const request_t* req) {
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
}  // namespace QClock

cache_t* QClockInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = QClockInit;
    cache->evict = QClock::QClockEvict;

    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);

    data.OnAccessCallback = QClock::OnAccess;
    data.OnEvictionCallback = QClock::OnEviction;
    data.OnInsertCallback = QClock::OnInsert;
    data.OnIterationEndCallback = QClock::OnIterationEnd;
    data.SetParamsCallback = QClock::SetParams;

    return cache;
}
}  // namespace algorithm
