#pragma once
#include <libCacheSim/cache.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cstdint>
#include <filesystem>
#include <vector>

#include "lib/json.hpp"
#include "simulator.hpp"

namespace CustomCache {
class ChainedCache {
   public:
    ChainedCache(
        std::string Algorithm,
        uint64_t cache_size,
        ChainedCache* next,
        const options& o,
        std::filesystem::path datasets
    );
    bool Get(const request_t* req);
    bool Find(const request_t* req);
    void SetupIteration(const options& o, bool last_iteration);
    void EndIteration(const options& o);
    void Admit(const request_t* req, uint64_t freq);
    void Print(nlohmann::json& output_json, uint64_t depth);
    void CleanUp(const options& o);

   public:
    cache_t* self;
    cache_t* tmp;

    ChainedCache* next;
    std::string algorithm;

    std::vector<uint64_t> req;
    std::vector<uint64_t> hit;
    std::vector<uint64_t> write;

    uint64_t admission_treshold = 1;
    bool isML = false;
};
}  // namespace CustomCache
