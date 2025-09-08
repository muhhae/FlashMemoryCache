#include <config.h>
#include <libCacheSim.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace S3FClockSequential {

class S3FClockSequential {
   public:
    S3FClockSequential() = default;
    void Initialize(cache_t* cache, const std::unordered_map<std::string, std::string> params) {
        hand_position = params.contains("h_position") ? std::stof(params.at("h_position")) : 0.1;
        assert(hand_position >= 0 && hand_position <= 1);
        assert(params.contains("cache_size"));
        cache_size = std::stoull(params.at("cache_size"));
        threshold = std::clamp(
            uint64_t(cache_size * hand_position) + 1, uint64_t(2), uint64_t(cache_size - 2)
        );
    }

    bool get(cache_t* cache, const request_t* req) {
        return cache_get_base(cache, req);
    }
    cache_obj_t* find(cache_t* cache, const request_t* req, const bool update_cache) {
        cache_obj_t* obj = cache_find_base(cache, req, update_cache);
        if (obj != nullptr && update_cache) {
            obj->clock.freq = 1;
        }
        return obj;
    }
    cache_obj_t* insert(cache_t* cache, const request_t* req) {
        if (queue.size() == threshold) {
            hand--;
        }

        if (hand != queue.end()) {
            (*hand)->clock.freq = 0;
            hand--;
        }

        cache_obj_t* obj = cache_insert_base(cache, req);
        queue.push_front(obj);
        obj->clock.freq = 0;

        return obj;
    }
    void evict(cache_t* cache, const request_t* req) {
        // size_t index = std::distance(queue.begin(), hand);
        // std::cout << "Index: " << index << "\n";  // prints 2
        // (*hand)->clock.freq = 0;
        if (queue.back()->clock.freq == 0) {
            cache_evict_base(cache, queue.back(), true);
            queue.pop_back();
            return;
        }

        (*hand)->clock.freq = 0;
        hand--;

        data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache).OnPromotion(
            queue.back(), req
        );
        queue.back()->clock.freq = 0;
        queue.push_front(queue.back());
        queue.pop_back();
    }
    cache_obj_t* to_evict(cache_t* cache, const request_t* req) {
        throw std::runtime_error("to_evict is not yet supported");
    }
    bool remove(cache_t* cache, const obj_id_t obj_id) {
        throw std::runtime_error("remove is not yet supported");
    }

   private:
    float hand_position;
    uint64_t cache_size;
    uint64_t threshold;
    std::list<cache_obj_t*> queue;
    std::list<cache_obj_t*>::iterator hand = queue.end();
};

// ***********************************************************************
// ****                                                               ****
// ****                   function declarations                       ****
// ****                                                               ****
// ***********************************************************************

static void S3FClockSequential_free(cache_t* cache);
static bool S3FClockSequential_get(cache_t* cache, const request_t* req);
static cache_obj_t* S3FClockSequential_find(
    cache_t* cache, const request_t* req, const bool update_cache
);
static cache_obj_t* S3FClockSequential_insert(cache_t* cache, const request_t* req);
static cache_obj_t* S3FClockSequential_to_evict(cache_t* cache, const request_t* req);
static void S3FClockSequential_evict(cache_t* cache, const request_t* req);
static bool S3FClockSequential_remove(cache_t* cache, const obj_id_t obj_id);

// ***********************************************************************
// ****                                                               ****
// ****                   end user facing functions                   ****
// ****                                                               ****
// ****                       init, free, get                         ****
// ***********************************************************************

cache_t* S3FClockSequential_init(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    cache_t* cache = cache_struct_init("S3FClockSequential", ccache_params, cache_specific_params);
    cache->eviction_params = new S3FClockSequential();
    cache->cache_init = S3FClockSequential_init;
    cache->cache_free = S3FClockSequential_free;

    cache->get = S3FClockSequential_get;
    cache->find = S3FClockSequential_find;
    cache->insert = S3FClockSequential_insert;
    cache->evict = S3FClockSequential_evict;
    cache->remove = S3FClockSequential_remove;

    if (ccache_params.consider_obj_metadata) {
        cache->obj_md_size = 8;
    } else {
        cache->obj_md_size = 0;
    }

    return cache;
}

static void S3FClockSequential_free(cache_t* cache) {
    auto* data = static_cast<S3FClockSequential*>(cache->eviction_params);
    delete data;
    cache_struct_free(cache);
}

static bool S3FClockSequential_get(cache_t* cache, const request_t* req) {
    return static_cast<S3FClockSequential*>(cache->eviction_params)->get(cache, req);
}
static cache_obj_t* S3FClockSequential_find(
    cache_t* cache, const request_t* req, const bool update_cache
) {
    return static_cast<S3FClockSequential*>(cache->eviction_params)->find(cache, req, update_cache);
}
static cache_obj_t* S3FClockSequential_insert(cache_t* cache, const request_t* req) {
    return static_cast<S3FClockSequential*>(cache->eviction_params)->insert(cache, req);
}
static cache_obj_t* S3FClockSequential_to_evict(cache_t* cache, const request_t* req) {
    return static_cast<S3FClockSequential*>(cache->eviction_params)->to_evict(cache, req);
}
static void S3FClockSequential_evict(cache_t* cache, const request_t* req) {
    return static_cast<S3FClockSequential*>(cache->eviction_params)->evict(cache, req);
}
static bool S3FClockSequential_remove(cache_t* cache, const obj_id_t obj_id) {
    return static_cast<S3FClockSequential*>(cache->eviction_params)->remove(cache, obj_id);
}

void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    assert(params.contains("cache_size"));
    auto* data = static_cast<S3FClockSequential*>(cache->eviction_params);
    data->Initialize(cache, params);
}

}  // namespace S3FClockSequential

cache_t* S3FClockSequentialInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = S3FClockSequential::S3FClockSequential_init(ccache_params, cache_specific_params);
    cache->cache_init = S3FClockSequentialInit;
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    data.SetParamsCallback = S3FClockSequential::SetParams;
    return cache;
}
}  // namespace algorithm
