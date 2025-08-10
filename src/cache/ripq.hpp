#pragma once

#include <config.h>
#include <libCacheSim/cacheObj.h>
#include <libCacheSim/const.h>
#include <libCacheSim/request.h>
#include <sys/types.h>

#include <cstdint>
#include <cstdio>
#include <list>
#include <memory>
#include <print>
#include <unordered_map>
#include <vector>

namespace RIPQ {
class Block {
   public:
    Block(uint64_t block_size = 256 * MB) : max_capacity(block_size) {}
    void insert(const request_t* req);
    bool can_insert(const request_t* req);
    bool is_sealed() { return sealed; }

   private:
    uint64_t max_capacity;
    uint64_t used_capacity = 0;
    uint64_t object_count = 0;

    bool sealed = false;
};
class Section {
   public:
    Section(float priority_start, float priority_end, uint64_t active_block_id)
        : priority_start(priority_start),
          priority_end(priority_end),
          active_block_id(active_block_id) {}

    bool in_range(float priority) const {
        return priority_start <= priority && priority < priority_end;
    }

   private:
    float priority_start;
    float priority_end;
    uint64_t active_block_id;
};
struct RIPQ_obj_metadata {
    uint64_t block_id;
    uint64_t virtual_block_id;
    uint64_t size;

    uint64_t index;
    uint64_t virtual_index;
};
class RIPQ {
   public:
    RIPQ(
        const uint64_t cache_size,
        const uint16_t n_section = 8,
        const uint64_t block_size = 256 * MiB
    );
    cache_obj_t* find(const request_t* req);

    void insert(const request_t* req, float priority);
    void increase(const request_t* req, float priority);
    void delete_min();
    void update_range();

   private:
    std::vector<Section> sections;
    std::vector<Block> blocks;

    size_t tail = 0;
};
}  // namespace RIPQ
