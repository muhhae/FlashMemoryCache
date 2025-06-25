#pragma once

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
#include <vector>

#include "cache.hpp"

namespace common {

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

struct RunningMeanData {
    float mean = 0;
    float m2 = 0;
    uint64_t n = 0;
};

class CustomParams : public Clock_params_t {
   public:
    CustomParams() = default;
    CustomParams(const Clock_params_t& base) {
        *(Clock_params_t*)this = base;
    }
    void GlobalTracking(const ObjMetadata& data);
    void InsertNext(cache_obj_t* obj);

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

void OnAccessTracking(ObjMetadata& data, CustomParams* custom_params, const request_t* req);

void BeforeEvaluationTracking(
    const cache_obj_t* obj, CustomParams* custom_params, const request_t* req
);

void BeforeEvictionTracking(
    const cache_obj_t* obj, CustomParams* custom_params, const request_t* req
);

void OnPromotionTracking(const cache_obj_t* obj, CustomParams* custom_params, const request_t* req);

std::unordered_map<std::string, float> CandidateMetadata(
    const common::ObjMetadata& data,
    common::CustomParams* params,
    const cache_t* cache,
    const request_t* current_req,
    const cache_obj_t* obj_to_evict
);

void TrackRunningMean(const float X, RunningMeanData& d);
float RunningMeanNormalize(const float X, RunningMeanData& d);
}  // namespace common
