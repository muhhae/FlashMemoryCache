#include "additional_data.hpp"

#include <libCacheSim/cacheObj.h>
#include <libCacheSim/reader.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>

namespace data {

void AdditionalCacheData::OnAccess(const request_t* req) {
    metric.req++;
    if (object_in_cache_metadatas && object_in_cache_metadatas->contains(req->obj_id)) {
        auto& object_in_cache_metadata = object_in_cache_metadatas.value()[req->obj_id];
        object_in_cache_metadata.cache_freq++;
        if (extra_metadata) [[unlikely]] {
            if (object_in_cache_metadata.cache_freq > extra_metadata->max_cache_freq) {
                extra_metadata->max_cache_freq = object_in_cache_metadata.cache_freq;
            }
            extra_metadata->rm_cache_freq.Track(object_in_cache_metadata.cache_freq);
            extra_metadata->rm_cache_freq_log.Track(log(object_in_cache_metadata.cache_freq + 1));
        }
    }
    if (object_lifetime_metadatas) {
        auto& object_lifetime_metadata = object_lifetime_metadatas.value()[req->obj_id];
        object_lifetime_metadata.lifetime_freq++;

        if (object_offline_clock_metadatas) {
            // currently_none
        }
        if (object_offline_clock_v2_metadatas) {
            auto& object_offline_clock_v2_metadata = object_offline_clock_v2_metadatas
                                                         .value()[req->obj_id];
            object_offline_clock_v2_metadata
                .current_access_after_promotion[object_lifetime_metadata.last_promotion]++;
        }

        if (extra_metadata) [[unlikely]] {
            if (object_lifetime_metadata.lifetime_freq > extra_metadata->max_lifetime_freq) {
                extra_metadata->max_lifetime_freq = object_lifetime_metadata.lifetime_freq;
            }
            extra_metadata->rm_lifetime_freq.Track(object_lifetime_metadata.lifetime_freq);
            extra_metadata->rm_lifetime_freq_log.Track(
                log(object_lifetime_metadata.lifetime_freq + 1)
            );
        }
    }
    if (object_extra_metadatas) [[unlikely]] {
        if (object_extra_metadatas->contains(req->obj_id)) {
            auto& object_extra_metadata = object_extra_metadatas.value()[req->obj_id];

            uint64_t rtime_since_access = req->clock_time - object_extra_metadata.rtime_access;
            uint64_t vtime_since_access = vtime - object_extra_metadata.vtime_access;

            object_extra_metadata
                .cache_freq_decayed_rtime = object_extra_metadata.cache_freq_decayed_rtime *
                                                exp(-decay_power * rtime_since_access) +
                                            1;
            object_extra_metadata
                .cache_freq_decayed_vtime = object_extra_metadata.cache_freq_decayed_vtime *
                                                exp(-decay_power * vtime_since_access) +
                                            1;
            if (object_extra_lifetime_metadatas) {
                auto& object_extra_lifetime_metadata = object_extra_lifetime_metadatas
                                                           .value()[req->obj_id];
                object_extra_lifetime_metadata
                    .lifetime_freq_decayed_rtime = object_extra_lifetime_metadata
                                                       .lifetime_freq_decayed_rtime *
                                                   exp(-decay_power * rtime_since_access);
                object_extra_lifetime_metadata
                    .lifetime_freq_decayed_vtime = object_extra_lifetime_metadata
                                                       .lifetime_freq_decayed_vtime *
                                                   exp(-decay_power * vtime_since_access);
            }
        }
        if (object_extra_lifetime_metadatas) {
            auto& object_extra_lifetime_metadata = object_extra_lifetime_metadatas
                                                       .value()[req->obj_id];
            object_extra_lifetime_metadata.lifetime_freq_decayed_rtime++;
            object_extra_lifetime_metadata.lifetime_freq_decayed_vtime++;
        }
    }
}
void AdditionalCacheData::OnPromotion(const cache_obj_t* obj_promoted, const request_t* req) {
    metric.reinserted++;
    metric.byte_reinserted += obj_promoted->obj_size;
    if (object_in_cache_metadatas && !object_in_cache_metadatas->contains(obj_promoted->obj_id))
        [[unlikely]] {
        throw std::runtime_error(
            "Somehow, object_in_cache_metadata is not initialized [func OnPromotion]"
        );
    }
    if (object_extra_metadatas) {
        if (!object_extra_metadatas->contains(obj_promoted->obj_id)) [[unlikely]] {
            throw std::runtime_error(
                "Somehow, object_extra_metadata is not initialized [func OnPromotion]"
            );
        }
        auto& object_extra_metadata = object_extra_metadatas.value()[obj_promoted->obj_id];
        object_extra_metadata.cache_freq_decayed_rtime = 0;
        object_extra_metadata.cache_freq_decayed_vtime = 0;
    }
    if (object_lifetime_metadatas) {
        auto& object_lifetime_metadata = object_lifetime_metadatas.value()[obj_promoted->obj_id];
        object_lifetime_metadata.last_promotion = object_lifetime_metadata.lifetime_freq;
    }
}
void AdditionalCacheData::OnEviction(const cache_obj_t* obj_evicted, const request_t* req) {
    InsertNext(obj_evicted);
    if (object_in_cache_metadatas) {
        object_in_cache_metadatas->erase(obj_evicted->obj_id);
    }
    if (object_extra_metadatas) {
        object_extra_metadatas->erase(obj_evicted->obj_id);
    }
}
void AdditionalCacheData::OnInsertion(const request_t* req) {
    metric.byte_inserted += req->obj_size;
    metric.inserted++;

    if (object_in_cache_metadatas) {
        object_in_cache_metadatas->emplace(req->obj_id, ObjectInCacheMetadata());
    }
    if (object_extra_metadatas) {
        object_extra_metadatas.value().emplace(req->obj_id, ObjectExtraMetadata());
    }
}

void AdditionalCacheData::InsertNext(const cache_obj_t* obj) {
    if (!next) {
        return;
    }
    if (!object_in_cache_metadatas && !object_lifetime_metadatas) {
        return next->Admit(obj, 0);
    }
    if (lifetime_freq_for_threshold) {
        return next->Admit(obj, object_lifetime_metadatas.value()[obj->obj_id].lifetime_freq);
    }
    next->Admit(obj, object_in_cache_metadatas.value()[obj->obj_id].cache_freq);
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

std::unordered_map<std::string, float> AdditionalCacheData::ObjectFeatures(
    const cache_obj_t* obj_to_evict, const cache_t* cache, const request_t* current_req
) {
    if (!object_in_cache_metadatas || !object_extra_metadatas || !object_offline_clock_metadatas ||
        !object_lifetime_metadatas || !object_extra_lifetime_metadatas || !extra_metadata) {
        throw std::runtime_error("All object_metadatas need to be initialized");
    }

    auto& obj_in_cache_metadata = object_in_cache_metadatas.value()[obj_to_evict->obj_id];
    auto& obj_extra_metadata = object_extra_metadatas.value()[obj_to_evict->obj_id];
    auto& obj_lifetime_metadata = object_lifetime_metadatas.value()[obj_to_evict->obj_id];
    auto& obj_extra_lifetime_metadata = object_extra_lifetime_metadatas
                                            .value()[obj_to_evict->obj_id];
    auto& obj_offline_clock_metadata = object_offline_clock_metadatas.value()[obj_to_evict->obj_id];

    float rtime_since_access = current_req->clock_time - obj_extra_metadata.rtime_access;
    float vtime_since_access = vtime - obj_extra_metadata.vtime_access;

    obj_extra_metadata.cache_freq_decayed_rtime = obj_extra_metadata.cache_freq_decayed_rtime *
                                                  exp(-decay_power * rtime_since_access);
    obj_extra_metadata.cache_freq_decayed_vtime = obj_extra_metadata.cache_freq_decayed_vtime *
                                                  exp(-decay_power * vtime_since_access);

    extra_metadata->rm_rtime_since_access.Track(rtime_since_access);
    extra_metadata->rm_rtime_since_access_log.Track(log(rtime_since_access + 1));

    extra_metadata->rm_vtime_since_access.Track(vtime_since_access);
    extra_metadata->rm_vtime_since_access_log.Track(log(vtime_since_access + 1));

    std::unordered_map<std::string, float> features;

    features["obj_id"] = obj_to_evict->obj_id;
    features["obj_size_relative"] = (float)obj_to_evict->obj_size / cache->cache_size;

    features["rtime_since_access"] = rtime_since_access;
    features["rtime_since_access_std"] = extra_metadata->rm_rtime_since_access.Normalize(
        rtime_since_access
    );
    features["rtime_since_access_log"] = log(rtime_since_access + 1);
    features["rtime_since_access_log_std"] = extra_metadata->rm_rtime_since_access_log.Normalize(
        log(rtime_since_access + 1)
    );

    features["vtime_since_access"] = vtime_since_access;
    features["vtime_since_access_std"] = extra_metadata->rm_vtime_since_access.Normalize(
        vtime_since_access
    );
    features["vtime_since_access_log"] = log(vtime_since_access + 1);
    features["vtime_since_access_log_std"] = extra_metadata->rm_vtime_since_access_log.Normalize(
        log(vtime_since_access + 1)
    );

    features["cache_freq"] = obj_in_cache_metadata.cache_freq;
    features["cache_freq_decayed_rtime"] = obj_extra_metadata.cache_freq_decayed_rtime;
    features["cache_freq_decayed_vtime"] = obj_extra_metadata.cache_freq_decayed_vtime;
    features["cache_freq_std"] = extra_metadata->rm_cache_freq.Normalize(
        obj_in_cache_metadata.cache_freq
    );
    features["cache_freq_log"] = log(obj_in_cache_metadata.cache_freq + 1);
    features["cache_freq_log_std"] = extra_metadata->rm_cache_freq_log.Normalize(
        log(obj_in_cache_metadata.cache_freq + 1)
    );

    features["lifetime_freq"] = obj_lifetime_metadata.lifetime_freq;
    features["lifetime_freq_decayed_rtime"] = obj_extra_lifetime_metadata
                                                  .lifetime_freq_decayed_rtime *
                                              exp(-decay_power * rtime_since_access);
    features["lifetime_freq_decayed_vtime"] = obj_extra_lifetime_metadata
                                                  .lifetime_freq_decayed_vtime *
                                              exp(-decay_power * vtime_since_access);
    features["lifetime_freq_std"] = extra_metadata->rm_lifetime_freq.Normalize(
        obj_lifetime_metadata.lifetime_freq
    );
    features["lifetime_freq_log"] = log(obj_lifetime_metadata.lifetime_freq + 1);
    features["lifetime_freq_log_std"] = extra_metadata->rm_lifetime_freq_log.Normalize(
        log(obj_lifetime_metadata.lifetime_freq + 1)
    );

    return features;
}
}  // namespace data
