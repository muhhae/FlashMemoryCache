#pragma once

#include <libCacheSim/cache.h>
#include <libCacheSim/cacheObj.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/plugin.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cstdint>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
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

class RunningMeanData {
   public:
    void Track(const float X);
    float Normalize(const float X);

   public:
    float mean = 0;
    float m2 = 0;
    uint64_t n = 0;
};

struct ObjectInCacheMetadata {
    uint64_t cache_freq = 0;
};
struct ObjectLifetimeMetadata {
    uint64_t lifetime_freq = 0;
    uint64_t last_promotion = 0;
};
struct ObjectOfflineClockMetadata {
    std::unordered_set<uint64_t> wasted_promotions;
};
struct ObjectOfflineClockV2Metadata {
    std::unordered_map<uint64_t, uint64_t> final_access_after_promotion;
    std::unordered_map<uint64_t, uint64_t> current_access_after_promotion;
};
struct ObjectExtraMetadata {
    uint64_t rtime_insert = 0;
    uint64_t vtime_insert = 0;
    uint64_t rtime_access = 0;
    uint64_t vtime_access = 0;

    float cache_freq_decayed_vtime = 0;
    float cache_freq_decayed_rtime = 0;
};
struct ObjectExtraLifetimeMetadata {
    float lifetime_freq_decayed_vtime = 0;
    float lifetime_freq_decayed_rtime = 0;
};
struct CacheExtraMetadata {
    uint64_t max_lifetime_freq = 1;
    uint64_t max_cache_freq = 1;
    uint64_t max_rtime = 1;
    uint64_t max_vtime_since_access = 1;
    uint64_t max_rtime_since_access = 1;

    RunningMeanData rm_cache_freq;
    RunningMeanData rm_lifetime_freq;
    RunningMeanData rm_vtime_since_access;
    RunningMeanData rm_rtime_since_access;

    RunningMeanData rm_cache_freq_log;
    RunningMeanData rm_lifetime_freq_log;
    RunningMeanData rm_vtime_since_access_log;
    RunningMeanData rm_rtime_since_access_log;
};

class AdditionalCacheData {
   public:
    AdditionalCacheData() = default;

    void InsertNext(const cache_obj_t* obj);
    void OnAccess(const request_t* req);
    void OnPromotion(const cache_obj_t* obj_promoted, const request_t* req);
    void OnEviction(const cache_obj_t* obj_evicted, const request_t* req);
    void OnInsertion(const request_t* req);

    std::unordered_map<std::string, float> ObjectFeatures(
        const cache_obj_t* obj_to_evict, const cache_t* cache, const request_t* current_req
    );

   public:
    CustomCache::ChainedCache* next;

    bool lifetime_freq_for_threshold = false;
    bool generate_datasets;
    std::ofstream datasets;

    uint64_t vtime = 0;
    float decay_power = 0.7;
    uint64_t dist_optimal_treshold = std::numeric_limits<uint64_t>::max();

    // Temporary metadata, only live until object evicted
    std::unordered_map<obj_id_t, ObjectInCacheMetadata> object_in_cache_metadatas;
    std::optional<std::unordered_map<obj_id_t, ObjectExtraMetadata>> object_extra_metadatas;

    // Lifetime metadata, live until iteration end
    std::optional<std::unordered_map<obj_id_t, ObjectLifetimeMetadata>> object_lifetime_metadatas;
    std::optional<std::unordered_map<obj_id_t, ObjectExtraLifetimeMetadata>>
        object_extra_lifetime_metadatas;

    // Offline metadata, live forever
    std::optional<std::unordered_map<obj_id_t, ObjectOfflineClockMetadata>>
        object_offline_clock_metadatas;
    std::optional<std::unordered_map<obj_id_t, ObjectOfflineClockV2Metadata>>
        object_offline_clock_v2_metadatas;

    CacheMetrics metric = {};
    std::optional<CacheExtraMetadata> extra_metadata;
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
