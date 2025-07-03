#include <libCacheSim/enum.h>
#include <libCacheSim/reader.h>
#include <libCacheSim/request.h>

#include "gtest/gtest.h"
#include "utils.hpp"

namespace nlohmann {
void PrintTo(const nlohmann::json& j, std::ostream* os) { *os << "\n" << j.dump(2); }
}  // namespace nlohmann

TEST(SimulatorTEST, SingleLayer) {
    nlohmann::json results = LayeredCacheSimulation({"fifo"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":12377,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"fifo","hit":12377,"inserted":101495,"layer":0,"miss_ratio":0.891307784178727,"reinserted":0,"req":113872}],"miss_ratio":0.891307784178727,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_FIFO) {
    nlohmann::json results = LayeredCacheSimulation({"fifo"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":12377,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"fifo","hit":12377,"inserted":101495,"layer":0,"miss_ratio":0.891307784178727,"reinserted":0,"req":113872}],"miss_ratio":0.891307784178727,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_LRU) {
    nlohmann::json results = LayeredCacheSimulation({"lru"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":13657,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"lru","hit":13657,"inserted":100215,"layer":0,"miss_ratio":0.8800670928762119,"reinserted":13657,"req":113872}],"miss_ratio":0.8800670928762119,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"clock"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":13825,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"clock","hit":13825,"inserted":100047,"layer":0,"miss_ratio":0.8785917521427568,"reinserted":5607,"req":113872}],"miss_ratio":0.8785917521427568,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_OFFLINE_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"offline-clock"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":14056,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"offline-clock","hit":14056,"inserted":99816,"layer":0,"miss_ratio":0.876563158634256,"reinserted":2376,"req":113872}],"miss_ratio":0.876563158634256,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_SLRU) {
    nlohmann::json results = LayeredCacheSimulation({"slru"});
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 15556,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 1,
      "algorithm": "slru",
      "hit": 15556,
      "inserted": 98316,
      "layer": 0,
      "miss_ratio": 0.8633904735141211,
      "reinserted": 0,
      "req": 113872
    }
  ],
  "miss_ratio": 0.8633904735141211,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_GDSF) {
    nlohmann::json results = LayeredCacheSimulation({"gdsf"});
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 14832,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 1,
      "algorithm": "gdsf",
      "hit": 14832,
      "inserted": 99040,
      "layer": 0,
      "miss_ratio": 0.8697484895321063,
      "reinserted": 0,
      "req": 113872
    }
  ],
  "miss_ratio": 0.8697484895321063,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "fifo"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":18752,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"lru","hit":13657,"inserted":100215,"layer":0,"miss_ratio":0.8800670928762119,"reinserted":13657,"req":113872},{"admission_treshold":1,"algorithm":"fifo","hit":5095,"inserted":95410,"layer":1,"miss_ratio":0.9491593074888989,"reinserted":0,"req":100215}],"miss_ratio":0.8353238724181538,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_FIFO) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "fifo"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":18752,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"lru","hit":13657,"inserted":100215,"layer":0,"miss_ratio":0.8800670928762119,"reinserted":13657,"req":113872},{"admission_treshold":1,"algorithm":"fifo","hit":5095,"inserted":95410,"layer":1,"miss_ratio":0.9491593074888989,"reinserted":0,"req":100215}],"miss_ratio":0.8353238724181538,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_LRU) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "lru"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":19098,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"lru","hit":13657,"inserted":100215,"layer":0,"miss_ratio":0.8800670928762119,"reinserted":13657,"req":113872},{"admission_treshold":1,"algorithm":"lru","hit":5441,"inserted":94818,"layer":1,"miss_ratio":0.9457067305293618,"reinserted":5297,"req":100215}],"miss_ratio":0.8322853730504426,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "clock"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":19177,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"lru","hit":13657,"inserted":100215,"layer":0,"miss_ratio":0.8800670928762119,"reinserted":13657,"req":113872},{"admission_treshold":1,"algorithm":"clock","hit":5520,"inserted":94707,"layer":1,"miss_ratio":0.9449184253854214,"reinserted":1890,"req":100215}],"miss_ratio":0.8315916116341155,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_OFFLINE_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "offline-clock"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({"hit":19169,"iteration":1,"metrics":[{"admission_treshold":1,"algorithm":"lru","hit":13657,"inserted":100215,"layer":0,"miss_ratio":0.8800670928762119,"reinserted":13657,"req":113872},{"admission_treshold":1,"algorithm":"offline-clock","hit":5512,"inserted":94725,"layer":1,"miss_ratio":0.944998253754428,"reinserted":833,"req":100215}],"miss_ratio":0.8316618659547562,"req":113872})"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_SLRU) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "slru"});
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 20256,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 1,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 1,
      "algorithm": "slru",
      "hit": 6599,
      "inserted": 93640,
      "layer": 1,
      "miss_ratio": 0.9341515741156513,
      "reinserted": 0,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8221160601376984,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_GDSF) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "gdsf"});
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 19601,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 1,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 1,
      "algorithm": "gdsf",
      "hit": 5944,
      "inserted": 94272,
      "layer": 1,
      "miss_ratio": 0.9406875218280697,
      "reinserted": 0,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8278681326401574,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
