#pragma once

#include <libCacheSim/cache.h>
#include <libCacheSim/evictionAlgo.h>

namespace decayed {
cache_t* DecayedClockInit(
	const common_cache_params_t ccache_params, const char* cache_specific_params
);
}  // namespace decayed
