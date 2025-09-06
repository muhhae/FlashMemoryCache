#include <config.h>
#include <libCacheSim.h>
#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cassert>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace S3FClock {

class S3FClock {
   public:
    S3FClock() = default;
    void Initialize(cache_t* cache, const std::unordered_map<std::string, std::string> params) {
        hand_position = params.contains("h_position") ? std::stof(params.at("h_position")) : 0.1;
        assert(hand_position > 0 && hand_position < 1);
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
        cache_obj_t* obj = cache_insert_base(cache, req);
        queue.push_front(obj);
        obj->clock.freq = 0;
        return obj;
    }
    void evict(cache_t* cache, const request_t* req) {
        if (hand == queue.end()) {
            hand = queue.begin();
            std::advance(hand, queue.size() * hand_position);
        }
        // size_t index = std::distance(queue.begin(), hand);
        // std::cout << "Index: " << index << "\n";  // prints 2
        if ((*hand)->clock.freq == 0) {
            cache_evict_base(cache, *hand, true);
            queue.erase(hand);
            hand--;
            return;
        }
        (*hand)->clock.freq = 0;
        if (queue.back()->clock.freq == 0) {
            cache_evict_base(cache, queue.back(), true);
            queue.pop_back();
            hand--;
            return;
        }
        data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache).OnPromotion(
            queue.back(), req
        );
        queue.back()->clock.freq = 0;
        queue.push_front(queue.back());
        queue.pop_back();
        hand--;
    }
    cache_obj_t* to_evict(cache_t* cache, const request_t* req) {
        throw std::runtime_error("to_evict is not yet supported");
    }
    bool remove(cache_t* cache, const obj_id_t obj_id) {
        throw std::runtime_error("remove is not yet supported");
    }

   private:
    float hand_position;
    std::list<cache_obj_t*> queue;
    std::list<cache_obj_t*>::iterator hand = queue.end();
};

// ***********************************************************************
// ****                                                               ****
// ****                   function declarations                       ****
// ****                                                               ****
// ***********************************************************************

static void S3FClock_free(cache_t* cache);
static bool S3FClock_get(cache_t* cache, const request_t* req);
static cache_obj_t* S3FClock_find(cache_t* cache, const request_t* req, const bool update_cache);
static cache_obj_t* S3FClock_insert(cache_t* cache, const request_t* req);
static cache_obj_t* S3FClock_to_evict(cache_t* cache, const request_t* req);
static void S3FClock_evict(cache_t* cache, const request_t* req);
static bool S3FClock_remove(cache_t* cache, const obj_id_t obj_id);

// ***********************************************************************
// ****                                                               ****
// ****                   end user facing functions                   ****
// ****                                                               ****
// ****                       init, free, get                         ****
// ***********************************************************************

cache_t* S3FClock_init(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    cache_t* cache = cache_struct_init("S3FClock", ccache_params, cache_specific_params);
    cache->eviction_params = new S3FClock();
    cache->cache_init = S3FClock_init;
    cache->cache_free = S3FClock_free;

    cache->get = S3FClock_get;
    cache->find = S3FClock_find;
    cache->insert = S3FClock_insert;
    cache->evict = S3FClock_evict;
    cache->remove = S3FClock_remove;

    if (ccache_params.consider_obj_metadata) {
        cache->obj_md_size = 8;
    } else {
        cache->obj_md_size = 0;
    }

    return cache;
}

static void S3FClock_free(cache_t* cache) {
    auto* data = static_cast<S3FClock*>(cache->eviction_params);
    delete data;
    cache_struct_free(cache);
}

static bool S3FClock_get(cache_t* cache, const request_t* req) {
    return static_cast<S3FClock*>(cache->eviction_params)->get(cache, req);
}
static cache_obj_t* S3FClock_find(cache_t* cache, const request_t* req, const bool update_cache) {
    return static_cast<S3FClock*>(cache->eviction_params)->find(cache, req, update_cache);
}
static cache_obj_t* S3FClock_insert(cache_t* cache, const request_t* req) {
    return static_cast<S3FClock*>(cache->eviction_params)->insert(cache, req);
}
static cache_obj_t* S3FClock_to_evict(cache_t* cache, const request_t* req) {
    return static_cast<S3FClock*>(cache->eviction_params)->to_evict(cache, req);
}
static void S3FClock_evict(cache_t* cache, const request_t* req) {
    return static_cast<S3FClock*>(cache->eviction_params)->evict(cache, req);
}
static bool S3FClock_remove(cache_t* cache, const obj_id_t obj_id) {
    return static_cast<S3FClock*>(cache->eviction_params)->remove(cache, obj_id);
}

void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    assert(params.contains("cache_size"));
    auto* data = static_cast<S3FClock*>(cache->eviction_params);
    data->Initialize(cache, params);
}

}  // namespace S3FClock

cache_t* S3FClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = S3FClock::S3FClock_init(ccache_params, cache_specific_params);
    cache->cache_init = S3FClockInit;
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    data.SetParamsCallback = S3FClock::SetParams;
    return cache;
}
}  // namespace algorithm
