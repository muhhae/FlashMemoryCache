#include "additional_data.hpp"

#include <libCacheSim/cacheObj.h>
#include <libCacheSim/reader.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace data {
std::unordered_map<std::string, float> AdditionalCacheData::CandidateMetadata(
    const ObjMetadata& data,
    const cache_t* cache,
    const request_t* current_req,
    const cache_obj_t* obj_to_evict
) {
    float rtime_since = current_req->clock_time - data.rtime;
    float vtime_since = vtime - data.vtime;

    rm_rtime_since.Track(rtime_since);
    rm_rtime_since_log.Track(log(rtime_since + 1));

    rm_vtime_since.Track(vtime_since);
    rm_vtime_since_log.Track(log(vtime_since + 1));

    std::unordered_map<std::string, float> features;

    features["obj_id"] = obj_to_evict->obj_id;
    features["obj_size_relative"] = (float)obj_to_evict->obj_size / cache->cache_size;

    features["rtime_since"] = rtime_since;
    features["rtime_since_std"] = rm_rtime_since.Normalize(rtime_since);
    features["rtime_since_log"] = log(rtime_since + 1);
    features["rtime_since_log_std"] = rm_rtime_since_log.Normalize(log(rtime_since + 1));

    features["vtime_since"] = vtime_since;
    features["vtime_since_std"] = rm_vtime_since.Normalize(vtime_since);
    features["vtime_since_log"] = log(vtime_since + 1);
    features["vtime_since_log_std"] = rm_vtime_since_log.Normalize(log(vtime_since + 1));

    features["rtime_between"] = data.rtime_between;
    features["rtime_between_std"] = rm_rtime_between.Normalize(data.rtime_between);
    features["rtime_between_log"] = log(data.rtime_between + 1);
    features["rtime_between_log_std"] =
        rm_rtime_between_log.Normalize(log(data.rtime_between + 1));

    features["clock_freq"] = data.clock_freq;
    features["clock_freq_decayed_rtime"] = data.clock_freq_decayed_rtime;
    features["clock_freq_decayed_vtime"] = data.clock_freq_decayed_vtime;
    features["clock_freq_std"] = rm_clock_freq.Normalize(data.clock_freq);
    features["clock_freq_log"] = log(data.clock_freq + 1);
    features["clock_freq_log_std"] =
        rm_clock_freq_log.Normalize(log(data.clock_freq + 1));

    features["lifetime_freq"] = data.lifetime_freq;
    features["lifetime_freq_decayed_rtime"] =
        data.lifetime_freq_decayed_rtime * exp(-decay_power * rtime_since);
    features["lifetime_freq_decayed_vtime"] =
        data.lifetime_freq_decayed_vtime * exp(-decay_power * vtime_since);
    features["lifetime_freq_std"] = rm_lifetime_freq.Normalize(data.lifetime_freq);
    features["lifetime_freq_log"] = log(data.lifetime_freq + 1);
    features["lifetime_freq_log_std"] =
        rm_lifetime_freq_log.Normalize(log(data.lifetime_freq + 1));

    return features;
}

void ObjMetadata::Reset() {
    obj_size_relative = 0;
    clock_freq = 0;
    rtime_between = 0;
    rtime = 0;
    obj_size = 0;
    vtime = 0;

    lifetime_freq_decayed_rtime = 0;
    lifetime_freq_decayed_vtime = 0;
    clock_freq_decayed_rtime = 0;
    clock_freq_decayed_vtime = 0;
}

void AdditionalCacheData::OnAccessTracking(ObjMetadata& data, const request_t* req) {
    n_req++;

    uint64_t rtime_since = req->clock_time - data.rtime;
    uint64_t vtime_since = vtime - data.vtime;

    data.rtime_between = req->clock_time - data.rtime;
    data.rtime = req->clock_time;
    data.vtime = vtime++;

    data.obj_size = req->obj_size;

    data.current_access_after_promotion[data.last_promotion]++;
    data.clock_freq++;
    data.lifetime_freq++;

    data.clock_freq_decayed_rtime =
        data.clock_freq_decayed_rtime * exp(-decay_power * rtime_since);
    data.clock_freq_decayed_vtime =
        data.clock_freq_decayed_vtime * exp(-decay_power * vtime_since);

    data.lifetime_freq_decayed_rtime =
        data.lifetime_freq_decayed_rtime * exp(-decay_power * rtime_since);
    data.lifetime_freq_decayed_vtime =
        data.lifetime_freq_decayed_vtime * exp(-decay_power * vtime_since);

    data.clock_freq_decayed_rtime++;
    data.clock_freq_decayed_vtime++;
    data.lifetime_freq_decayed_rtime++;
    data.lifetime_freq_decayed_vtime++;
}

void AdditionalCacheData::BeforeEvaluationTracking(
    const cache_obj_t* obj, const request_t* req
) {
    auto& data = objs_metadata[obj->obj_id];

    uint64_t rtime_since = req->clock_time - data.rtime;
    uint64_t vtime_since = vtime - data.vtime;

    data.clock_freq_decayed_rtime =
        data.clock_freq_decayed_rtime * exp(-decay_power * rtime_since);
    data.clock_freq_decayed_vtime =
        data.clock_freq_decayed_vtime * exp(-decay_power * vtime_since);
}

void AdditionalCacheData::BeforeEvictionTracking(
    const cache_obj_t* obj, const request_t* req
) {
    auto& data = objs_metadata[obj->obj_id];

    data.clock_freq_decayed_rtime = 0;
    data.clock_freq_decayed_vtime = 0;
    data.clock_freq = 0;
}

void AdditionalCacheData::OnPromotionTracking(
    const cache_obj_t* obj, const request_t* req
) {
    n_promoted++;
    auto& data = objs_metadata[obj->obj_id];
    // data.Reset();
}

void RunningMeanData::Track(const float X) {
    n++;
    float d1 = X - mean;
    mean += d1 / n;
    float d2 = X - mean;
    m2 += d1 * d2;
}

float RunningMeanData::Normalize(const float X) {
    float variance = m2 / (n - 1);
    float std = sqrt(variance);
    if (std == 0 || std::isnan(std)) {
        return 0;
    }
    float norm = (X - mean) / std;
    return norm;
}

void AdditionalCacheData::InsertNext(cache_obj_t* obj) {
    if (!next) {
        return;
    }
    next->Admit(obj, objs_metadata[obj->obj_id].clock_freq);
}

void AdditionalCacheData::GlobalTracking(const ObjMetadata& data) {
    if (data.lifetime_freq > max_lifetime_freq) {
        max_lifetime_freq = data.lifetime_freq;
    }

    if (data.clock_freq > max_clock_freq) {
        max_clock_freq = data.clock_freq;
    }

    if (data.rtime_between > max_rtime_between) {
        max_rtime_between = data.rtime_between;
    }

    rm_clock_freq.Track(data.clock_freq);
    rm_lifetime_freq.Track(data.lifetime_freq);
    rm_rtime_between.Track(data.rtime_between);

    rm_clock_freq_log.Track(log(data.clock_freq + 1));
    rm_lifetime_freq_log.Track(log(data.lifetime_freq + 1));
    rm_rtime_between_log.Track(log(data.rtime_between + 1));
}
}  // namespace data
