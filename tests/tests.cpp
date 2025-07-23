#include <libCacheSim/enum.h>
#include <libCacheSim/reader.h>
#include <libCacheSim/request.h>

#include "gtest/gtest.h"
#include "utils.hpp"

namespace nlohmann {
void PrintTo(const nlohmann::json& j, std::ostream* os) { *os << "\n" << j.dump(2); }
}  // namespace nlohmann

TEST(SimulatorTEST, SingleLayer_FIFO) {
    nlohmann::json results = LayeredCacheSimulation({"fifo"}, 0);
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 12377,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
              "algorithm": "fifo",
              "hit": 12377,
              "inserted": 101495,
              "layer": 0,
              "miss_ratio": 0.891307784178727,
              "reinserted": 0,
              "req": 113872
            }
          ],
          "miss_ratio": 0.891307784178727,
          "req": 113872
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_LRU) {
    nlohmann::json results = LayeredCacheSimulation({"lru"}, 0);
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 13657,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
              "algorithm": "lru",
              "hit": 13657,
              "inserted": 100215,
              "layer": 0,
              "miss_ratio": 0.8800670928762119,
              "reinserted": 13657,
              "req": 113872
            }
          ],
          "miss_ratio": 0.8800670928762119,
          "req": 113872
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"clock"}, 0);
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 13825,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
              "algorithm": "clock",
              "hit": 13825,
              "inserted": 100047,
              "layer": 0,
              "miss_ratio": 0.8785917521427568,
              "reinserted": 5607,
              "req": 113872
            }
          ],
          "miss_ratio": 0.8785917521427568,
          "req": 113872
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_OFFLINE_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"offline-clock"}, 0);
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 14056,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
              "algorithm": "offline-clock",
              "hit": 14056,
              "inserted": 99816,
              "layer": 0,
              "miss_ratio": 0.876563158634256,
              "reinserted": 2376,
              "req": 113872
            }
          ],
          "miss_ratio": 0.876563158634256,
          "req": 113872
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_SLRU) {
    nlohmann::json results = LayeredCacheSimulation({"slru"}, 0);
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 15556,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
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
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayer_GDSF) {
    nlohmann::json results = LayeredCacheSimulation({"gdsf"}, 0);
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 14832,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
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
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_FIFO) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "fifo"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 18752,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
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
              "algorithm": "fifo",
              "hit": 5095,
              "inserted": 95410,
              "layer": 1,
              "miss_ratio": 0.9491593074888989,
              "reinserted": 0,
              "req": 100215
            }
          ],
          "miss_ratio": 0.8353238724181538,
          "req": 113872
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_LRU) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "lru"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 19098,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
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
              "algorithm": "lru",
              "hit": 5441,
              "inserted": 94818,
              "layer": 1,
              "miss_ratio": 0.9457067305293618,
              "reinserted": 5297,
              "req": 100215
            }
          ],
          "miss_ratio": 0.8322853730504426,
          "req": 113872
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "clock"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 19177,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
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
              "algorithm": "clock",
              "hit": 5520,
              "inserted": 94707,
              "layer": 1,
              "miss_ratio": 0.9449184253854214,
              "reinserted": 1890,
              "req": 100215
            }
          ],
          "miss_ratio": 0.8315916116341155,
          "req": 113872
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_OFFLINE_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "offline-clock"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 19169,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
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
              "algorithm": "offline-clock",
              "hit": 5512,
              "inserted": 94725,
              "layer": 1,
              "miss_ratio": 0.944998253754428,
              "reinserted": 833,
              "req": 100215
            }
          ],
          "miss_ratio": 0.8316618659547562,
          "req": 113872
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_SLRU) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "slru"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 20256,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
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
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_GDSF) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "gdsf"});
    nlohmann::json expected = nlohmann::json::parse(
        R"({
          "hit": 19601,
          "iteration": 1,
          "metrics": [
            {
              "admission_treshold": 0,
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
        })"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_FIFO_ADMISSION_3_LIFETIME_FREQ) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "fifo"}, 3, true);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 19663,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "fifo",
      "hit": 6006,
      "inserted": 20278,
      "layer": 1,
      "miss_ratio": 0.9400688519682683,
      "reinserted": 0,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8273236616551918,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_LRU_ADMISSION_3_LIFETIME_FREQ) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "lru"}, 3, true);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 19569,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "lru",
      "hit": 5912,
      "inserted": 20363,
      "layer": 1,
      "miss_ratio": 0.9410068353040962,
      "reinserted": 5807,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8281491499227203,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_CLOCK_ADMISSION_3_LIFETIME_FREQ) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "clock"}, 3, true);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 19509,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "clock",
      "hit": 5852,
      "inserted": 20416,
      "layer": 1,
      "miss_ratio": 0.941605548071646,
      "reinserted": 1188,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8286760573275256,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_OFFLINE_CLOCK_ADMISSION_3_LIFETIME_FREQ) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "offline-clock"}, 3, true);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 19697,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "offline-clock",
      "hit": 6040,
      "inserted": 20231,
      "layer": 1,
      "miss_ratio": 0.93972958139999,
      "reinserted": 339,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8270250807924687,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_SLRU_ADMISSION_3_LIFETIME_FREQ) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "slru"}, 3, true);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 21578,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "slru",
      "hit": 7921,
      "inserted": 18332,
      "layer": 1,
      "miss_ratio": 0.9209599361373048,
      "reinserted": 0,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8105065336518196,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_GDSF_ADMISSION_3_LIFETIME_FREQ) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "gdsf"}, 3, true);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 19743,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "gdsf",
      "hit": 6086,
      "inserted": 20173,
      "layer": 1,
      "miss_ratio": 0.9392705682782019,
      "reinserted": 0,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8266211184487846,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_FIFO_ADMISSION_3) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "fifo"}, 3);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 15880,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "fifo",
      "hit": 2223,
      "inserted": 1189,
      "layer": 1,
      "miss_ratio": 0.9778176919622811,
      "reinserted": 0,
      "req": 100215
    }
  ],
  "miss_ratio": 0.860545173528172,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_LRU_ADMISSION_3) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "lru"}, 3);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 15928,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "lru",
      "hit": 2271,
      "inserted": 1178,
      "layer": 1,
      "miss_ratio": 0.9773387217482413,
      "reinserted": 499,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8601236476043277,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_CLOCK_ADMISSION_3) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "clock"}, 3);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 15928,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "clock",
      "hit": 2271,
      "inserted": 1178,
      "layer": 1,
      "miss_ratio": 0.9773387217482413,
      "reinserted": 28,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8601236476043277,
  "req": 113872
}

)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_OFFLINE_CLOCK_ADMISSION_3) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "offline-clock"}, 3);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 15928,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "offline-clock",
      "hit": 2271,
      "inserted": 1178,
      "layer": 1,
      "miss_ratio": 0.9773387217482413,
      "reinserted": 28,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8601236476043277,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_SLRU_ADMISSION_3) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "slru"}, 3);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 15928,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "slru",
      "hit": 2271,
      "inserted": 1178,
      "layer": 1,
      "miss_ratio": 0.9773387217482413,
      "reinserted": 0,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8601236476043277,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, DoubleLayer_GDSF_ADMISSION_3) {
    nlohmann::json results = LayeredCacheSimulation({"lru", "gdsf"}, 3);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 15928,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 0,
      "algorithm": "lru",
      "hit": 13657,
      "inserted": 100215,
      "layer": 0,
      "miss_ratio": 0.8800670928762119,
      "reinserted": 13657,
      "req": 113872
    },
    {
      "admission_treshold": 3,
      "algorithm": "gdsf",
      "hit": 2271,
      "inserted": 1178,
      "layer": 1,
      "miss_ratio": 0.9773387217482413,
      "reinserted": 0,
      "req": 100215
    }
  ],
  "miss_ratio": 0.8601236476043277,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
#undef SINGLE_LAYER_ADMISSIOn
#ifdef SINGLE_LAYER_ADMISSION
SINGLE_LAYER_ADMISSIONTEST(SimulatorTEST, SingleLayerWithAdmission_FIFO) {
    nlohmann::json results = LayeredCacheSimulation({"fifo"}, 4);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 12506,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 4,
      "algorithm": "fifo",
      "hit": 12506,
      "inserted": 15381,
      "layer": 0,
      "miss_ratio": 0.8901749332583954,
      "reinserted": 0,
      "req": 113872
    }
  ],
  "miss_ratio": 0.8901749332583954,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayerWithAdmission_LRU) {
    nlohmann::json results = LayeredCacheSimulation({"lru"}, 4);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 13843,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 4,
      "algorithm": "lru",
      "hit": 13843,
      "inserted": 14044,
      "layer": 0,
      "miss_ratio": 0.8784336799213152,
      "reinserted": 13843,
      "req": 113872
    }
  ],
  "miss_ratio": 0.8784336799213152,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayerWithAdmission_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"clock"}, 4);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 13790,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 4,
      "algorithm": "clock",
      "hit": 13790,
      "inserted": 14097,
      "layer": 0,
      "miss_ratio": 0.8788991147955599,
      "reinserted": 3743,
      "req": 113872
    }
  ],
  "miss_ratio": 0.8788991147955599,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayerWithAdmission_OFFLINE_CLOCK) {
    nlohmann::json results = LayeredCacheSimulation({"offline-clock"}, 4);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 13930,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 4,
      "algorithm": "offline-clock",
      "hit": 13930,
      "inserted": 13957,
      "layer": 0,
      "miss_ratio": 0.8776696641843473,
      "reinserted": 1987,
      "req": 113872
    }
  ],
  "miss_ratio": 0.8776696641843473,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayerWithAdmission_SLRU) {
    nlohmann::json results = LayeredCacheSimulation({"slru"}, 4);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 13492,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 4,
      "algorithm": "slru",
      "hit": 13492,
      "inserted": 14395,
      "layer": 0,
      "miss_ratio": 0.8815160882394267,
      "reinserted": 0,
      "req": 113872
    }
  ],
  "miss_ratio": 0.8815160882394267,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
TEST(SimulatorTEST, SingleLayerWithAdmission_GDSF) {
    nlohmann::json results = LayeredCacheSimulation({"gdsf"}, 4);
    nlohmann::json expected = nlohmann::json::parse(
        R"(
{
  "hit": 13978,
  "iteration": 1,
  "metrics": [
    {
      "admission_treshold": 4,
      "algorithm": "gdsf",
      "hit": 13978,
      "inserted": 13909,
      "layer": 0,
      "miss_ratio": 0.8772481382605031,
      "reinserted": 0,
      "req": 113872
    }
  ],
  "miss_ratio": 0.8772481382605031,
  "req": 113872
}
)"
    );
    ASSERT_EQ(results, expected);
}
#endif  // SINGLE_LAYER_ADMISSION
