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
    void Admit(cache_obj_t* obj, uint64_t freq);
    void Print(nlohmann::json& output_json, uint64_t depth);
    void CleanUp();

   public:
    cache_t* self;
    cache_t* tmp;

    ChainedCache* next;
    std::string algorithm;

    std::vector<uint64_t> req;
    std::vector<uint64_t> hit;
    std::vector<uint64_t> inserted;
    std::vector<uint64_t> reinserted;

    uint64_t admission_treshold = 1;
    bool isML = false;
};
}  // namespace CustomCache
