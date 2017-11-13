#include "storage_providers.h"

#include <stdlib.h>
#include <cstring>
#include <stdint.h>
#include <assert.h>

#include "define.h"

namespace niffler {

    memory_storage_provider::memory_storage_provider()
    {
        buffer_ = static_cast<u8*>(malloc(PAGE_SIZE * 10));
        size_ = PAGE_SIZE * 10;
    }

    memory_storage_provider::~memory_storage_provider()
    {
        if (buffer_ != nullptr)
            free(buffer_);
    }

    bool memory_storage_provider::ok() const
    {
        return buffer_ != nullptr;
    }

    bool memory_storage_provider::load(void *buffer, offset offset, size_t size)
    {
        resize_if_needed(offset + size);
        memcpy(buffer, buffer_ + offset, size);
        return true;
    }

    bool memory_storage_provider::store(void *value, offset offset, size_t size)
    {
        resize_if_needed(offset + size);
        memcpy(buffer_ + offset, value, size);
        return true;
    }

    bool memory_storage_provider::sync()
    {
        return true;
    }

    void memory_storage_provider::resize_if_needed(size_t min_size)
    {
        if (size_ >= min_size)
            return;

        auto new_size = size_ * 2;
        while (new_size < min_size)
        {
            new_size *= 2;
        }

        buffer_ = static_cast<u8*>(realloc(buffer_, new_size));
        size_ = new_size;
    }

    file_storage_provider::file_storage_provider(const char *file_path, file_mode file_mode)
        :
        file_handle_(file_path, file_mode)
    {
    }

    file_storage_provider::~file_storage_provider()
    {
    }

    bool file_storage_provider::ok() const
    {
        return file_handle_.ok();
    }

    bool file_storage_provider::load(void *buffer, offset offset, size_t size)
    {
        fseek(file_handle_.file, offset, SEEK_SET);
        return fread(buffer, size, 1, file_handle_.file) == 1;
    }

    bool file_storage_provider::store(void *value, offset offset, size_t size)
    {
        fseek(file_handle_.file, offset, SEEK_SET);
        return fwrite(value, size, 1, file_handle_.file) == 1;
    }

    bool file_storage_provider::sync()
    {
        return fsync(file_handle_.file) == 0;
    }

    offset file_storage_provider::alloc_block(size_t size)
    {
        return 0;
    }

    void file_storage_provider::free_block(offset offset, size_t size)
    {
    }
}
