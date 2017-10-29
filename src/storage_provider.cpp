#include "storage_provider.h"

#include <stdlib.h>
#include <cstring>
#include <stdint.h>
#include <assert.h>

namespace niffler {

    storage_provider::storage_provider(offset base_offset, const char *file_path, file_mode file_mode)
        :
        base_offset_(base_offset),
        file_handle_(file_path, file_mode)
    {
    }

    storage_provider::~storage_provider()
    {
    }

    bool storage_provider::ok() const
    {
        return file_handle_.ok();
    }

    bool storage_provider::load(void *buffer, offset offset, size_t size) const
    {
        fseek(file_handle_.file, offset, SEEK_SET);
        return fread(buffer, size, 1, file_handle_.file) == 1;
    }

    bool storage_provider::store(void *value, offset offset, size_t size) const
    {
        fseek(file_handle_.file, offset, SEEK_SET);
        return fwrite(value, size, 1, file_handle_.file) == 1;
    }

    offset storage_provider::alloc_block(size_t size)
    {
        return 0;
    }

    void storage_provider::free_block(offset offset, size_t size)
    {
    }

    bool storage_provider::sync()
    {
        return fsync(file_handle_.file) == 0;
    }
}
