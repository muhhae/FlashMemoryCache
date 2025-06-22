#include <libCacheSim.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "experiment.hpp"
#include "lib/CLI11.hpp"

int main(int argc, char** argv) {
	options o;
	CLI::App app{"Offline Clock Simulator, based on libCacheSim"};
	app.add_option(
		"-f,--fixed-cache-sizes",
		o.fixed_cache_sizes,
		"Fixed cache sizes in MiB or object count if given "
		"--ignore-obj-size, can be more than one"
	);
	app.add_option(
		"-r,--relative-cache-sizes",
		o.relative_cache_sizes,
		"Relative cache sizes in floating number, can be more than one"
	);
	app.add_option("-o,--output-directory", o.output_directory, "Output directory")
		->default_val("./result");
	app.add_option("-i,--max-iteration", o.max_iteration, "Offline clock max iteration")
		->default_val(1);
	app.add_option(
		"-d,--descriptions",
		o.descs,
		"Additional description for experiment, would shows on filename, can be more than one"
	);
	app.add_option("trace_paths", o.trace_paths, "Can be more than one")->required();
	app.add_option(
		   "-a,--algo", o.algorithm, "available [offline-clock, clock, fifo, ML, decayed-clock]"
	)
		->required();
	app.add_option("-m,--model", o.ml_model, "ML model to use, required when algo = ML");
	app.add_option(
		   "-I, --input",
		   o.input_type,
		   "Input features datatype for Model Inference [I32, I64, F32]"
	)
		->default_val("I64");
	app.add_option(
		   "-F, --features",
		   o.features_name,
		   "Input features for Model Inference (The Sequence should "
		   "exactly the same as model input)"
	)
		->default_val(
			std::vector<std::string>{
				"rtime_between", "clock_freq", "lifetime_freq", "obj_size_relative"
			}
		);
	app.add_option("-T, --trace-type", o.trace_type, "Traces type [oracleGeneral, csv]")
		->default_val("oracleGeneral");
	app.add_option("-H, --treshold", o.treshold, "ML Model decision treshold")->default_val(0.5);
	app.add_option(
		   "-s, --flash-admission-treshold",
		   o.flash_admission_treshold,
		   "Min. freq an object has before admitted into Flash"
	)
		->default_val(1);
	app.add_option("-p, --decay-power", o.decay_power, "Decaying rate of clock and lifetime freq")
		->default_val(0.001);
	app.add_flag("--ignore-obj-size", o.ignore_obj_size, "Ignore object sizes from trace");
	app.add_flag("--generate-datasets", o.generate_datasets, "Generate datasets from simulation");
	app.add_flag("--id-num", o.id_num, "Skip the Hashing process of ObjectID");
	app.add_flag("--dram", o.dram_enabled, "Enable Dram Cache (1\% of the cache)");

	CLI11_PARSE(app, argc, argv);

	bool first = true;
	for (size_t i = 0; i < o.descs.size(); i++) {
		if (o.descs[i] == "")
			continue;
		o.desc += (first ? "" : ",") + o.descs[i];
		first = false;
	}

	RunExperiment(o);
	return 0;
}
