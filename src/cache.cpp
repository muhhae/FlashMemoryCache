#include "cache.hpp"

#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/cacheObj.h>
#include <libCacheSim/request.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <string>
#include <utility>

#include "cache/additional_data.hpp"
#include "cache/clock.hpp"
#include "cache/decayed_clock.hpp"
#include "cache/dist_clock.hpp"
#include "cache/fifo.hpp"
#include "cache/lru.hpp"
#include "cache/my_clock.hpp"
#include "cache/offline_clock.hpp"
#include "lib/json.hpp"

std::function<
    cache_t*(const common_cache_params_t ccache_params, const char* cache_specific_params)>
AlgoSelector(std::string algorithm) {
    if (algorithm == "decayed-clock") {
        return decayed::DecayedClockInit;
    }
    if (algorithm == "fifo") {
        return fifo::FIFOInit;
    }
    if (algorithm == "offline-clock") {
        return cclock::OfflineClockInit;
    }
    if (algorithm == "dist-optimal") {
        return distclock::DistClockInit;
    }
    if (algorithm == "lru") {
        return lru::LRUInit;
    }
    if (algorithm == "clock") {
        return bclock::ClockInit;
    }
    if (algorithm == "my") {
        return myclock::MyClockInit;
    }
    if (algorithm == "ML") {
        throw std::runtime_error("ML is currently disabled");
        // if (ml_model == "") {
        //     throw std::runtime_error("ML model need to be provided in ONNX format");
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
    bool generate_datasets
)
    : next(next), algorithm(Algorithm), admission_treshold(admission_treshold) {
    self = AlgoSelector(Algorithm)({.cache_size = cache_size}, NULL);
    auto& additional_cache_data =
        AdditionalData::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(self);
    if (generate_datasets) {
        additional_cache_data.datasets = std::ofstream(datasets);
        for (size_t i = 0; i < AdditionalData::datasets_columns.size(); i++) {
            additional_cache_data.datasets
                << AdditionalData::datasets_columns[i]
                << (i == AdditionalData::datasets_columns.size() - 1 ? '\n' : ',');
        }
    }
}
void ChainedCache::SetupIteration(bool generate_datasets) {
    tmp = clone_cache(self);

    auto& additional_cache_data_storage = AdditionalData::AdditionalCacheDataStorage::GetStorage();
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

    tmp_additional_cache_data.n_hit = 0;
    tmp_additional_cache_data.n_req = 0;
    tmp_additional_cache_data.n_promoted = 0;
    tmp_additional_cache_data.n_inserted = 0;

    tmp_additional_cache_data.generate_datasets = generate_datasets;

    if (next) {
        tmp_additional_cache_data.next = next;
        next->SetupIteration(generate_datasets);
    }
}
void ChainedCache::EndIteration() {
    auto& additional_cache_data_storage = AdditionalData::AdditionalCacheDataStorage::GetStorage();
    auto& tmp_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(tmp);

    req.push_back(tmp_additional_cache_data.n_req);
    hit.push_back(tmp_additional_cache_data.n_hit);
    inserted.push_back(tmp_additional_cache_data.n_inserted);
    reinserted.push_back(tmp_additional_cache_data.n_promoted);

    for (auto& e : tmp_additional_cache_data.objs_metadata) {
        e.second.Reset();
        e.second.lifetime_freq = 0;
        e.second.last_promotion = 0;
    }

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
void ChainedCache::Admit(cache_obj_t* obj, uint64_t freq) {
    if (freq < admission_treshold)
        return;
    request_t req;
    copy_cache_obj_to_request(&req, obj);
    auto& additional_cache_data_storage = AdditionalData::AdditionalCacheDataStorage::GetStorage();
    auto& tmp_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(tmp);
    if (!tmp->get(tmp, &req))
        tmp_additional_cache_data.n_inserted++;
}
void ChainedCache::Print(nlohmann::json& output_json, uint64_t depth) {
    for (size_t i = 0; i < hit.size(); ++i) {
        nlohmann::json j;
        j["layer"] = depth;
        j["admission_treshold"] = admission_treshold;
        j["algorithm"] = algorithm;
        j["hit"] = hit[i];
        j["req"] = req[i];
        j["inserted"] = inserted[i];
        j["reinserted"] = reinserted[i];
        j["miss_ratio"] = 1 - (double)hit[i] / req[i];
        output_json[i]["metrics"].push_back(j);
        output_json[i]["iteration"] = i;
    }
    if (next)
        next->Print(output_json, ++depth);
}
void ChainedCache::CleanUp() {
    auto& additional_cache_data_storage = AdditionalData::AdditionalCacheDataStorage::GetStorage();
    auto& self_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(self);
    self_additional_cache_data.objs_metadata.clear();
    self->cache_free(self);

    if (self_additional_cache_data.datasets.is_open())
        self_additional_cache_data.datasets.close();
    if (next)
        next->CleanUp();
}
bool ChainedCache::Get(const request_t* req) {
    auto& additional_cache_data_storage = AdditionalData::AdditionalCacheDataStorage::GetStorage();
    auto& tmp_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(tmp);
    auto& data = tmp_additional_cache_data.objs_metadata[req->obj_id];

    tmp_additional_cache_data.OnAccessTracking(data, req);
    tmp_additional_cache_data.n_req++;

    if (!tmp->get(tmp, req)) {
        tmp_additional_cache_data.n_inserted++;
        if (next)
            return next->Find(req);
        return false;
    }
    tmp_additional_cache_data.n_hit++;
    return true;
}
bool ChainedCache::Find(const request_t* req) {
    auto& additional_cache_data_storage = AdditionalData::AdditionalCacheDataStorage::GetStorage();
    auto& tmp_additional_cache_data = additional_cache_data_storage.GetAdditionalCacheData(tmp);

    auto& data = tmp_additional_cache_data.objs_metadata[req->obj_id];

    tmp_additional_cache_data.OnAccessTracking(data, req);
    tmp_additional_cache_data.n_req++;
    if (tmp->find(tmp, req, false) == NULL) {
        if (next)
            return next->Find(req);
        return false;
    }
    tmp_additional_cache_data.n_hit++;
    return true;
}
}  // namespace CustomCache
