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
#include <vector>

#include "additional_data.hpp"
#include "math.hpp"

namespace algorithm {
namespace SxFIFO {

double normal_pdf(double x, double peak, double std_deviation) {
    const double inv_sqrt_2pi = 0.3989422804014327;
    double z = (x - peak) / std_deviation;
    return (inv_sqrt_2pi / std_deviation) * std::exp(-0.5 * z * z);
}

std::vector<double> discrete_proportions_point_eval(
    const std::vector<double>& X, double peak, double std_deviation = 1
) {
    std::vector<double> Y(X.size());
    double sum = 0.0;
    for (size_t i = 0; i < X.size(); ++i) {
        Y[i] = normal_pdf(X[i], peak, std_deviation);
        sum += Y[i];
    }
    if (sum == 0.0)
        return Y;
    for (double& v : Y)
        v /= sum;
    return Y;
}

struct SxFIFO_segment {
    std::list<cache_obj_t*> queue;
    uint64_t size;
    uint64_t occupied;
};
struct SxFIFO_Metadata {
    uint64_t freq = 0;
};

class SxFIFO {
   public:
    SxFIFO() = default;
    void Initialize(cache_t* cache, const std::unordered_map<std::string, std::string> params) {
        assert(params.contains("cache_size"));
        uint64_t cache_size = std::stoull(params.at("cache_size"));
        uint64_t freq_mean = params.contains("freq_mean") ? std::stoull(params.at("freq_mean")) : 1;
        float std_deviation = params.contains("std_deviation")
                                  ? std::stof(params.at("std_deviation"))
                                  : 1;
        uint64_t n_segment = params.contains("n_segment") ? std::stoull(params.at("n_segment")) : 3;

        std::vector<double> x;
        x.reserve(n_segment);
        segments.reserve(n_segment);

        for (size_t i = 0; i < n_segment; i++) {
            x.push_back(i);
        }
        auto ratios = discrete_proportions_point_eval(x, freq_mean, std_deviation);
        for (const auto& ratio : ratios) {
            uint64_t size = cache_size * ratio;
            if (size <= 0) {
                continue;
            }
            segments.push_back(SxFIFO_segment{.size = size});
        }
        cache->cache_size = segments[0].size;
        // for (const auto& x : ratios) {
        //     std::cout << x << "\n";
        // }
        // uint64_t sum = 0;
        // for (const auto& x : segments) {
        //     std::cout << x.size << "\n";
        //     sum += x.size;
        // }
        // std::cout << sum << "\n";
    }

    bool get(cache_t* cache, const request_t* req) {
        return cache_get_base(cache, req);
    }
    cache_obj_t* find(cache_t* cache, const request_t* req, const bool update_cache) {
        cache_obj_t* obj = cache_find_base(cache, req, update_cache);
        if (obj != nullptr && update_cache) {
            auto& metadata = metadatas.at(obj->obj_id);
            metadata.freq++;
        }
        return obj;
    }
    cache_obj_t* insert(cache_t* cache, const request_t* req) {
        cache_obj_t* obj = cache_insert_base(cache, req);
        segments[0].queue.push_front(obj);
        metadatas[obj->obj_id] = {};
        return obj;
    }
    cache_obj_t* to_evict(cache_t* cache, const request_t* req) {
        return segments[0].queue.back();
    }
    void evict(cache_t* cache, const request_t* req) {
        cache_obj_t* obj = segments[0].queue.back();
        const auto& metadata = metadatas.at(obj->obj_id);
        uint64_t index = std::min(segments.size() - 1, metadata.freq);
        if (index > 0) {
            data::AdditionalCacheDataStorage::GetStorage()
                .GetAdditionalCacheData(cache)
                .OnPromotion(obj, req);

            cache->occupied_byte -= obj->obj_size;
            auto& fifo = segments[index];
            while (fifo.occupied + obj->obj_size > fifo.size) {
                metadatas.erase(fifo.queue.back()->obj_id);
                fifo.occupied -= fifo.queue.back()->obj_size + cache->obj_md_size;
                cache_remove_obj_base(cache, fifo.queue.back(), true);

                // note: this code written because cache_remove_obj_base substract the
                // occupied_byte, but occupied_byte only represent the first queue
                cache->occupied_byte += fifo.queue.back()->obj_size + cache->obj_md_size;

                fifo.queue.pop_back();
            }
            fifo.occupied += obj->obj_size + cache->obj_md_size;
            fifo.queue.push_front(obj);
        } else {
            metadatas.erase(obj->obj_id);
            cache_remove_obj_base(cache, obj, true);
        }
        segments[0].queue.pop_back();
    }

    bool remove(cache_t* cache, const obj_id_t obj_id) {
        throw std::runtime_error("Remove is not yet supported");
    }

   private:
    std::vector<SxFIFO_segment> segments;
    std::unordered_map<obj_id_t, SxFIFO_Metadata> metadatas;
};

// ***********************************************************************
// ****                                                               ****
// ****                   function declarations                       ****
// ****                                                               ****
// ***********************************************************************

static void SxFIFO_free(cache_t* cache);
static bool SxFIFO_get(cache_t* cache, const request_t* req);
static cache_obj_t* SxFIFO_find(cache_t* cache, const request_t* req, const bool update_cache);
static cache_obj_t* SxFIFO_insert(cache_t* cache, const request_t* req);
static cache_obj_t* SxFIFO_to_evict(cache_t* cache, const request_t* req);
static void SxFIFO_evict(cache_t* cache, const request_t* req);
static bool SxFIFO_remove(cache_t* cache, const obj_id_t obj_id);

// ***********************************************************************
// ****                                                               ****
// ****                   end user facing functions                   ****
// ****                                                               ****
// ****                       init, free, get                         ****
// ***********************************************************************

cache_t* SxFIFO_init(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    cache_t* cache = cache_struct_init("SxFIFO", ccache_params, cache_specific_params);
    cache->eviction_params = new SxFIFO();
    cache->cache_init = SxFIFO_init;
    cache->cache_free = SxFIFO_free;

    cache->get = SxFIFO_get;
    cache->find = SxFIFO_find;
    cache->insert = SxFIFO_insert;
    cache->evict = SxFIFO_evict;
    cache->remove = SxFIFO_remove;

    if (ccache_params.consider_obj_metadata) {
        cache->obj_md_size = 8;
    } else {
        cache->obj_md_size = 0;
    }

    return cache;
}

static void SxFIFO_free(cache_t* cache) {
    auto* data = static_cast<SxFIFO*>(cache->eviction_params);
    delete data;
    cache_struct_free(cache);
}

static bool SxFIFO_get(cache_t* cache, const request_t* req) {
    return static_cast<SxFIFO*>(cache->eviction_params)->get(cache, req);
}
static cache_obj_t* SxFIFO_find(cache_t* cache, const request_t* req, const bool update_cache) {
    return static_cast<SxFIFO*>(cache->eviction_params)->find(cache, req, update_cache);
}
static cache_obj_t* SxFIFO_insert(cache_t* cache, const request_t* req) {
    return static_cast<SxFIFO*>(cache->eviction_params)->insert(cache, req);
}
static cache_obj_t* SxFIFO_to_evict(cache_t* cache, const request_t* req) {
    return static_cast<SxFIFO*>(cache->eviction_params)->to_evict(cache, req);
}
static void SxFIFO_evict(cache_t* cache, const request_t* req) {
    return static_cast<SxFIFO*>(cache->eviction_params)->evict(cache, req);
}
static bool SxFIFO_remove(cache_t* cache, const obj_id_t obj_id) {
    return static_cast<SxFIFO*>(cache->eviction_params)->remove(cache, obj_id);
}

void SetParams(cache_t* cache, std::unordered_map<std::string, std::string>& params) {
    assert(params.contains("cache_size"));
    auto* data = static_cast<SxFIFO*>(cache->eviction_params);
    data->Initialize(cache, params);
}

}  // namespace SxFIFO

cache_t* SxFIFOInit(const common_cache_params_t ccache_params, const char* cache_specific_params) {
    auto cache = SxFIFO::SxFIFO_init(ccache_params, cache_specific_params);

    cache->cache_init = SxFIFOInit;
    auto& data = data::AdditionalCacheDataStorage::GetStorage().GetAdditionalCacheData(cache);
    data.SetParamsCallback = SxFIFO::SetParams;
    return cache;
}
}  // namespace algorithm
