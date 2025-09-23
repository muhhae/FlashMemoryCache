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
namespace D3Clock {

class D3Clock {
   public:
    D3Clock() = default;
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
        current_time += 1;
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
        current_time += 1;
        obj_to_evict->clock.freq -= 1;
        metadatas.at(obj_to_evict->obj_id).last_access = current_time;
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
    struct D3ClockMetadata {
        uint64_t last_access;
    };
    std::unordered_map<obj_id_t, D3ClockMetadata> metadatas;
};

// ***********************************************************************
// ****                                                               ****
// ****                   function declarations                       ****
// ****                                                               ****
// ***********************************************************************

static void D3Clock_free(cache_t* cache);
static bool D3Clock_get(cache_t* cache, const request_t* req);
static cache_obj_t* D3Clock_find(cache_t* cache, const request_t* req, const bool update_cache);
static cache_obj_t* D3Clock_insert(cache_t* cache, const request_t* req);
static cache_obj_t* D3Clock_to_evict(cache_t* cache, const request_t* req);
static void D3Clock_evict(cache_t* cache, const request_t* req);
static bool D3Clock_remove(cache_t* cache, const obj_id_t obj_id);

// ***********************************************************************
// ****                                                               ****
// ****                   end user facing functions                   ****
// ****                                                               ****
// ****                       init, free, get                         ****
// ***********************************************************************

cache_t* D3Clock_init(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    cache_t* cache = cache_struct_init("D3Clock", ccache_params, cache_specific_params);
    cache->eviction_params = new D3Clock();
    cache->cache_init = D3Clock_init;
    cache->cache_free = D3Clock_free;

    cache->get = D3Clock_get;
    cache->find = D3Clock_find;
    cache->insert = D3Clock_insert;
    cache->evict = D3Clock_evict;
    cache->remove = D3Clock_remove;

    if (ccache_params.consider_obj_metadata) {
        cache->obj_md_size = 8;
    } else {
        cache->obj_md_size = 0;
    }

    return cache;
}

static void D3Clock_free(cache_t* cache) {
    auto* data = static_cast<D3Clock*>(cache->eviction_params);
    delete data;
    cache_struct_free(cache);
}

static bool D3Clock_get(cache_t* cache, const request_t* req) {
    return static_cast<D3Clock*>(cache->eviction_params)->get(cache, req);
}
static cache_obj_t* D3Clock_find(cache_t* cache, const request_t* req, const bool update_cache) {
    return static_cast<D3Clock*>(cache->eviction_params)->find(cache, req, update_cache);
}
static cache_obj_t* D3Clock_insert(cache_t* cache, const request_t* req) {
    return static_cast<D3Clock*>(cache->eviction_params)->insert(cache, req);
}
static cache_obj_t* D3Clock_to_evict(cache_t* cache, const request_t* req) {
    return static_cast<D3Clock*>(cache->eviction_params)->to_evict(cache, req);
}
static void D3Clock_evict(cache_t* cache, const request_t* req) {
    return static_cast<D3Clock*>(cache->eviction_params)->evict(cache, req);
}
static bool D3Clock_remove(cache_t* cache, const obj_id_t obj_id) {
    return static_cast<D3Clock*>(cache->eviction_params)->remove(cache, obj_id);
}

void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    assert(params.contains("cache_size"));
    auto* data = static_cast<D3Clock*>(cache->eviction_params);
    data->Initialize(cache, params);
}

}  // namespace D3Clock

cache_t* D3ClockInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = D3Clock::D3Clock_init(ccache_params, cache_specific_params);
    cache->cache_init = D3ClockInit;
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    data.SetParamsCallback = D3Clock::SetParams;
    return cache;
}
}  // namespace algorithm
