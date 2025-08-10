#include "simulator.hpp"

#include <libCacheSim/admissionAlgo.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/enum.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/reader.h>
#include <sys/types.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "additional_data.hpp"
#include "cache.hpp"
#include "lib/cache_size.h"
#include "lib/json.hpp"

void RunExperiment(options o) {
    std::array<std::string, 4> offline_algo = {
        "offline-clock", "offline-clock-v2", "offline-q-clock", "offline-qtime-clock"
    };
    if (o.max_iteration < 2 && std::ranges::contains(offline_algo, o.algorithm))
        o.max_iteration = 2;

    std::filesystem::create_directories(o.output_directory / "log");
    if (o.generate_datasets)
        std::filesystem::create_directories(o.output_directory / "datasets");
    for (const auto& p : o.trace_paths) {
        reader_init_param_t reader_init_param = {
            .ignore_obj_size = o.ignore_obj_size,
            .obj_id_is_num = o.id_num,
            .obj_id_is_num_set = o.id_num,
            .time_field = 1,
            .obj_id_field = 2,
            .obj_size_field = 3,
            .has_header = true
        };

        trace_type_e trace_type = ORACLE_GENERAL_TRACE;
        if (o.trace_type == "csv") {
            trace_type = CSV_TRACE;
        }
        reader_t* reader = open_trace(p.c_str(), trace_type, &reader_init_param);
        int64_t wss_obj = 0;
        int64_t wss_byte = 0;
        auto approximate_request_count = cal_working_set_size(reader, &wss_obj, &wss_byte);
        close_reader(reader);
        int64_t wss = o.ignore_obj_size ? wss_obj : wss_byte;

        for (const auto& fcs : o.fixed_cache_sizes) {
            o.dist_optimal_treshold = o.ignore_obj_size ? fcs : fcs / wss_byte * wss_obj;
            std::string desc = "[" + std::to_string(fcs) + (o.ignore_obj_size ? "" : "MiB") +
                               (o.desc != "" ? "," : "") + o.desc + "]";
            uint64_t cache_size = o.ignore_obj_size ? fcs : fcs * MiB;
            Simulate(cache_size, p, o, desc, approximate_request_count);
        }
        for (const auto& rcs : o.relative_cache_sizes) {
            o.dist_optimal_treshold = rcs * wss_obj;
            std::string s = std::to_string(rcs);
            s = s.substr(0, s.find_last_not_of('0') + 1);
            if (s.back() == '.')
                s.pop_back();

            std::string desc = "[" + s + (o.desc != "" ? "," : "") + o.desc + "]";
            uint64_t cache_size = wss * rcs;
            Simulate(cache_size, p, o, desc, approximate_request_count);
        }
    }
}

reader_t* SetupReader(const options& o, std::filesystem::path trace_path) {
    reader_init_param_t reader_init_param = {
        .ignore_obj_size = o.ignore_obj_size,
        .obj_id_is_num = o.id_num,
        .obj_id_is_num_set = o.id_num,
        .time_field = 1,
        .obj_id_field = 2,
        .obj_size_field = 3,
        .has_header = true
    };

    trace_type_e trace_type = ORACLE_GENERAL_TRACE;
    if (o.trace_type == "csv") {
        trace_type = CSV_TRACE;
    }

    return open_trace(trace_path.c_str(), trace_type, &reader_init_param);
}

nlohmann::json SimulationResults(CustomCache::ChainedCache* Cache) {
    nlohmann::json output_json;
    Cache->Print(output_json["results"], 0);
    for (size_t i = 0; i < output_json["results"].size(); ++i) {
        auto& metrics = output_json["results"][i]["metrics"];
        uint64_t overall_hit = 0;
        uint64_t overall_req = metrics[0]["req"];
        for (size_t j = 0; j < metrics.size(); ++j) {
            overall_hit += (uint64_t)metrics[j]["hit"];
        }
        double overall_miss_ratio = 1 - (double)overall_hit / overall_req;
        output_json["results"][i]["hit"] = overall_hit;
        output_json["results"][i]["req"] = overall_req;
        output_json["results"][i]["miss_ratio"] = overall_miss_ratio;
    }
    return output_json;
}

template <typename K, typename V>
size_t get_unordered_map_memory_usage(const std::unordered_map<K, V>& map) {
    size_t size = sizeof(map);
    size += map.bucket_count() * sizeof(void*);
    size_t node_size = sizeof(K) + sizeof(V) + sizeof(void*);
    size += map.size() * node_size;
    return size;
}

void Simulate(
    const uint64_t cache_size,
    const std::filesystem::path trace_path,
    options o,
    const std::string desc,
    const int64_t approximate_request_count
) {
    std::string base_path = std::filesystem::path(trace_path).filename();
    size_t pos = base_path.find(".oracleGeneral");
    if (o.trace_type == "csv") {
        pos = base_path.find(".csv");
    }
    if (pos != std::string::npos) {
        base_path = base_path.substr(0, pos);
    }

    std::filesystem::path output_path = o.output_directory / "log" / (base_path + desc + ".json");
    std::filesystem::path dataset_path = o.output_directory / "datasets" /
                                         (base_path + desc + ".csv");

    reader_t* reader = SetupReader(o, trace_path);
    request_t* req = new_request();

    CustomCache::object_metadatas_enabled dram_object_metadatas_enabled = {};
    CustomCache::object_metadatas_enabled flash_object_metadatas_enabled = {};
    if (o.flash_admission_treshold > 0) {
        if (o.lifetime_freq_treshold) {
            dram_object_metadatas_enabled.lifetime = true;
        } else {
            dram_object_metadatas_enabled.in_cache = true;
        }
        if (!o.dram_enabled) {
            flash_object_metadatas_enabled.lifetime = true;
        }
    }
    CustomCache::ChainedCache Flash = CustomCache::ChainedCache(
        o.algorithm,
        cache_size,
        NULL,
        dataset_path,
        o.flash_admission_treshold,
        o.generate_datasets,
        o.lifetime_freq_treshold,
        flash_object_metadatas_enabled
    );
    CustomCache::ChainedCache DRAM = CustomCache::ChainedCache(
        "lru",
        cache_size * o.dram_size,
        &Flash,
        dataset_path,
        0,
        o.generate_datasets,
        o.lifetime_freq_treshold,
        dram_object_metadatas_enabled
    );
    CustomCache::ChainedCache* Cache = o.dram_enabled ? &DRAM : &Flash;
    if (o.bloomfilter) {
        Flash.self->admissioner = create_bloomfilter_admissioner(NULL);
    }

    auto& DRAM_data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(
        DRAM.self
    );
    auto& Flash_data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(
        Flash.self
    );

    if (DRAM_data.SetParamsCallback)
        DRAM_data.SetParamsCallback(DRAM_data, o.cache_params);
    if (Flash_data.SetParamsCallback)
        Flash_data.SetParamsCallback(Flash_data, o.cache_params);

    uint64_t req_counter = 0;
    uint64_t req_limit = o.req_limit * approximate_request_count;

    for (size_t i = 0; i < o.max_iteration; ++i) {
        Cache->SetupIteration(i == o.max_iteration - 1 && o.generate_datasets);
        while (read_one_req(reader, req) == 0) {
            if (o.req_limit != 1 && req_counter >= req_limit)
                break;
            req_counter++;
            Cache->Get(req);
            if (o.timeline) {
                Cache->TrackMetricsTime(req->clock_time);
            }
        }
        Cache->EndIteration();
        reset_reader(reader);
        req_counter = 0;
    }

    auto output_json = SimulationResults(Cache);
    output_json["trace"] = std::filesystem::path(trace_path).filename();
    output_json["dram_cache_size"] = DRAM.self->cache_size;
    output_json["flash_cache_size"] = Flash.self->cache_size;
    std::cout << output_json.dump(2) << "\n";
    std::ofstream(output_path) << output_json.dump(2);

    // auto& dram_data =
    //     data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(DRAM.self);
    // auto& flash_data =
    //     data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(Flash.self);
    //
    // std::cout << "DRAM metadata memory usage: "
    //           << get_unordered_map_memory_usage(dram_data.objs_metadata) / 1024.0 /
    //           1024.0
    //           << " MB" << std::endl;
    // std::cout << "Flash metadata memory usage: "
    //           << get_unordered_map_memory_usage(flash_data.objs_metadata) / 1024.0 /
    //                  1024.0
    //           << " MB" << std::endl;

    Cache->CleanUp();
    free_request(req);
    close_reader(reader);
}
