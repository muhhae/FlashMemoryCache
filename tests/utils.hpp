#pragma once

#include <libCacheSim/enum.h>
#include <libCacheSim/reader.h>

#include <cstddef>
#include <vector>

#include "cache.hpp"
#include "lib/json.hpp"
#include "simulator.hpp"

static nlohmann::json LayeredCacheSimulation(
    std::vector<std::string> algorithms,
    uint64_t cache_size = 100,
    bool ignore_obj_size = true,
    std::string trace = "../trace/cloudPhysicsIO.oracleGeneral.bin",
    trace_type_e trace_type = ORACLE_GENERAL_TRACE
) {
    reader_init_param_t reader_init_param = {
        .ignore_obj_size = ignore_obj_size,
        .time_field = 1,
        .obj_id_field = 2,
        .obj_size_field = 3,
        .has_header = true
    };

    auto reader = open_trace(trace.c_str(), trace_type, &reader_init_param);
    auto req = new_request();

    std::vector<CustomCache::ChainedCache> Caches;
    for (const auto& a : algorithms) {
        Caches.push_back(CustomCache::ChainedCache(a, cache_size, NULL, "", 4, false));
        cache_size *= 10;
    }
    for (size_t i = 0; i < Caches.size() - 1; ++i) {
        Caches[i].next = &Caches[i + 1];
    }

    for (size_t i = 0; i < 2; ++i) {
        Caches[0].SetupIteration(false);
        while (read_one_req(reader, req) == 0) {
            Caches[0].Get(req);
        }
        Caches[0].EndIteration();
        reset_reader(reader);
    }
    Caches[0].CleanUp();
    return SimulationResults(&Caches[0])["results"][1];
}
