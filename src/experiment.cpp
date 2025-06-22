#include "experiment.hpp"
#include <libCacheSim/cache.h>
#include <libCacheSim/enum.h>
#include <libCacheSim/evictionAlgo.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "cache/base.hpp"
#include "cache/common.hpp"
#include "cache/decayed_clock.hpp"
#include "cache/dist_clock.hpp"
#include "cache/dram.hpp"
#include "cache/ml_clock.hpp"
#include "cache/my_clock.hpp"
#include "cache/offline_clock.hpp"
#include "lib/cache_size.h"

const std::string csv_header =
	"trace_path,ignore_obj_size,cache_size,miss_ratio,n_req,n_promoted,n_miss,n_hit\n";

std::function<
	cache_t*(const common_cache_params_t ccache_params, const char* cache_specific_params)>
AlgoSelector(const options& o) {
	if (o.algorithm == "decayed-clock") {
		return decayed::DecayedClockInit;
	}
	if (o.algorithm == "fifo") {
		return base::FIFOInit;
	}
	if (o.algorithm == "offline-clock") {
		return cclock::OfflineClockInit;
	}
	if (o.algorithm == "dist-optimal") {
		return distclock::DistClockInit;
	}
	if (o.algorithm == "lru") {
		return base::LRUInit;
	}
	if (o.algorithm == "clock") {
		return base::ClockInit;
	}
	if (o.algorithm == "my") {
		return myclock::MyClockInit;
	}
	if (o.algorithm == "ML") {
		if (o.ml_model == "") {
			throw std::runtime_error("ML model need to be provided in ONNX format");
		}
		if (o.input_type == "I32") {
			return mlclock::MLClockInit<int32_t>;
		}
		if (o.input_type == "I64") {
			return mlclock::MLClockInit<int64_t>;
		}
		if (o.input_type == "F32") {
			return mlclock::MLClockInit<float>;
		}
		throw std::runtime_error("Input type is not valid");
	}
	throw std::runtime_error("algorithm not found");
}

void RunExperiment(options o) {
	std::vector<std::future<void>> tasks;
	auto CacheInit = AlgoSelector(o);
	std::filesystem::create_directories(o.output_directory / "log");
	if (o.generate_datasets)
		std::filesystem::create_directories(o.output_directory / "datasets");

	for (const auto& p : o.trace_paths) {
		reader_init_param_t reader_init_param = {
			.ignore_obj_size = o.ignore_obj_size,
			.obj_id_is_num = o.id_num,
			.obj_id_is_num_set = o.id_num,
			.time_field = 1,
			.obj_id_field = 2,
			.obj_size_field = 3,
			.has_header = true
		};

		trace_type_e trace_type = ORACLE_GENERAL_TRACE;
		if (o.trace_type == "csv") {
			trace_type = CSV_TRACE;
		}
		reader_t* reader = open_trace(p.c_str(), trace_type, &reader_init_param);
		int64_t wss_obj = 0;
		int64_t wss_byte = 0;
		cal_working_set_size(reader, &wss_obj, &wss_byte);
		close_reader(reader);
		int64_t wss = o.ignore_obj_size ? wss_obj : wss_byte;
		const char* cache_specific_params = NULL;

		std::cout << csv_header;
		for (const auto& fcs : o.fixed_cache_sizes) {
			o.dist_optimal_treshold = o.ignore_obj_size ? fcs : fcs / wss_byte * wss_obj;
			std::string desc = "[" + std::to_string(fcs) + (o.ignore_obj_size ? "" : "MiB") +
							   (o.desc != "" ? "," : "") + o.desc + "]";
			tasks.emplace_back(
				std::async(
					std::launch::async,
					Simulate,
					CacheInit(
						{.cache_size = o.ignore_obj_size ? fcs : fcs * MiB}, cache_specific_params
					),
					p,
					o,
					desc
				)
			);
		}
		for (const auto& rcs : o.relative_cache_sizes) {
			o.dist_optimal_treshold = rcs * wss_obj;
			std::string s = std::to_string(rcs);
			s = s.substr(0, s.find_last_not_of('0') + 1);
			if (s.back() == '.')
				s.pop_back();

			std::string desc = "[" + s + (o.desc != "" ? "," : "") + o.desc + "]";
			tasks.emplace_back(
				std::async(
					std::launch::async,
					Simulate,
					CacheInit({.cache_size = uint64_t(wss * rcs)}, cache_specific_params),
					p,
					o,
					desc
				)
			);
		}
	}

	for (auto& t : tasks) {
		t.get();
	}
}

void Simulate(
	cache_t* cache, const std::filesystem::path trace_path, const options o, const std::string desc
) {
	reader_init_param_t reader_init_param = {
		.ignore_obj_size = o.ignore_obj_size,
		.obj_id_is_num = o.id_num,
		.obj_id_is_num_set = o.id_num,
		.time_field = 1,
		.obj_id_field = 2,
		.obj_size_field = 3,
		.has_header = true
	};

	trace_type_e trace_type = ORACLE_GENERAL_TRACE;
	if (o.trace_type == "csv") {
		trace_type = CSV_TRACE;
	}

	reader_t* reader = open_trace(trace_path.c_str(), trace_type, &reader_init_param);
	request_t* req = new_request();

	std::string base_path = std::filesystem::path(reader->trace_path).filename();
	size_t pos = base_path.find(".oracleGeneral");
	if (o.trace_type == "csv") {
		pos = base_path.find(".csv");
	}
	if (pos != std::string::npos) {
		base_path = base_path.substr(0, pos);
	}
	std::filesystem::path log_path = o.output_directory / "log" / (base_path + desc + ".csv");
	std::filesystem::path dataset_path =
		o.output_directory / "datasets" / (base_path + desc + ".csv");

	std::ofstream csv_file(log_path);
	csv_file << csv_header;

	uint64_t first_promoted = 0;
	common_cache_params_t* params = (common_cache_params_t*)cache->eviction_params;

	common::CustomParams* custom_params = (common::CustomParams*)cache->eviction_params;

	if (o.generate_datasets) {
		custom_params->datasets = std::ofstream(dataset_path);
		for (size_t i = 0; i < common::datasets_columns.size(); i++) {
			custom_params->datasets << common::datasets_columns[i]
									<< (i == common::datasets_columns.size() - 1 ? '\n' : ',');
		}
	}
	if (o.algorithm == "ML") {
		((mlclock::MLClockParam*)custom_params)->LoadModel(o.ml_model);
		((mlclock::MLClockParam*)custom_params)->features_name = o.features_name;
	}

	for (size_t i = 0; i < o.max_iteration; ++i) {
		auto tmp = clone_cache(cache);
		auto tmp_custom_params = (common::CustomParams*)tmp->eviction_params;
		std::swap(tmp_custom_params->objs_metadata, custom_params->objs_metadata);
		std::swap(tmp_custom_params->datasets, custom_params->datasets);
		if (o.algorithm == "ML") {
			auto tmp_ml_param = (mlclock::MLClockParam*)tmp_custom_params;
			auto ml_param = (mlclock::MLClockParam*)custom_params;
			std::swap(tmp_ml_param->session, ml_param->session);
			std::swap(tmp_ml_param->session_options, ml_param->session_options);
			std::swap(tmp_ml_param->env, ml_param->env);
			std::swap(tmp_ml_param->features_name, ml_param->features_name);
			tmp_ml_param->treshold = o.treshold;
		}
		tmp_custom_params->n_hit = 0;
		tmp_custom_params->n_req = 0;
		tmp_custom_params->n_promoted = 0;
		tmp_custom_params->dist_optimal_treshold = o.dist_optimal_treshold;
		tmp_custom_params->decay_power = o.decay_power;

		if (i == o.max_iteration - 1) {
			tmp_custom_params->generate_datasets = o.generate_datasets;
		}

		uint64_t dram_cache_size = tmp->cache_size / 100;
		cache_t* dram_cache = dram::LRUInit({.cache_size = dram_cache_size}, NULL);
		auto dram_param = (dram::DRAMParam*)dram_cache->eviction_params;
		dram_param->main_cache = tmp;

		while (read_one_req(reader, req) == 0) {
			if (o.dram_enabled) {
				bool hit = dram_cache->get(dram_cache, req);
				if (dram_param->req_map[req->obj_id] != NULL) {
					free(dram_param->req_map[req->obj_id]);
				}
				dram_param->req_map[req->obj_id] = clone_request(req);
				if (!hit) {
					auto& data = tmp_custom_params->objs_metadata[req->obj_id];
					common::OnAccessTracking(data, tmp_custom_params, req);
					tmp_custom_params->GlobalTracking(data);
					if (tmp->find(tmp, req, false) != NULL) {
						tmp_custom_params->n_hit++;
					}
					tmp_custom_params->n_req++;
				}
			} else {
				auto& data = tmp_custom_params->objs_metadata[req->obj_id];
				common::OnAccessTracking(data, tmp_custom_params, req);
				tmp_custom_params->GlobalTracking(data);
				if (tmp->get(tmp, req)) {
					tmp_custom_params->n_hit++;
				}
				tmp_custom_params->n_req++;
			}
		}

		const std::string csv_header =
			"trace_path,ignore_obj_size,cache_size,miss_ratio,n_req,n_promoted,n_miss,n_hit\n";
		std::ostringstream s;
		s << std::filesystem::path(reader->trace_path).filename();
		s << "," << reader->ignore_obj_size;
		s << "," << (reader->ignore_obj_size ? cache->cache_size : cache->cache_size / MiB);
		s << "," << 1 - (double)tmp_custom_params->n_hit / tmp_custom_params->n_req;
		s << "," << tmp_custom_params->n_req;
		s << "," << tmp_custom_params->n_promoted;
		s << "," << tmp_custom_params->n_req - tmp_custom_params->n_hit;
		s << "," << tmp_custom_params->n_hit;
		s << "\n";
		std::cout << s.str();
		csv_file << s.str();

		reset_reader(reader);
		if (i == 0)
			first_promoted = tmp_custom_params->n_promoted;
		for (auto& e : tmp_custom_params->objs_metadata) {
			e.second.Reset();
			e.second.lifetime_freq = 0;
			e.second.last_promotion = 0;
		}
		std::swap(tmp_custom_params->objs_metadata, custom_params->objs_metadata);
		std::swap(tmp_custom_params->datasets, custom_params->datasets);
		if (o.algorithm == "ML") {
			auto tmp_ml_param = (mlclock::MLClockParam*)tmp_custom_params;
			auto ml_param = (mlclock::MLClockParam*)custom_params;
			std::swap(tmp_ml_param->session, ml_param->session);
			std::swap(tmp_ml_param->session_options, ml_param->session_options);
			std::swap(tmp_ml_param->env, ml_param->env);
			std::swap(tmp_ml_param->features_name, ml_param->features_name);
		}
		tmp->cache_free(tmp);
	}
	csv_file.close();
	if (o.generate_datasets)
		custom_params->datasets.close();

	uint64_t sum = 0;
	for (const auto& e : custom_params->objs_metadata) {
		sum += e.second.wasted_promotions.size();
	}
	custom_params->objs_metadata.clear();
	free_request(req);
	cache->cache_free(cache);
	close_reader(reader);
}
