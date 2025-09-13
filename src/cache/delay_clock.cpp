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
namespace DelayClock {

class DelayClock {
   public:
    DelayClock() = default;
    void Initialize(cache_t* cache, const std::unordered_map<std::string, std::string> params) {
        assert(params.contains("cache_size"));
        cache_size = std::stoull(params.at("cache_size"));
        if (cache_size < 10) {
            std::cout << "cache size < 10, aborting...";
            exit(0);
        }
        delay_ratio = params.contains("delay_ratio") ? std::stof(params.at("delay_ratio")) : 0.1;
        frequency_limit = params.contains("n_bit") ? (1 << std::stoi(params.at("n_bit"))) - 1 : 1;
        assert(delay_ratio >= 0 && delay_ratio <= 1);
        threshold = uint64_t(cache_size * delay_ratio) + 1;
    }
    bool get(cache_t* cache, const request_t* req) {
        return cache_get_base(cache, req);
    }
    cache_obj_t* find(cache_t* cache, const request_t* req, const bool update_cache) {
        cache_obj_t* obj = cache_find_base(cache, req, update_cache);
        if (obj != nullptr && update_cache && obj->clock.freq < frequency_limit) {
            obj->clock.freq++;
        }
        return obj;
    }
    void reset_and_move_hand() {
        if (hand == queue.end()) {
            return;
        }
        auto* obj_to_reset = *hand--;
        obj_to_reset->clock.freq = metadatas.at(obj_to_reset->obj_id).last_freq;
    }
    cache_obj_t* insert(cache_t* cache, const request_t* req) {
        if (queue.size() == threshold) [[unlikely]] {
            hand--;
        }
        cache_obj_t* obj = cache_insert_base(cache, req);
        queue.push_front(obj);
        metadatas[obj->obj_id] = {};
        obj->clock.freq = 0;
        reset_and_move_hand();
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
        metadatas.at(obj_to_evict->obj_id).last_freq = obj_to_evict->clock.freq - 1;
        queue.push_front(obj_to_evict);
        reset_and_move_hand();
        queue.pop_back();
        data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache).OnPromotion(
            obj_to_evict, req
        );
    }

    cache_obj_t* to_evict(cache_t* cache, const request_t* req) {
        throw std::runtime_error("to_evict is not yet supported");
    }
    bool remove(cache_t* cache, const obj_id_t obj_id) {
        throw std::runtime_error("remove is not yet supported");
    }

   private:
    float delay_ratio;
    uint64_t cache_size;
    uint64_t threshold;
    uint64_t frequency_limit;
    std::list<cache_obj_t*> queue;
    std::list<cache_obj_t*>::iterator hand = queue.end();
    struct DelayClockMetadata {
        uint64_t last_freq;
    };
    std::unordered_map<obj_id_t, DelayClockMetadata> metadatas;
};

// ***********************************************************************
// ****                                                               ****
// ****                   function declarations                       ****
// ****                                                               ****
// ***********************************************************************

static void DelayClock_free(cache_t* cache);
static bool DelayClock_get(cache_t* cache, const request_t* req);
static cache_obj_t* DelayClock_find(cache_t* cache, const request_t* req, const bool update_cache);
static cache_obj_t* DelayClock_insert(cache_t* cache, const request_t* req);
static cache_obj_t* DelayClock_to_evict(cache_t* cache, const request_t* req);
static void DelayClock_evict(cache_t* cache, const request_t* req);
static bool DelayClock_remove(cache_t* cache, const obj_id_t obj_id);

// ***********************************************************************
// ****                                                               ****
// ****                   end user facing functions                   ****
// ****                                                               ****
// ****                       init, free, get                         ****
// ***********************************************************************

cache_t* DelayClock_init(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    cache_t* cache = cache_struct_init("DelayClock", ccache_params, cache_specific_params);
    cache->eviction_params = new DelayClock();
    cache->cache_init = DelayClock_init;
    cache->cache_free = DelayClock_free;

    cache->get = DelayClock_get;
    cache->find = DelayClock_find;
    cache->insert = DelayClock_insert;
    cache->evict = DelayClock_evict;
    cache->remove = DelayClock_remove;

    if (ccache_params.consider_obj_metadata) {
        cache->obj_md_size = 8;
    } else {
        cache->obj_md_size = 0;
    }

    return cache;
}

static void DelayClock_free(cache_t* cache) {
    auto* data = static_cast<DelayClock*>(cache->eviction_params);
    delete data;
    cache_struct_free(cache);
}

static bool DelayClock_get(cache_t* cache, const request_t* req) {
    return static_cast<DelayClock*>(cache->eviction_params)->get(cache, req);
}
static cache_obj_t* DelayClock_find(cache_t* cache, const request_t* req, const bool update_cache) {
    return static_cast<DelayClock*>(cache->eviction_params)->find(cache, req, update_cache);
}
static cache_obj_t* DelayClock_insert(cache_t* cache, const request_t* req) {
    return static_cast<DelayClock*>(cache->eviction_params)->insert(cache, req);
}
static cache_obj_t* DelayClock_to_evict(cache_t* cache, const request_t* req) {
    return static_cast<DelayClock*>(cache->eviction_params)->to_evict(cache, req);
}
static void DelayClock_evict(cache_t* cache, const request_t* req) {
    return static_cast<DelayClock*>(cache->eviction_params)->evict(cache, req);
}
static bool DelayClock_remove(cache_t* cache, const obj_id_t obj_id) {
    return static_cast<DelayClock*>(cache->eviction_params)->remove(cache, obj_id);
}

void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    assert(params.contains("cache_size"));
    auto* data = static_cast<DelayClock*>(cache->eviction_params);
    data->Initialize(cache, params);
}

}  // namespace DelayClock

cache_t* DelayClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = DelayClock::DelayClock_init(ccache_params, cache_specific_params);
    cache->cache_init = DelayClockInit;
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    data.SetParamsCallback = DelayClock::SetParams;
    return cache;
}
}  // namespace algorithm
