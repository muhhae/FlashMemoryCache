#include <config.h>
#include <libCacheSim.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace Gap2Clock {

class Gap2Clock {
   public:
    Gap2Clock() = default;
    void Initialize(cache_t* cache, const std::unordered_map<std::string, std::string> params) {
        assert(params.contains("cache_size"));
        uint64_t cache_size = std::stoull(params.at("cache_size"));
        if (cache_size < 10) {
            std::cout << "cache size < 10, aborting...";
            exit(0);
        }
        float delay_ratio = params.contains("delay_ratio") ? std::stof(params.at("delay_ratio"))
                                                           : 0.1;
        frequency_limit = params.contains("n_bit") ? (1 << std::stoi(params.at("n_bit"))) - 1 : 1;
        assert(delay_ratio >= 0 && delay_ratio <= 1);
        delay_time = uint64_t(cache_size * delay_ratio);
    }
    bool get(cache_t* cache, const request_t* req) {
        return cache_get_base(cache, req);
    }
    cache_obj_t* find(cache_t* cache, const request_t* req, const bool update_cache) {
        cache_obj_t* obj = cache_find_base(cache, req, update_cache);
        current_time += update_cache;
        if (obj != nullptr && update_cache && obj->clock.freq < frequency_limit) {
            auto& metadata = metadatas.at(obj->obj_id);
            uint64_t time_passed = current_time - metadata.last_access;
            if (time_passed > delay_time) {
                obj->clock.freq += 1;
                metadata.last_access = current_time;
            }
        }
        return obj;
    }
    cache_obj_t* insert(cache_t* cache, const request_t* req) {
        cache_obj_t* obj = cache_insert_base(cache, req);
        queue.push_front(obj);
        metadatas[obj->obj_id] = {.last_access = current_time};
        obj->clock.freq = 0;
        return obj;
    }
    void evict(cache_t* cache, const request_t* req) {
        auto obj_to_evict = queue.back();
        if (obj_to_evict->clock.freq == 0) {
            metadatas.erase(obj_to_evict->obj_id);
            cache_evict_base(cache, obj_to_evict, true);
            queue.pop_back();
            return;
        }
        obj_to_evict->clock.freq -= 1;
        queue.push_front(obj_to_evict);
        queue.pop_back();
        data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache).OnPromotion(
            obj_to_evict, req
        );
    }

    cache_obj_t* to_evict(cache_t* cache, const request_t* req) {
        throw std::invalid_argument("to_evict is not yet supported");
    }
    bool remove(cache_t* cache, const obj_id_t obj_id) {
        throw std::invalid_argument("remove is not yet supported");
    }

   private:
    uint64_t current_time;
    uint64_t delay_time;
    uint64_t frequency_limit;

    std::list<cache_obj_t*> queue;
    struct Gap2ClockMetadata {
        uint64_t last_access;
    };
    std::unordered_map<obj_id_t, Gap2ClockMetadata> metadatas;
};

// ***********************************************************************
// ****                                                               ****
// ****                   function declarations                       ****
// ****                                                               ****
// ***********************************************************************

static void Gap2Clock_free(cache_t* cache);
static bool Gap2Clock_get(cache_t* cache, const request_t* req);
static cache_obj_t* Gap2Clock_find(cache_t* cache, const request_t* req, const bool update_cache);
static cache_obj_t* Gap2Clock_insert(cache_t* cache, const request_t* req);
static cache_obj_t* Gap2Clock_to_evict(cache_t* cache, const request_t* req);
static void Gap2Clock_evict(cache_t* cache, const request_t* req);
static bool Gap2Clock_remove(cache_t* cache, const obj_id_t obj_id);

// ***********************************************************************
// ****                                                               ****
// ****                   end user facing functions                   ****
// ****                                                               ****
// ****                       init, free, get                         ****
// ***********************************************************************

cache_t* Gap2Clock_init(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    cache_t* cache = cache_struct_init("Gap2Clock", ccache_params, cache_specific_params);
    cache->eviction_params = new Gap2Clock();
    cache->cache_init = Gap2Clock_init;
    cache->cache_free = Gap2Clock_free;

    cache->get = Gap2Clock_get;
    cache->find = Gap2Clock_find;
    cache->insert = Gap2Clock_insert;
    cache->evict = Gap2Clock_evict;
    cache->remove = Gap2Clock_remove;

    if (ccache_params.consider_obj_metadata) {
        cache->obj_md_size = 8;
    } else {
        cache->obj_md_size = 0;
    }

    return cache;
}

static void Gap2Clock_free(cache_t* cache) {
    auto* data = static_cast<Gap2Clock*>(cache->eviction_params);
    delete data;
    cache_struct_free(cache);
}

static bool Gap2Clock_get(cache_t* cache, const request_t* req) {
    return static_cast<Gap2Clock*>(cache->eviction_params)->get(cache, req);
}
static cache_obj_t* Gap2Clock_find(cache_t* cache, const request_t* req, const bool update_cache) {
    return static_cast<Gap2Clock*>(cache->eviction_params)->find(cache, req, update_cache);
}
static cache_obj_t* Gap2Clock_insert(cache_t* cache, const request_t* req) {
    return static_cast<Gap2Clock*>(cache->eviction_params)->insert(cache, req);
}
static cache_obj_t* Gap2Clock_to_evict(cache_t* cache, const request_t* req) {
    return static_cast<Gap2Clock*>(cache->eviction_params)->to_evict(cache, req);
}
static void Gap2Clock_evict(cache_t* cache, const request_t* req) {
    return static_cast<Gap2Clock*>(cache->eviction_params)->evict(cache, req);
}
static bool Gap2Clock_remove(cache_t* cache, const obj_id_t obj_id) {
    return static_cast<Gap2Clock*>(cache->eviction_params)->remove(cache, obj_id);
}

void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    assert(params.contains("cache_size"));
    auto* data = static_cast<Gap2Clock*>(cache->eviction_params);
    data->Initialize(cache, params);
}

}  // namespace Gap2Clock

cache_t* Gap2ClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Gap2Clock::Gap2Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = Gap2ClockInit;
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    data.SetParamsCallback = Gap2Clock::SetParams;
    return cache;
}
}  // namespace algorithm
