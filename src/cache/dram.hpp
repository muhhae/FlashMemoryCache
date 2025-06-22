#pragma once
#include <config.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/request.h>

#include <unordered_map>

#include "cache/common.hpp"

namespace dram {
cache_t* LRUInit(const common_cache_params_t ccache_params, const char* cache_specific_params);
class DRAMParam : public common::CustomParams {
   public:
    DRAMParam(const Clock_params_t& base) {
        *(Clock_params_t*)this = base;
    }
    void InsertToMain(obj_id_t obj_id);

   public:
    std::unordered_map<obj_id_t, request_t*> req_map;
    cache_t* main_cache;
    float treshold = 1;
};
}  // namespace dram
