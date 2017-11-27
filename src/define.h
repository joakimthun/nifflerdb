#pragma once

#include <cstddef>
#include <stdint.h>

namespace niffler {

    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;

    constexpr u32 PAGE_SIZE = 4096;
    constexpr u32 DEFAULT_PAGER_SIZE = 1000;

    using page_index = u32;
}