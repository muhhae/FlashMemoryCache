#include <libCacheSim/enum.h>
#include <libCacheSim/reader.h>
#include <libCacheSim/request.h>

#include "gtest/gtest.h"
#include "utils.hpp"

TEST(SimulatorTEST, SingleLayer) {
    auto r = LayeredCacheSimulation({"fifo"});
    ASSERT_EQ(r["hit"], 12377);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.891307784178727);
}
TEST(SimulatorTEST, SingleLayer_FIFO) {
    auto r = LayeredCacheSimulation({"fifo"});
    ASSERT_EQ(r["hit"], 12377);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.891307784178727);
}
TEST(SimulatorTEST, SingleLayer_LRU) {
    auto r = LayeredCacheSimulation({"lru"});
    ASSERT_EQ(r["hit"], 13657);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.8800670928762119);
}
TEST(SimulatorTEST, SingleLayer_CLOCK) {
    auto r = LayeredCacheSimulation({"clock"});
    ASSERT_EQ(r["hit"], 13825);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.8785917521427568);
}
TEST(SimulatorTEST, SingleLayer_OFFLINE_CLOCK) {
    auto r = LayeredCacheSimulation({"offline-clock"});
    ASSERT_EQ(r["hit"], 14056);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.876563158634256);
}
TEST(SimulatorTEST, DoubleLayer) {
    auto r = LayeredCacheSimulation({"lru", "fifo"});
    ASSERT_EQ(r["hit"], 18752);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.8353238724181538);
}
TEST(SimulatorTEST, DoubleLayer_FIFO) {
    auto r = LayeredCacheSimulation({"lru", "fifo"});
    ASSERT_EQ(r["hit"], 18752);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.8353238724181538);
}
TEST(SimulatorTEST, DoubleLayer_LRU) {
    std::cerr << "START\n";
    auto r = LayeredCacheSimulation({"lru", "lru"});
    std::cerr << "END\n";
    ASSERT_EQ(r["hit"], 18752);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.8353238724181538);
}
TEST(SimulatorTEST, DoubleLayer_CLOCK) {
    auto r = LayeredCacheSimulation({"lru", "clock"});
    ASSERT_EQ(r["hit"], 19177);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.8315916116341155);
}
TEST(SimulatorTEST, DoubleLayer_OFFLINE_CLOCK) {
    auto r = LayeredCacheSimulation({"lru", "offline-clock"});
    ASSERT_EQ(r["hit"], 19169);
    ASSERT_EQ(r["req"], 113872);
    ASSERT_EQ(r["miss_ratio"], 0.8316618659547562);
}
