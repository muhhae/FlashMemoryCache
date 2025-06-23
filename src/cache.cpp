#include "cache.hpp"

#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/request.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>

#include "cache/common.hpp"
#include "cache/ml_clock.hpp"
#include "simulator.hpp"

namespace CustomCache {
ChainedCache::ChainedCache(
    cache_t* self, ChainedCache* next, const options& o, std::filesystem::path datasets
)
    : self(self), next(next) {
    auto params = (common::CustomParams*)self->eviction_params;
    if (o.generate_datasets) {
        params->datasets = std::ofstream(datasets);
        for (size_t i = 0; i < common::datasets_columns.size(); i++) {
            params->datasets << common::datasets_columns[i]
                             << (i == common::datasets_columns.size() - 1 ? '\n' : ',');
        }
    }
    if (o.algorithm == "ML") {
        ((mlclock::MLClockParam*)params)->LoadModel(o.ml_model);
        ((mlclock::MLClockParam*)params)->features_name = o.features_name;
    }
}
void ChainedCache::EndIteration(const options& o) {
    auto params = (common::CustomParams*)self->eviction_params;
    auto tmp_params = (common::CustomParams*)tmp->eviction_params;

    req.push_back(tmp_params->n_req);
    hit.push_back(tmp_params->n_hit);

    for (auto& e : tmp_params->req_map) {
        free(e.second);
    }

    for (auto& e : tmp_params->objs_metadata) {
        e.second.Reset();
        e.second.lifetime_freq = 0;
        e.second.last_promotion = 0;
    }
    std::swap(tmp_params->objs_metadata, params->objs_metadata);
    std::swap(tmp_params->datasets, params->datasets);
    if (isML) {
        auto tmp_ml_param = (mlclock::MLClockParam*)tmp_params;
        auto ml_param = (mlclock::MLClockParam*)params;
        std::swap(tmp_ml_param->session, ml_param->session);
        std::swap(tmp_ml_param->session_options, ml_param->session_options);
        std::swap(tmp_ml_param->env, ml_param->env);
        std::swap(tmp_ml_param->features_name, ml_param->features_name);
    }
    tmp->cache_free(tmp);
    if (next)
        next->EndIteration(o);
}
void ChainedCache::SetupIteration(const options& o, bool last_iteration) {
    tmp = clone_cache(self);
    auto params = (common::CustomParams*)self->eviction_params;
    auto tmp_params = (common::CustomParams*)tmp->eviction_params;

    std::swap(tmp_params->objs_metadata, params->objs_metadata);
    std::swap(tmp_params->datasets, params->datasets);

    if (isML) {
        auto tmp_ml_param = (mlclock::MLClockParam*)tmp_params;
        auto ml_param = (mlclock::MLClockParam*)params;
        std::swap(tmp_ml_param->session, ml_param->session);
        std::swap(tmp_ml_param->session_options, ml_param->session_options);
        std::swap(tmp_ml_param->env, ml_param->env);
        std::swap(tmp_ml_param->features_name, ml_param->features_name);

        tmp_ml_param->treshold = ml_param->treshold;
    }

    tmp_params->n_hit = 0;
    tmp_params->n_req = 0;
    tmp_params->n_promoted = 0;

    if (last_iteration) {
        tmp_params->generate_datasets = o.generate_datasets;
    }

    if (next) {
        ((common::CustomParams*)tmp->eviction_params)->next = next;
        next->SetupIteration(o, last_iteration);
    }
}
void ChainedCache::Admit(const request_t* req, uint64_t freq) {
    if (freq >= admission_treshold) {
        tmp->get(tmp, req);
    }
}
void ChainedCache::Print(std::ostringstream& s) {
    for (size_t i = 0; i < hit.size(); ++i) {
        std::cout << "Hit: " << hit[i] << '\n';
        std::cout << "Req: " << req[i] << '\n';
        std::cout << "MR: " << 1 - (double)hit[i] / req[i] << '\n';
    }
    if (next)
        next->Print(s);
}
void ChainedCache::CleanUp(const options& o) {
    auto params = (common::CustomParams*)self->eviction_params;
    params->objs_metadata.clear();
    self->cache_free(self);

    if (o.generate_datasets)
        params->datasets.close();
    if (next)
        next->CleanUp(o);
}
bool ChainedCache::Get(const request_t* req) {
    auto params = (common::CustomParams*)tmp->eviction_params;
    auto& data = params->objs_metadata[req->obj_id];

    if (params->req_map[req->obj_id] != NULL) {
        free(params->req_map[req->obj_id]);
    }
    params->req_map[req->obj_id] = clone_request(req);

    common::OnAccessTracking(data, params, req);
    params->n_req++;
    if (!tmp->get(tmp, req)) {
        if (next)
            return next->Find(req);
        return false;
    }
    params->n_hit++;
    return true;
}
bool ChainedCache::Find(const request_t* req) {
    auto params = (common::CustomParams*)tmp->eviction_params;
    auto& data = params->objs_metadata[req->obj_id];

    if (params->req_map[req->obj_id] != NULL) {
        free(params->req_map[req->obj_id]);
    }
    params->req_map[req->obj_id] = clone_request(req);

    common::OnAccessTracking(data, params, req);
    params->n_req++;
    if (tmp->find(tmp, req, false) == NULL) {
        if (next)
            return next->Find(req);
        return false;
    }
    params->n_hit++;
    return true;
}
}  // namespace CustomCache
