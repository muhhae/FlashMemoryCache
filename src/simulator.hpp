#pragma once

#include <libCacheSim/cache.h>
#include <libCacheSim/reader.h>
#include <sys/types.h>

#include <cstdint>
#include <filesystem>
#include <string>

#include "cache.hpp"

void Simulate(
    const uint64_t cache_size,
    const std::filesystem::path trace_path,
    const options o,
    const std::string desc,
    const int64_t approximate_request_count
);
void RunExperiment(options o);
void BasicSimulation(CustomCache::ChainedCache* Cache, reader_t* reader);
nlohmann::json SimulationResults(CustomCache::ChainedCache* Cache);
