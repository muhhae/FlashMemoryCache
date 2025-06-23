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
[INFO]  06-23-2025 16:50:38 cache_size.h:26   (tid=139963417235584): calculating working set size...
[INFO]  06-23-2025 16:50:38 cache_size.h:54   (tid=139963417235584): working set size: 1000 object 1000 byte
{
  "cache_size": 100,
  "results": [
    {
      "algorithm": "offline-clock",
      "layer": 0,
      "metrics": [
        {
          "hit": 589962,
          "iteration": 0,
          "miss_ratio": 0.410038,
          "req": 1000000
        },
        {
          "hit": 597566,
          "iteration": 1,
          "miss_ratio": 0.40243399999999996,
          "req": 1000000
        }
      ]
    }
  ],
  "trace": "small_zipf.oracleGeneral"
}
```
2. Offline Clock With DRAM (2 Layer Cache), DRAM has size equal to 1% of Flash Cache
```
cd build && ./cacheSimulator -a offline-clock -r 0.1 --ignore-obj-size ../trace/small_zipf.oracleGeneral --dram
[INFO]  06-23-2025 16:52:22 cache_size.h:26   (tid=140689779695744): calculating working set size...
[INFO]  06-23-2025 16:52:22 cache_size.h:54   (tid=140689779695744): working set size: 1000 object 1000 byte
{
  "cache_size": 100,
  "results": [
    {
      "algorithm": "lru",
      "layer": 0,
      "metrics": [
        {
          "hit": 29200,
          "iteration": 0,
          "miss_ratio": 0.9708,
          "req": 1000000
        },
        {
          "hit": 29200,
          "iteration": 1,
          "miss_ratio": 0.9708,
          "req": 1000000
        }
      ]
    },
    {
      "algorithm": "offline-clock",
      "layer": 1,
      "metrics": [
        {
          "hit": 560704,
          "iteration": 0,
          "miss_ratio": 0.4224309847548414,
          "req": 970800
        },
        {
          "hit": 568290,
          "iteration": 1,
          "miss_ratio": 0.4146168108776267,
          "req": 970800
        }
      ]
    }
  ],
  "trace": "small_zipf.oracleGeneral"
}
```
