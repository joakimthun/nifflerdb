#pragma once

#include <cstddef>
#include <stdint.h>

namespace niffler {

    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using page_index = u32;

    constexpr u32 PAGE_SIZE = 4096;
    constexpr u32 DEFAULT_PAGER_SIZE = 1000;
    constexpr u32 NODE_DISK_SIZE_NO_CHILDREN = sizeof(page_index) + sizeof(page_index) + sizeof(page_index) + sizeof(u32);

    // 24 == bp_tree_record::DISK_SIZE()
    // 8  == page_header::DISK_SIZE() 
    constexpr u32 DEFAULT_TREE_ORDER = ((PAGE_SIZE - NODE_DISK_SIZE_NO_CHILDREN) / 24) - 8;
}