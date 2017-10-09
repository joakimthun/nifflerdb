#include "storage_providers.h"

#include <stdlib.h>
#include <cstring>
#include <stdint.h>
#include <assert.h>

namespace niffler {

    mem_storage_provider::mem_storage_provider(size_t initial_size)
    {
        buffer_ = calloc(initial_size, 1);
        size_ = initial_size;
    }

    mem_storage_provider::~mem_storage_provider()
    {
        if (buffer_ != nullptr)
            free(buffer_);
    }

    bool mem_storage_provider::load(void *buffer, offset offset, size_t size) const
    {
        assert((size + offset) < size_);
        if ((size + offset) >= size_)
            return false;

        memcpy(buffer, (static_cast<uint8_t*>(buffer_) + offset), size);
        return true;
    }

    bool mem_storage_provider::store(void *value, offset offset, size_t size) const
    {
        if ((size + offset) >= size_)
        {
            auto x = 10;
        }

        assert((size + offset) < size_);
        if ((size + offset) >= size_)
            return false;

        memcpy((static_cast<uint8_t*>(buffer_) + offset), value, size);
        return true;
    }

}
