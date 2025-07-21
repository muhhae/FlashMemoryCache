#include <cstdint>

#include "cache/ripq.hpp"
namespace RIPQ {
class RIPQ_SLRU {
    RIPQ_SLRU(uint64_t cache_size, uint8_t n_cache, uint64_t n_ripq_section)
        : cache_size(cache_size), n_cache(n_cache), ripq(cache_size, n_ripq_section) {}

   private:
    RIPQ ripq;
    uint64_t cache_size;
    uint8_t n_cache;
};
}  // namespace RIPQ
