#include "ripq.hpp"

#include <libCacheSim/request.h>

#include <cstdint>

namespace RIPQ {

RIPQ::RIPQ(const uint64_t cache_size, const uint16_t n_section, const uint64_t block_size) {
    const uint64_t n_block = cache_size / block_size;
    blocks.reserve(n_block);
    for (size_t i = 0; i < n_block; i++) {
        blocks.emplace_back(Block(block_size));
    }
    uint64_t ip = n_block - 1;
    const uint64_t gap = n_block / n_section;
    sections.reserve(n_section);
    for (size_t i = 0; i < n_section; i++) {
        sections.emplace_back(Section(float(i) / n_section, float(i + 1) / n_section, ip));
        ip -= gap;
    }
}

void RIPQ::insert(const request_t* req, float priority) {
    auto obj = RIPQ_obj_metadata{
        .size = static_cast<uint64_t>(req->obj_size),
    };
    for (const auto& section : sections) {
        if (section.in_range(priority)) {
            break;
        }
    }
}

void RIPQ::increase(const request_t* req, float priority) {}
void RIPQ::delete_min() {}

}  // namespace RIPQ
