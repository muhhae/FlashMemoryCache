cmake_minimum_required(VERSION 3.14)
project(cacheSimulator)

message(
  "****************** this example only works after libCacheSim has been installed ******************"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(GLib "2.40" REQUIRED)
include_directories(${GLib_INCLUDE_DIRS})
message(
  STATUS
    "glib found? ${GLib_FOUND}, lib = ${GLib_LIBRARY}, header = ${GLib_INCLUDE_DIRS}"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TCMALLOC REQUIRED libtcmalloc)

find_package(ZSTD)
# https://stackoverflow.com/questions/61377055/cannot-find-gflags-gflags-h-while-building-library-osx/61379123#61379123
include_directories(${ZSTD_INCLUDE_DIR})
if("${ZSTD_LIBRARIES}" STREQUAL "")
  message(FATAL_ERROR "zstd not found")
endif()

# find_library(ONNXRUNTIME_LIB onnxruntime)
# find_path(onnxruntimeInclude onnxruntime)
# message(STATUS "ONNX runtime found? ${onnxruntime} ${libCacheSimInclude}")
# include_directories(onnxruntimeInclude)

add_library(cachesim_libs INTERFACE)
target_include_directories(cachesim_libs INTERFACE "/usr/local/include")
target_link_libraries(cachesim_libs INTERFACE
    "/usr/local/lib/liblibCacheSim.a"
    ${ZSTD_LIBRARIES}
    ${TCMALLOC_LIBRARIES}
    ${GLib_LIBRARY}
    m
)

add_subdirectory(external/googletest)

enable_testing()

add_library(my_cache_sim
  src/simulator.cpp
  src/additional_data.cpp
  src/cache.cpp
  src/cache/offline_clock.cpp
  src/cache/offline_clock_v2.cpp
  src/cache/clock.cpp
  src/cache/lru.cpp
  src/cache/fifo.cpp
  src/cache/ripq.cpp
  src/cache/offline_q_clock.cpp
  src/cache/offline_qtime_clock.cpp
  src/cache/q_clock.cpp
  src/cache/q_auto.cpp
  src/cache/q2_auto.cpp
  src/cache/q3_auto.cpp
  src/cache/t2_auto.cpp
  src/cache/t3_auto.cpp
  src/cache/t4_auto.cpp
  src/cache/t5_auto.cpp
  src/cache/t6_auto.cpp
  src/cache/t7_auto.cpp
  src/cache/t_auto.cpp
  src/cache/time_auto.cpp
  src/cache/time2_auto.cpp
  src/cache/time3_auto.cpp
  src/cache/qor_clock.cpp
  src/cache/qand_clock.cpp
  src/cache/qand_clock_v2.cpp
  src/cache/qtime_clock.cpp
  src/cache/qtime_excl_clock.cpp
  src/cache/sxfifo.cpp
  src/cache/s3fclock.cpp
  src/cache/s3fclock_sequential.cpp
  # src/cache/decayed_clock.cpp
  # src/cache/dist_clock.cpp
)

add_executable(cacheSimulator src/main.cpp)
add_executable(run_tests tests/tests.cpp)

target_include_directories(cacheSimulator PRIVATE src .)
target_include_directories(my_cache_sim PRIVATE src .)
target_include_directories(run_tests PRIVATE src .)

target_link_libraries(my_cache_sim PRIVATE cachesim_libs)
target_link_libraries(cacheSimulator PRIVATE my_cache_sim)
target_link_libraries(run_tests PRIVATE my_cache_sim gtest_main)

include(GoogleTest)
gtest_discover_tests(run_tests)

# Experimentation
add_executable(ripq playgrounds/ripq.cpp)
target_include_directories(ripq PRIVATE src .)
target_link_libraries(ripq PRIVATE my_cache_sim)
