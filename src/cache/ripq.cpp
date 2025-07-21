#include "ripq.hpp"

#include <libCacheSim/request.h>

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace RIPQ {

void RIPQ::insert(const request_t* req, float priority) {
    auto it = std::find_if(sections.begin(), sections.end(), [=](const Section& section) {
        return section.in_range(priority);
    });
    if (it == sections.end()) [[unlikely]] {
        throw std::runtime_error("Invalid priority value");
    }
    it->insert(req);
    update_range();
}
void RIPQ::increase(const request_t* req, float priority) {
    auto it = std::find_if(sections.begin(), sections.end(), [=](const Section& section) {
        return section.in_range(priority);
    });
    if (it == sections.end()) [[unlikely]] {
        throw std::runtime_error("Invalid priority value");
    }
    it->virtual_insert(req);
}
void RIPQ::delete_min() {}
void RIPQ::update_range() {}

void Section::insert(const request_t* req) {
    if (!active_block->can_insert(req)) {
        sealed_blocks.push_front(std::move(active_block));
        sealed_virtual_blocks.push_front(std::move(active_virtual_block));
        active_block = std::make_shared<Block>(block_size);
        active_virtual_block = std::make_shared<Block>(block_size);
    }
    active_block->insert(req);
}
void Section::virtual_insert(const request_t* req) { active_virtual_block->insert(req); }

bool Block::can_insert(const request_t* req) {
    if (req->obj_size > max_capacity) [[unlikely]] {
        throw std::runtime_error("Object is bigger than max block capacity");
    }
    sealed = used_capacity + req->obj_size > max_capacity;
    return !sealed;
}
void Block::insert(const request_t* req) {
    bucket[req->obj_id] = std::make_unique<cache_obj_t>(create_cache_obj_from_request(req));
    used_capacity += req->obj_size;
}
}  // namespace RIPQ
