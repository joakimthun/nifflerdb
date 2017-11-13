#pragma once

#include <cstddef>
#include <stdint.h>

namespace niffler {

    constexpr size_t PAGE_SIZE = 4096;
    using offset = std::size_t;
    using u8 = uint8_t;
}