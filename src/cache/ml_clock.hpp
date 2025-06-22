#pragma once

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>
#include <onnxruntime/onnxruntime_c_api.h>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include "cache/common.hpp"

namespace mlclock {
template <typename T>
cache_t* MLClockInit(const common_cache_params_t ccache_params, const char* cache_specific_params);
class MLClockParam : public common::CustomParams {
   public:
	MLClockParam() : env(ORT_LOGGING_LEVEL_WARNING, "ml_clock") {
		session_options.SetIntraOpNumThreads(8);
	}
	MLClockParam(const Clock_params_t& base) : MLClockParam() {
		*(Clock_params_t*)this = base;
	}
	void LoadModel(std::filesystem::path model) {
		try {
			session = Ort::Session(env, model.c_str(), session_options);
		} catch (const Ort::Exception& e) {
			throw std::runtime_error(std::string("Failed to load model: ") + e.what());
		}
	}

	template <typename T>
	bool PromotionIsWasted(
		std::vector<T> input, std::array<int64_t, 2> shape, float treshold = 0.5
	);

   public:
	float treshold = 0.5;
	std::vector<std::string> features_name;
	std::optional<Ort::Session> session;
	Ort::Env env;
	Ort::SessionOptions session_options;
};
}  // namespace mlclock
