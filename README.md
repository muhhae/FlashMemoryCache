# DRAM + Flash Cache Simulator
## Dependencies
1. [libCacheSim](https://github.com/1a1a11a/libCacheSim)
1. libglib-2.0
1. libzstd
2. ONNX Runtime
## Build
```
mkdir -p build
cd build
cmake .. && make
```
## Usage
```
./cacheSimulator [OPTIONS] trace_paths...


POSITIONALS:
  trace_paths TEXT ... REQUIRED
                              Can be more than one

OPTIONS:
  -h,     --help              Print this help message and exit
  -f,     --fixed-cache-sizes UINT ...
                              Fixed cache sizes in MiB or object count if given
                              --ignore-obj-size, can be more than one
  -r,     --relative-cache-sizes FLOAT ...
                              Relative cache sizes in floating number, can be more than one
  -o,     --output-directory TEXT [./result]
                              Output directory
  -i,     --max-iteration UINT [1]
                              Offline clock max iteration
  -d,     --descriptions TEXT ...
                              Additional description for experiment, would shows on filename,
                              can be more than one
  -a,     --algo TEXT REQUIRED
                              available [offline-clock, clock, fifo, ML, decayed-clock]
  -m,     --model TEXT        ML model to use, required when algo = ML
  -I,     --input TEXT [I64]  Input features datatype for Model Inference [I32, I64, F32]
  -F,     --features TEXT [[rtime_between,clock_freq,lifetime_freq,obj_size_relative]]  ...
                              Input features for Model Inference (The Sequence should exactly
                              the same as model input)
  -T,     --trace-type TEXT [oracleGeneral]
                              Traces type [oracleGeneral, csv]
  -H,     --treshold FLOAT [0.5]
                              ML Model decision treshold
  -s,     --flash-admission-treshold UINT [1]
                              Min. freq an object has before admitted into Flash
  -p,     --decay-power FLOAT [0.001]
                              Decaying rate of clock and lifetime freq
          --ignore-obj-size   Ignore object sizes from trace
          --generate-datasets Generate datasets from simulation
          --id-num            Skip the Hashing process of ObjectID
          --dram              Enable Dram Cache (1% of the cache)

```
## Example
1. Offline Clock Without DRAM (1 Layer Cache)
```
cd build && ./cacheSimulator -a offline-clock -r 0.1 --ignore-obj-size ../trace/small_zipf.oracleGeneral
[INFO]  06-23-2025 19:25:21 cache_size.h:26   (tid=139817414889600): calculating working set size...
[INFO]  06-23-2025 19:25:21 cache_size.h:54   (tid=139817414889600): working set size: 1000 object 1000 byte
{
  "cache_size": 100,
  "results": [
    {
      "iteration": 0,
      "metrics": [
        {
          "algorithm": "offline-clock",
          "hit": 589962,
          "layer": 0,
          "miss_ratio": 0.410038,
          "req": 1000000
        }
      ],
      "overall_hit": 589962,
      "overall_miss_ratio": 0.410038,
      "overall_req": 1000000
    },
    {
      "iteration": 1,
      "metrics": [
        {
          "algorithm": "offline-clock",
          "hit": 597566,
          "layer": 0,
          "miss_ratio": 0.40243399999999996,
          "req": 1000000
        }
      ],
      "overall_hit": 597566,
      "overall_miss_ratio": 0.40243399999999996,
      "overall_req": 1000000
    }
  ],
  "trace": "small_zipf.oracleGeneral"
}
```
2. Offline Clock With DRAM (2 Layer Cache), DRAM has size equal to 1% of Flash Cache
```
cd build && ./cacheSimulator -a offline-clock -r 0.1 --ignore-obj-size ../trace/small_zipf.oracleGeneral --dram
[INFO]  06-23-2025 19:22:12 cache_size.h:26   (tid=140079037268096): calculating working set size...
[INFO]  06-23-2025 19:22:13 cache_size.h:54   (tid=140079037268096): working set size: 1000 object 1000 byte
{
  "cache_size": 100,
  "results": [
    {
      "iteration": 0,
      "metrics": [
        {
          "algorithm": "lru",
          "hit": 29200,
          "layer": 0,
          "miss_ratio": 0.9708,
          "req": 1000000
        },
        {
          "algorithm": "offline-clock",
          "hit": 560704,
          "layer": 1,
          "miss_ratio": 0.4224309847548414,
          "req": 970800
        }
      ],
      "overall_hit": 589904,
      "overall_miss_ratio": 0.410096,
      "overall_req": 1000000
    },
    {
      "iteration": 1,
      "metrics": [
        {
          "algorithm": "lru",
          "hit": 29200,
          "layer": 0,
          "miss_ratio": 0.9708,
          "req": 1000000
        },
        {
          "algorithm": "offline-clock",
          "hit": 568290,
          "layer": 1,
          "miss_ratio": 0.4146168108776267,
          "req": 970800
        }
      ],
      "overall_hit": 597490,
      "overall_miss_ratio": 0.40251000000000003,
      "overall_req": 1000000
    }
  ],
  "trace": "small_zipf.oracleGeneral"
}
```
