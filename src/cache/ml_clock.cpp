#include "ml_clock.hpp"

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <onnxruntime/onnxruntime_c_api.h>
#include <onnxruntime/onnxruntime_cxx_api.h>

#include <algorithm>
#include <vector>

#include "common.hpp"

template <typename T>
bool mlclock::MLClockParam::PromotionIsWasted(
    std::vector<T> input, std::array<int64_t, 2> shape, float treshold
) {
    Ort::AllocatorWithDefaultOptions allocator;
    auto input_name = session->GetInputNameAllocated(0, allocator);
    auto output_label = session->GetOutputNameAllocated(0, allocator);
    auto output_proba = session->GetOutputNameAllocated(1, allocator);
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    Ort::Value input_tensor = Ort::Value::CreateTensor<T>(
        memory_info, input.data(), input.size(), shape.data(), shape.size()
    );
    std::array<const char*, 1> input_names = {input_name.get()};
    std::array<const char*, 2> output_names = {output_label.get(), output_proba.get()};
    std::vector<Ort::Value> output_tensors = session->Run(
        Ort::RunOptions{nullptr},
        input_names.data(),
        &input_tensor,
        input_names.size(),
        output_names.data(),
        output_names.size()
    );
    Ort::Value& prob_seq = output_tensors[1];
    float prob =
        prob_seq.GetValue(0, allocator).GetValue(1, allocator).GetTensorMutableData<float>()[1];
    return prob >= treshold;
}

template <typename T>
void MLClockEvict(cache_t* cache, const request_t* req) {
    Clock_params_t* params = (Clock_params_t*)cache->eviction_params;
    mlclock::MLClockParam* custom_params = (mlclock::MLClockParam*)cache->eviction_params;

    cache_obj_t* obj_to_evict = params->q_tail;
    while (obj_to_evict->clock.freq >= 1) {
        auto data = custom_params->objs_metadata[obj_to_evict->obj_id];
        common::BeforeEvaluationTracking(obj_to_evict, custom_params, req);

        auto features = common::CandidateMetadata(data, custom_params, cache, req, obj_to_evict);
        std::vector<T> input_features;
        input_features.reserve(custom_params->features_name.size());
        for (const auto& f : custom_params->features_name) {
            input_features.push_back(features[f]);
        }
        bool wasted = custom_params->PromotionIsWasted(
            input_features, {1, static_cast<long>(input_features.size())}, custom_params->treshold
        );

        common::BeforeEvictionTracking(obj_to_evict, custom_params, req);
        if (wasted) {
            break;
        }
        common::OnPromotionTracking(obj_to_evict, custom_params, req);
        obj_to_evict->clock.freq -= 1;
        params->n_obj_rewritten += 1;
        params->n_byte_rewritten += obj_to_evict->obj_size;
        move_obj_to_head(&params->q_head, &params->q_tail, obj_to_evict);
        custom_params->n_promoted++;
        obj_to_evict = params->q_tail;
    }
    remove_obj_from_list(&params->q_head, &params->q_tail, obj_to_evict);
    cache_evict_base(cache, obj_to_evict, true);
}

template <typename T>
cache_t* mlclock::MLClockInit(
    const common_cache_params_t ccache_params, const char* cache_specific_params
) {
    auto cache = Clock_init(ccache_params, cache_specific_params);

    cache->cache_init = MLClockInit<T>;
    cache->evict = MLClockEvict<T>;

    MLClockParam* params = new MLClockParam(*(Clock_params_t*)cache->eviction_params);
    free(cache->eviction_params);

    cache->eviction_params = params;
    return cache;
}

template cache* mlclock::MLClockInit<float>(common_cache_params_t, const char*);
template cache* mlclock::MLClockInit<double>(common_cache_params_t, const char*);
template cache* mlclock::MLClockInit<int>(common_cache_params_t, const char*);
template cache* mlclock::MLClockInit<long>(common_cache_params_t, const char*);
