#pragma once

#include <libCacheSim/cache.h>
#include <libCacheSim/cacheObj.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cache.hpp"

namespace data {

const static std::vector<std::string> datasets_columns = {
    "obj_id",
    "rtime_since",
    "rtime_since_log",
    "rtime_since_std",
    "rtime_since_log_std",
    "vtime_since",
    "vtime_since_log",
    "vtime_since_std",
    "vtime_since_log_std",
    "rtime_between",
    "rtime_between_log",
    "rtime_between_std",
    "rtime_between_log_std",
    "clock_freq",
    "clock_freq_decayed_rtime",
    "clock_freq_decayed_vtime",
    "clock_freq_log",
    "clock_freq_std",
    "clock_freq_log_std",
    "lifetime_freq",
    "lifetime_freq_decayed_rtime",
    "lifetime_freq_decayed_vtime",
    "lifetime_freq_log",
    "lifetime_freq_std",
    "lifetime_freq_log_std",
    "obj_size_relative",
    "wasted"
};

struct ObjMetadata {
    int64_t clock_freq = 0;
    uint64_t lifetime_freq = 0;
    uint64_t last_promotion = 0;

    std::unordered_set<uint64_t> wasted_promotions;

    std::unordered_map<uint64_t, uint64_t> final_access_after_promotion;
    std::unordered_map<uint64_t, uint64_t> current_access_after_promotion;

    void Reset();

    int64_t rtime = 0;
    int64_t vtime = 0;

    int64_t obj_size_relative = 0;
    int64_t rtime_between = 0;
    int64_t obj_size = 0;

    float lifetime_freq_decayed_vtime = 0;
    float lifetime_freq_decayed_rtime = 0;
    float clock_freq_decayed_vtime = 0;
    float clock_freq_decayed_rtime = 0;
};

class RunningMeanData {
   public:
    void Track(const float X);
    float Normalize(const float X);

   public:
    float mean = 0;
    float m2 = 0;
    uint64_t n = 0;
};

class AdditionalCacheData {
   public:
    AdditionalCacheData() = default;
    void GlobalTracking(const ObjMetadata& data);
    void InsertNext(cache_obj_t* obj);
    void OnAccessTracking(ObjMetadata& data, const request_t* req);
    void BeforeEvaluationTracking(const cache_obj_t* obj, const request_t* req);
    void BeforeEvictionTracking(const cache_obj_t* obj, const request_t* req);
    void OnPromotionTracking(const cache_obj_t* obj, const request_t* req);
    std::unordered_map<std::string, float> CandidateMetadata(
        const ObjMetadata& data,
        const cache_t* cache,
        const request_t* current_req,
        const cache_obj_t* obj_to_evict
    );

   public:
    CustomCache::ChainedCache* next;

    std::ofstream datasets;
    std::unordered_map<obj_id_t, ObjMetadata> objs_metadata;

    uint64_t n_hit;
    uint64_t n_req;
    uint64_t n_promoted;
    uint64_t n_inserted;

    uint64_t max_lifetime_freq = 1;
    uint64_t max_clock_freq = 1;
    uint64_t max_rtime_between = 1;
    uint64_t max_rtime = 1;
    uint64_t max_vtime_since = 1;
    uint64_t max_rtime_since = 1;

    RunningMeanData rm_clock_freq;
    RunningMeanData rm_lifetime_freq;
    RunningMeanData rm_vtime_since;
    RunningMeanData rm_rtime_since;
    RunningMeanData rm_rtime_between;

    RunningMeanData rm_clock_freq_log;
    RunningMeanData rm_lifetime_freq_log;
    RunningMeanData rm_vtime_since_log;
    RunningMeanData rm_rtime_since_log;
    RunningMeanData rm_rtime_between_log;

    uint64_t vtime = 0;
    float decay_power = 0.7;

    uint64_t dist_optimal_treshold = std::numeric_limits<uint64_t>::max();
    bool generate_datasets;
};
class AdditionalCacheDataStorage {
   public:
    AdditionalCacheData& operator[](cache_t* cache) { return storage[cache]; }
    AdditionalCacheData& GetAdditionalCacheData(cache_t* cache) { return storage[cache]; }

    void TransferOwnership(cache_t* source, cache_t* destination) {
        if (source == destination)
            return;

        auto it = storage.find(source);
        if (it == storage.end())
            return;

        storage[destination] = std::move(it->second);
        storage.erase(it);
    }

   public:
    static AdditionalCacheDataStorage& GetStorage() {
        static AdditionalCacheDataStorage instance;
        return instance;
    }
    AdditionalCacheDataStorage(const AdditionalCacheDataStorage&) = delete;
    AdditionalCacheDataStorage& operator=(const AdditionalCacheDataStorage&) = delete;

   private:
    std::unordered_map<cache_t*, AdditionalCacheData> storage;
    AdditionalCacheDataStorage() = default;
};
}  // namespace data
