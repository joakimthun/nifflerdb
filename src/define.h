#pragma once

#include <cstddef>
#include <stdint.h>

namespace niffler {

    constexpr size_t PAGE_SIZE = 4096;
    constexpr size_t DEFAULT_PAGER_SIZE = 1000;
    using page_index = std::size_t;
    using u8 = uint8_t;
}