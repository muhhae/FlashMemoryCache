#include "cache.hpp"

#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/cacheObj.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "additional_data.hpp"
#include "cache/clock.hpp"
#include "cache/cm_clock.hpp"
#include "cache/d3_clock.hpp"
#include "cache/delay_clock.hpp"
#include "cache/fifo.hpp"
#include "cache/gap2_clock.hpp"
#include "cache/gap_clock.hpp"
#include "cache/lru.hpp"
#include "cache/offline_clock.hpp"
#include "cache/offline_clock_v2.hpp"
#include "cache/offline_q_clock.hpp"
#include "cache/offline_qtime_clock.hpp"
#include "cache/q2_auto.hpp"
#include "cache/q3_auto.hpp"
#include "cache/q_auto.hpp"
#include "cache/q_clock.hpp"
#include "cache/qand_clock.hpp"
#include "cache/qand_clock_v2.hpp"
#include "cache/qor_clock.hpp"
#include "cache/qtime_clock.hpp"
#include "cache/qtime_excl_clock.hpp"
#include "cache/s3fclock.hpp"
#include "cache/sxfifo.hpp"
#include "cache/t2_auto.hpp"
#include "cache/t3_auto.hpp"
#include "cache/t4_auto.hpp"
#include "cache/t5_auto.hpp"
#include "cache/t6_auto.hpp"
#include "cache/t7_auto.hpp"
#include "cache/t_auto.hpp"
#include "cache/time2_auto.hpp"
#include "cache/time3_auto.hpp"
#include "cache/time_auto.hpp"
#include "lib/json.hpp"

typedef std::function<
    cache_t*(const common_cache_params_t ccache_params, const char* cache_specific_params)>
    cache_init_func;

cache_init_func AlgoSelector(std::string algorithm) {
    std::set<std::string> disabled_algo = {"dist-optimal", "ML"};
    if (disabled_algo.contains(algorithm)) {
        throw std::invalid_argument(algorithm + " algorithm is currently disabled");
    }
    std::unordered_map<std::string, cache_init_func> simple_algorithm = {
        {"fifo", algorithm::FIFOInit},
        // {"decay", algorithm::DecayedClockInit},
        {"offline-q-clock", algorithm::OfflineQClockInit},
        {"offline-qtime-clock", algorithm::OfflineQTimeClockInit},
        {"q-clock", algorithm::QClockInit},
        {"qor-clock", algorithm::QORClockInit},
        {"qand-clock", algorithm::QANDClockInit},
        {"qand-clock-v2", algorithm::QANDv2ClockInit},
        {"qauto", algorithm::QAutoInit},
        {"q2auto", algorithm::Q2AutoInit},
        {"q3auto", algorithm::Q3AutoInit},
        {"tauto", algorithm::TAutoInit},
        {"t2auto", algorithm::T2AutoInit},
        {"t3auto", algorithm::T3AutoInit},
        {"t4auto", algorithm::T4AutoInit},
        {"t5auto", algorithm::T5AutoInit},
        {"t6auto", algorithm::T6AutoInit},
        {"t7auto", algorithm::T7AutoInit},
        {"time-auto", algorithm::TimeAutoInit},
        {"time2-auto", algorithm::Time2AutoInit},
        {"time3-auto", algorithm::Time3AutoInit},
        {"qtime-clock", algorithm::QTimeClockInit},
        {"qtime-excl-clock", algorithm::QTimeExclClockInit},
        {"offline-clock", algorithm::OfflineClockInit},
        {"offline-clock-v2", algorithm::OfflineClockV2Init},
        {"lru", algorithm::LRUInit},
        {"clock", algorithm::ClockInit},
        {"s3fclock", algorithm::S3FClockInit},
        {"dclock", algorithm::DelayClockInit},
        {"d3clock", algorithm::D3ClockInit},
        {"gclock", algorithm::GapClockInit},
        {"g2clock", algorithm::Gap2ClockInit},
        {"cm-clock", algorithm::CMClockInit},
        {"sxfifo", algorithm::SxFIFOInit},
        {"slru", SLRU_init},
        {"gdsf", GDSF_init},
    };
    if (simple_algorithm.count(algorithm))
        return simple_algorithm.at(algorithm);

    if (algorithm == "ML") {
        throw std::runtime_error("ML is currently disabled");
        // if (ml_model == "") {
        //     throw std::runtime_error("ML model need to be provided in ONNX
        //     format");
        // }
        // if (input_type == "I32") {
        //     return mlclock::MLClockInit<int32_t>;
        // }
        // if (input_type == "I64") {
        //     return mlclock::MLClockInit<int64_t>;
        // }
        // if (input_type == "F32") {
        //     return mlclock::MLClockInit<float>;
        // }
        // throw std::runtime_error("Input type is not valid");
    }
    throw std::runtime_error("algorithm not found");
}

namespace CustomCache {
ChainedCache::ChainedCache(
    std::string Algorithm,
    uint64_t cache_size,
    ChainedCache* next,
    std::filesystem::path datasets,
    uint64_t admission_treshold,
    bool generate_datasets,
    bool lifetime_freq_for_threshold,
    object_metadatas_enabled object_metadatas_enabled
)
    : next(next), algorithm(Algorithm), admission_treshold(admission_treshold) {
    self = AlgoSelector(Algorithm)({.cache_size = cache_size}, NULL);
    auto& additional_cache_data = data::AdditionalCacheDataStorage::GetStorage()
                                      .GetAdditionalCacheData(self);
    if (object_metadatas_enabled.in_cache) {
        additional_cache_data.object_in_cache_metadatas.emplace();
    }
    if (object_metadatas_enabled.lifetime) {
        additional_cache_data.object_lifetime_metadatas.emplace();
    }
    additional_cache_data.lifetime_freq_for_threshold = lifetime_freq_for_threshold;
    if (generate_datasets) {
        additional_cache_data.object_in_cache_metadatas.emplace();
        additional_cache_data.object_extra_metadatas.emplace();
        additional_cache_data.object_offline_clock_metadatas.emplace();
        additional_cache_data.object_lifetime_metadatas.emplace();
        additional_cache_data.object_extra_lifetime_metadatas.emplace();
        additional_cache_data.extra_metadata.emplace();
        additional_cache_data.datasets = std::ofstream(datasets);
        for (size_t i = 0; i < data::datasets_columns.size(); i++) {
            additional_cache_data.datasets << data::datasets_columns[i]
                                           << (i == data::datasets_columns.size() - 1 ? '\n' : ',');
        }
    }
}
void ChainedCache::SetupIteration(
    bool generate_datasets, std::unordered_map<std::string, std::string> params
) {
    tmp = clone_cache(self);

    auto& additional_cache_data_storage = data::AdditionalCacheDataStorage::GetStorage();
    additional_cache_data_storage.TransferOwnership(self, tmp);

    auto& tmp_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(tmp);

    if (isML) {
        throw std::runtime_error("ML is currently disabled");
        // auto tmp_ml_param = (mlclock::MLClockParam*)tmp_params;
        // auto ml_param = (mlclock::MLClockParam*)params;
        //
        // tmp_ml_param->session = std::move(ml_param->session);
        // tmp_ml_param->session_options = std::move(ml_param->session_options);
        // tmp_ml_param->env = std::move(ml_param->env);
        // tmp_ml_param->features_name = std::move(ml_param->features_name);
        //
        // tmp_ml_param->treshold = ml_param->treshold;
    }

    tmp_additional_cache_data.metric.hit = 0;
    tmp_additional_cache_data.metric.req = 0;
    tmp_additional_cache_data.metric.reinserted = 0;
    tmp_additional_cache_data.metric.inserted = 0;
    tmp_additional_cache_data.generate_datasets = generate_datasets;

    if (tmp_additional_cache_data.SetParamsCallback) {
        params["cache_size"] = std::to_string(self->cache_size);
        tmp_additional_cache_data.SetParamsCallback(tmp, params);
    }
    if (next) {
        tmp_additional_cache_data.next = next;
        next->SetupIteration(generate_datasets, params);
    }
}
void ChainedCache::EndIteration() {
    auto& additional_cache_data_storage = data::AdditionalCacheDataStorage::GetStorage();
    auto& tmp_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(tmp);

    metrics.push_back(tmp_additional_cache_data.metric);

    metrics_times.push_back(metrics_time);
    metrics_time.clear();

    if (tmp_additional_cache_data.object_in_cache_metadatas) {
        tmp_additional_cache_data.object_in_cache_metadatas->clear();
    }
    if (tmp_additional_cache_data.object_extra_metadatas) {
        tmp_additional_cache_data.object_extra_metadatas->clear();
    }
    if (tmp_additional_cache_data.object_lifetime_metadatas) {
        for (auto& [k, v] : tmp_additional_cache_data.object_lifetime_metadatas.value()) {
            v = {};
        }
    }
    if (tmp_additional_cache_data.object_extra_lifetime_metadatas) {
        for (auto& [k, v] : tmp_additional_cache_data.object_extra_lifetime_metadatas.value()) {
            v = {};
        }
    }

    if (tmp_additional_cache_data.OnIterationEndCallback)
        tmp_additional_cache_data.OnIterationEndCallback(tmp_additional_cache_data);

    additional_cache_data_storage.TransferOwnership(tmp, self);

    if (isML) {
        throw std::runtime_error("ML is currently disabled");
        // auto tmp_ml_param = (mlclock::MLClockParam*)tmp_params;
        // auto ml_param = (mlclock::MLClockParam*)params;
        // ml_param->session = std::move(tmp_ml_param->session);
        // ml_param->session_options = std::move(tmp_ml_param->session_options);
        // ml_param->env = std::move(tmp_ml_param->env);
        // ml_param->features_name = std::move(tmp_ml_param->features_name);
    }
    tmp->cache_free(tmp);
    if (next)
        next->EndIteration();
}
void ChainedCache::Admit(const request_t* req, const uint64_t freq) {
    if (freq < admission_treshold)
        return;
    if (!tmp->get(tmp, req)) {
        data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(tmp).OnInsertion(req);
    }
}
void ChainedCache::Admit(const request_t* req) {
    auto& additional_cache_data_storage = data::AdditionalCacheDataStorage::GetStorage();
    auto& tmp_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(tmp);

    uint64_t freq = 0;
    if (tmp_additional_cache_data.object_lifetime_metadatas) {
        freq = tmp_additional_cache_data.object_lifetime_metadatas.value()[req->obj_id]
                   .lifetime_freq;
    }
    Admit(req, freq);
}
void ChainedCache::Admit(const cache_obj_t* obj, const uint64_t freq) {
    request_t req;
    copy_cache_obj_to_request(&req, obj);
    Admit(&req, freq);
}

bool ChainedCache::LookUp(const request_t* req, bool update_cache_state) {
    auto& additional_cache_data_storage = data::AdditionalCacheDataStorage::GetStorage();
    auto& tmp_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(tmp);

    bool hit = tmp->find(tmp, req, false);
    if (!hit) {
        tmp_additional_cache_data.metric.byte_miss += req->obj_size;
        if (update_cache_state) {
            Admit(req);
        }
    } else {
        tmp_additional_cache_data.metric.byte_read += req->obj_size;
        tmp_additional_cache_data.metric.hit++;
        if (update_cache_state) {
            tmp->get(tmp, req);
        }
    }
    tmp_additional_cache_data.OnAccess(req);
    return hit;
}

bool ChainedCache::Get(const request_t* req) {
    bool hit = LookUp(req, true);
    if (hit)
        return true;
    if (next)
        return next->Find(req);
    return false;
}

bool ChainedCache::Find(const request_t* req) {
    bool hit = LookUp(req, false);
    if (hit)
        return true;
    if (next)
        return next->Find(req);
    return false;
}

void ChainedCache::TrackMetricsTime(uint64_t time) {
    if (time - prev_time < 3600)
        return;

    CacheMetrics current_metrics = data::AdditionalCacheDataStorage::GetStorage()
                                       .GetAdditionalCacheData(tmp)
                                       .metric;
    CacheMetrics delta_metrics = {
        current_metrics.req - prev_metrics.req,
        current_metrics.hit - prev_metrics.hit,
        current_metrics.inserted - prev_metrics.inserted,
        current_metrics.reinserted - prev_metrics.reinserted,
        current_metrics.byte_read - prev_metrics.byte_read,
        current_metrics.byte_miss - prev_metrics.byte_miss,
        current_metrics.byte_reinserted - prev_metrics.byte_reinserted,
        current_metrics.byte_inserted - prev_metrics.byte_inserted,
    };

    metrics_time.push_back(delta_metrics);
    prev_metrics = current_metrics;
    prev_time = time;

    if (next)
        next->TrackMetricsTime(time);
}

void ChainedCache::Print(nlohmann::json& output_json, uint64_t depth) {
    for (size_t i = 0; i < metrics.size(); ++i) {
        nlohmann::json j;
        j["layer"] = depth;
        j["admission_treshold"] = admission_treshold;
        j["algorithm"] = algorithm;
        j["hit"] = metrics[i].hit;
        j["req"] = metrics[i].req;
        j["inserted"] = metrics[i].inserted;
        j["reinserted"] = metrics[i].reinserted;
        j["miss_ratio"] = 1 - (double)metrics[i].hit / metrics[i].req;
        j["byte_miss"] = metrics[i].byte_miss;
        j["byte_read"] = metrics[i].byte_read;
        j["byte_inserted"] = metrics[i].byte_inserted;
        j["byte_reinserted"] = metrics[i].byte_reinserted;
        for (const auto& x : metrics_times[i]) {
            nlohmann::json e;
            e["hit"] = x.hit;
            e["req"] = x.req;
            e["inserted"] = x.inserted;
            e["reinserted"] = x.reinserted;
            e["miss_ratio"] = 1 - (double)x.hit / x.req;
            e["byte_miss"] = x.byte_miss;
            e["byte_read"] = x.byte_read;
            e["byte_inserted"] = x.byte_inserted;
            e["byte_reinserted"] = x.byte_reinserted;

            j["metrics_time"].push_back(e);
        }

        output_json[i]["metrics"].push_back(j);
        output_json[i]["iteration"] = i;
    }
    if (next)
        next->Print(output_json, ++depth);
}
void ChainedCache::CleanUp() {
    auto& additional_cache_data_storage = data::AdditionalCacheDataStorage::GetStorage();
    auto& self_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(self);
    self->cache_free(self);

    if (self_additional_cache_data.datasets.is_open())
        self_additional_cache_data.datasets.close();
    if (next)
        next->CleanUp();
}
}  // namespace CustomCache
