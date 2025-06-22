#pragma once

#include <libCacheSim/cache.h>
#include <libCacheSim/reader.h>
#include <sys/types.h>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

struct options {
    std::string trace_type;
    std::string algorithm;
    std::vector<std::filesystem::path> trace_paths;

    std::vector<std::string> features_name;
    std::vector<uint64_t> fixed_cache_sizes;
    std::vector<float> relative_cache_sizes;
    std::vector<std::string> descs;

    float treshold;
    uint64_t flash_admission_treshold = 0;
    uint64_t dist_optimal_treshold = std::numeric_limits<uint64_t>::max();
    float decay_power = 0.001;

    bool dram_enabled = false;
    bool id_num = false;
    bool ignore_obj_size = false;
    bool generate_datasets = false;

    uint64_t max_iteration;
    std::filesystem::path output_directory;
    std::string ml_model;
    std::string input_type;
    std::string desc;
};

void Simulate(
    cache_t* cache, const std::filesystem::path trace_path, const options o, const std::string desc
);
void RunExperiment(options o);
