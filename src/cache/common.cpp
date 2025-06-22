#include "common.hpp"
#include <libCacheSim/cacheObj.h>
#include <libCacheSim/reader.h>
#include <libCacheSim/request.h>
#include <sys/types.h>
#include <cmath>
#include <unordered_map>

std::unordered_map<std::string, float> common::CandidateMetadata(
	const common::ObjMetadata& data, common::CustomParams* params, const cache_t* cache,
	const request_t* current_req, const cache_obj_t* obj_to_evict
) {
	float rtime_since = current_req->clock_time - data.rtime;
	float vtime_since = params->vtime - data.vtime;

	TrackRunningMean(rtime_since, params->rm_rtime_since);
	TrackRunningMean(log(rtime_since + 1), params->rm_rtime_since_log);

	TrackRunningMean(vtime_since, params->rm_vtime_since);
	TrackRunningMean(log(vtime_since + 1), params->rm_vtime_since_log);

	std::unordered_map<std::string, float> features;

	features["obj_id"] = obj_to_evict->obj_id;
	features["obj_size_relative"] = (float)obj_to_evict->obj_size / cache->cache_size;

	features["rtime_since"] = rtime_since;
	features["rtime_since_std"] = common::RunningMeanNormalize(rtime_since, params->rm_rtime_since);
	features["rtime_since_log"] = log(rtime_since + 1);
	features["rtime_since_log_std"] =
		common::RunningMeanNormalize(log(rtime_since + 1), params->rm_rtime_since_log);

	features["vtime_since"] = vtime_since;
	features["vtime_since_std"] = common::RunningMeanNormalize(vtime_since, params->rm_vtime_since);
	features["vtime_since_log"] = log(vtime_since + 1);
	features["vtime_since_log_std"] =
		common::RunningMeanNormalize(log(vtime_since + 1), params->rm_vtime_since_log);

	features["rtime_between"] = data.rtime_between;
	features["rtime_between_std"] =
		common::RunningMeanNormalize(data.rtime_between, params->rm_rtime_between);
	features["rtime_between_log"] = log(data.rtime_between + 1);
	features["rtime_between_log_std"] =
		common::RunningMeanNormalize(log(data.rtime_between + 1), params->rm_rtime_between_log);

	features["clock_freq"] = data.clock_freq;
	features["clock_freq_decayed_rtime"] = data.clock_freq_decayed_rtime;
	features["clock_freq_decayed_vtime"] = data.clock_freq_decayed_vtime;
	features["clock_freq_std"] =
		common::RunningMeanNormalize(data.clock_freq, params->rm_clock_freq);
	features["clock_freq_log"] = log(data.clock_freq + 1);
	features["clock_freq_log_std"] =
		common::RunningMeanNormalize(log(data.clock_freq + 1), params->rm_clock_freq_log);

	features["lifetime_freq"] = data.lifetime_freq;
	features["lifetime_freq_decayed_rtime"] =
		data.lifetime_freq_decayed_rtime * exp(-params->decay_power * rtime_since);
	features["lifetime_freq_decayed_vtime"] =
		data.lifetime_freq_decayed_vtime * exp(-params->decay_power * vtime_since);
	features["lifetime_freq_std"] =
		common::RunningMeanNormalize(data.lifetime_freq, params->rm_lifetime_freq);
	features["lifetime_freq_log"] = log(data.lifetime_freq + 1);
	features["lifetime_freq_log_std"] =
		common::RunningMeanNormalize(log(data.lifetime_freq + 1), params->rm_lifetime_freq_log);

	return features;
}

void common::ObjMetadata::Reset() {
	obj_size_relative = 0;
	clock_freq = 0;
	rtime_between = 0;
	rtime = 0;
	obj_size = 0;
	vtime = 0;

	lifetime_freq_decayed_rtime = 0;
	lifetime_freq_decayed_vtime = 0;
	clock_freq_decayed_rtime = 0;
	clock_freq_decayed_vtime = 0;
}

void common::OnAccessTracking(
	ObjMetadata& data, CustomParams* custom_params, const request_t* req
) {
	uint64_t rtime_since = req->clock_time - data.rtime;
	uint64_t vtime_since = custom_params->vtime - data.vtime;

	data.rtime_between = req->clock_time - data.rtime;
	data.rtime = req->clock_time;
	data.vtime = custom_params->vtime++;

	data.obj_size = req->obj_size;

	data.clock_freq++;
	data.lifetime_freq++;

	data.clock_freq_decayed_rtime =
		data.clock_freq_decayed_rtime * exp(-custom_params->decay_power * rtime_since);
	data.clock_freq_decayed_vtime =
		data.clock_freq_decayed_vtime * exp(-custom_params->decay_power * vtime_since);

	data.lifetime_freq_decayed_rtime =
		data.lifetime_freq_decayed_rtime * exp(-custom_params->decay_power * rtime_since);
	data.lifetime_freq_decayed_vtime =
		data.lifetime_freq_decayed_vtime * exp(-custom_params->decay_power * vtime_since);

	data.clock_freq_decayed_rtime++;
	data.clock_freq_decayed_vtime++;
	data.lifetime_freq_decayed_rtime++;
	data.lifetime_freq_decayed_vtime++;
}

void common::BeforeEvaluationTracking(
	const cache_obj_t* obj, CustomParams* custom_params, const request_t* req
) {
	auto& data = custom_params->objs_metadata[obj->obj_id];

	uint64_t rtime_since = req->clock_time - data.rtime;
	uint64_t vtime_since = custom_params->vtime - data.vtime;

	data.clock_freq_decayed_rtime =
		data.clock_freq_decayed_rtime * exp(-custom_params->decay_power * rtime_since);
	data.clock_freq_decayed_vtime =
		data.clock_freq_decayed_vtime * exp(-custom_params->decay_power * vtime_since);
}

void common::BeforeEvictionTracking(
	const cache_obj_t* obj, CustomParams* custom_params, const request_t* req
) {
	auto& data = custom_params->objs_metadata[obj->obj_id];

	data.clock_freq_decayed_rtime = 0;
	data.clock_freq_decayed_vtime = 0;
	data.clock_freq = 0;
}

void common::OnPromotionTracking(
	const cache_obj_t* obj, CustomParams* custom_params, const request_t* req
) {
	auto& data = custom_params->objs_metadata[obj->obj_id];
	// data.Reset();
}

void common::TrackRunningMean(const float X, RunningMeanData& d) {
	d.n++;
	float d1 = X - d.mean;
	d.mean += d1 / d.n;
	float d2 = X - d.mean;
	d.m2 += d1 * d2;
}

float common::RunningMeanNormalize(const float X, RunningMeanData& d) {
	float variance = d.m2 / (d.n - 1);
	float std = sqrt(variance);
	if (std == 0 || std::isnan(std)) {
		return 0;
	}
	float norm = (X - d.mean) / std;
	return norm;
}

void common::CustomParams::GlobalTracking(const common::ObjMetadata& data) {
	if (data.lifetime_freq > max_lifetime_freq) {
		max_lifetime_freq = data.lifetime_freq;
	}

	if (data.clock_freq > max_clock_freq) {
		max_clock_freq = data.clock_freq;
	}

	if (data.rtime_between > max_rtime_between) {
		max_rtime_between = data.rtime_between;
	}

	TrackRunningMean(data.clock_freq, rm_clock_freq);
	TrackRunningMean(data.lifetime_freq, rm_lifetime_freq);
	TrackRunningMean(data.rtime_between, rm_rtime_between);

	TrackRunningMean(log(data.clock_freq + 1), rm_clock_freq_log);
	TrackRunningMean(log(data.lifetime_freq + 1), rm_lifetime_freq_log);
	TrackRunningMean(log(data.rtime_between + 1), rm_rtime_between_log);
}
