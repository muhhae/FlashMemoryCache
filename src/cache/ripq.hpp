#pragma once

#include <config.h>
#include <libCacheSim/cacheObj.h>
#include <libCacheSim/const.h>
#include <libCacheSim/request.h>

#include <cstdint>
#include <list>
#include <memory>
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
    std::unordered_map<obj_id_t, std::unique_ptr<cache_obj_t>> bucket;

    uint64_t max_capacity;
    uint64_t used_capacity = 0;

    bool sealed = false;
};
class Section {
   public:
    Section(float priority_start, float priority_end, uint64_t block_size = 256 * MB)
        : priority_start(priority_start), priority_end(priority_end), block_size(block_size) {}
    void insert(const request_t* req);
    void virtual_insert(const request_t* req);
    bool in_range(float priority) const {
        return priority_start <= priority && priority < priority_end;
    }

   private:
    std::unordered_map<obj_id_t, std::weak_ptr<Block>> block_table;
    std::unordered_map<obj_id_t, std::weak_ptr<Block>> virtual_block_table;

    std::list<std::shared_ptr<Block>> sealed_blocks;
    std::list<std::shared_ptr<Block>> sealed_virtual_blocks;

    std::shared_ptr<Block> active_block;
    std::shared_ptr<Block> active_virtual_block;

    float priority_start;
    float priority_end;
    uint64_t block_size;
};
class RIPQ {
   public:
    RIPQ(uint64_t total_capacity, uint16_t n_section) : total_capacity(total_capacity) {
        sections.reserve(n_section);
        for (size_t i = 0; i < n_section; i++) {
            sections.emplace_back(Section(float(i) / n_section, float(i + 1) / n_section));
        }
    }
    cache_obj_t* find(const request_t* req);

    void insert(const request_t* req, float priority);
    void increase(const request_t* req, float priority);
    void delete_min();
    void update_range();

   private:
    uint64_t total_capacity;
    uint64_t used_capacity;
    std::vector<Section> sections;
};
}  // namespace RIPQ
