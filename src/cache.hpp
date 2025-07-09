#pragma once
#include <libCacheSim/cache.h>
#include <libCacheSim/cacheObj.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cstdint>
#include <filesystem>
#include <vector>

#include "lib/json.hpp"
#include "options.hpp"

namespace CustomCache {
class ChainedCache {
   public:
    ChainedCache(
        std::string Algorithm,
        uint64_t cache_size,
        ChainedCache* next,
        std::filesystem::path datasets,
        uint64_t admission_treshold,
        bool generate_datasets
    );
    bool Get(const request_t* req);
    bool Find(const request_t* req);
    void SetupIteration(bool generate_datasets);
    void EndIteration();
    void Admit(const cache_obj_t* obj, const uint64_t freq);
    void Admit(const request_t* req, const uint64_t freq);
    void Print(nlohmann::json& output_json, uint64_t depth);
    void CleanUp();

   public:
    cache_t* self;
    cache_t* tmp;

    ChainedCache* next;
    std::string algorithm;

    struct CacheMetrics {
        uint64_t req;
        uint64_t hit;
        uint64_t inserted;
        uint64_t reinserted;

        uint64_t byte_read;
        uint64_t byte_reinserted;
        uint64_t byte_inserted;
    };

    std::vector<CacheMetrics> metrics;

    uint64_t admission_treshold = 1;
    bool isML = false;
};
}  // namespace CustomCache
