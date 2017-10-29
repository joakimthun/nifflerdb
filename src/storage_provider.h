#pragma once

#include <vector>

#include "define.h"
#include "files.h"

namespace niffler {

    using std::size_t;

    struct storage_block
    {
        size_t size;
        offset block_offset;
    };

    class storage_provider
    {
    public:
        storage_provider(offset base_offset, const char *file_path, file_mode file_mode);
        ~storage_provider();
        bool ok() const;
        bool load(void *buffer, offset offset, size_t size) const;
        bool store(void *value, offset offset, size_t size) const;
        offset alloc_block(size_t size);
        void free_block(offset offset, size_t size);
        bool sync();
    private:
        offset base_offset_;
        size_t size_ = 0;
        std::vector<storage_block> free_blocks_;
        file_handle file_handle_;
    };
}
