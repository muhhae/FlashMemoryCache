#include "simulator.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/enum.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/reader.h>
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <sstream>
#include <stdexcept>
#include <string>

#include "cache.hpp"
#include "cache/base.hpp"
#include "cache/decayed_clock.hpp"
#include "cache/dist_clock.hpp"
#include "cache/ml_clock.hpp"
#include "cache/my_clock.hpp"
#include "cache/offline_clock.hpp"
#include "lib/cache_size.h"

const std::string csv_header =
    "trace_path,ignore_obj_size,cache_size,miss_ratio,n_req,n_promoted,n_miss,n_hit\n";

std::function<
    cache_t*(const common_cache_params_t ccache_params, const char* cache_specific_params)>
AlgoSelector(const options& o) {
    if (o.algorithm == "decayed-clock") {
        return decayed::DecayedClockInit;
    }
    if (o.algorithm == "fifo") {
        return base::FIFOInit;
    }
    if (o.algorithm == "offline-clock") {
        return cclock::OfflineClockInit;
    }
    if (o.algorithm == "dist-optimal") {
        return distclock::DistClockInit;
    }
    if (o.algorithm == "lru") {
        return base::LRUInit;
    }
    if (o.algorithm == "clock") {
        return base::ClockInit;
    }
    if (o.algorithm == "my") {
        return myclock::MyClockInit;
    }
    if (o.algorithm == "ML") {
        if (o.ml_model == "") {
            throw std::runtime_error("ML model need to be provided in ONNX format");
        }
        if (o.input_type == "I32") {
            return mlclock::MLClockInit<int32_t>;
        }
        if (o.input_type == "I64") {
            return mlclock::MLClockInit<int64_t>;
        }
        if (o.input_type == "F32") {
            return mlclock::MLClockInit<float>;
        }
        throw std::runtime_error("Input type is not valid");
    }
    throw std::runtime_error("algorithm not found");
}

void RunExperiment(options o) {
    if (o.algorithm == "offline-clock" && o.max_iteration < 2)
        o.max_iteration = 2;
    std::filesystem::create_directories(o.output_directory / "log");
    if (o.generate_datasets)
        std::filesystem::create_directories(o.output_directory / "datasets");

    std::vector<std::future<void>> tasks;
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
        cal_working_set_size(reader, &wss_obj, &wss_byte);
        close_reader(reader);
        int64_t wss = o.ignore_obj_size ? wss_obj : wss_byte;

        for (const auto& fcs : o.fixed_cache_sizes) {
            o.dist_optimal_treshold = o.ignore_obj_size ? fcs : fcs / wss_byte * wss_obj;
            std::string desc = "[" + std::to_string(fcs) + (o.ignore_obj_size ? "" : "MiB") +
                               (o.desc != "" ? "," : "") + o.desc + "]";
            tasks.emplace_back(
                std::async(
                    std::launch::async, Simulate, o.ignore_obj_size ? fcs : fcs * MiB, p, o, desc
                )
            );
        }
        for (const auto& rcs : o.relative_cache_sizes) {
            o.dist_optimal_treshold = rcs * wss_obj;
            std::string s = std::to_string(rcs);
            s = s.substr(0, s.find_last_not_of('0') + 1);
            if (s.back() == '.')
                s.pop_back();

            std::string desc = "[" + s + (o.desc != "" ? "," : "") + o.desc + "]";
            tasks.emplace_back(std::async(std::launch::async, Simulate, wss * rcs, p, o, desc));
        }
    }

    for (auto& t : tasks) {
        t.get();
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

void Simulate(
    uint64_t cache_size,
    const std::filesystem::path trace_path,
    const options o,
    const std::string desc
) {
    std::string base_path = std::filesystem::path(trace_path).filename();
    size_t pos = base_path.find(".oracleGeneral");
    if (o.trace_type == "csv") {
        pos = base_path.find(".csv");
    }
    if (pos != std::string::npos) {
        base_path = base_path.substr(0, pos);
    }

    std::filesystem::path log_path = o.output_directory / "log" / (base_path + desc + ".csv");
    std::filesystem::path dataset_path =
        o.output_directory / "datasets" / (base_path + desc + ".csv");

    std::ofstream csv_file(log_path);
    csv_file << csv_header;

    reader_t* reader = SetupReader(o, trace_path);
    request_t* req = new_request();

    CustomCache::ChainedCache Flash = CustomCache::ChainedCache(
        AlgoSelector(o)({.cache_size = cache_size}, NULL), NULL, o, dataset_path
    );
    CustomCache::ChainedCache DRAM = CustomCache::ChainedCache(
        base::LRUInit({.cache_size = cache_size / 100}, NULL), &Flash, o, dataset_path
    );
    CustomCache::ChainedCache* Cache = o.dram_enabled ? &DRAM : &Flash;
    for (size_t i = 0; i < o.max_iteration; ++i) {
        Cache->SetupIteration(o, i == o.max_iteration - 1);
        while (read_one_req(reader, req) == 0) {
            Cache->Get(req);
        }
        Cache->EndIteration(o);
        reset_reader(reader);
    }
    std::ostringstream s;
    Cache->Print(s);

    free_request(req);
    close_reader(reader);
}
