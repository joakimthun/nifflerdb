#include "memory_storage_provider.h"

#include <stdlib.h>
#include <math.h>
#include <algorithm>

#include "file_storage_provider.h"

namespace niffler {

    memory_storage_provider::memory_storage_provider()
    {
        init(DEFAULT_NUM_PAGES);
    }

    memory_storage_provider::memory_storage_provider(unique_ptr<file_storage_provider> file_storage)
        :
        file_storage_(std::move(file_storage)),
        has_file_storage_(true)
    {
        if (!file_storage_->ok())
            return;

        const auto file_size = file_storage_->size();
        if (file_size == -1)
            return;

        if (file_size == 0)
        {
            init(DEFAULT_NUM_PAGES);
        }
        else
        {
            const auto num_pages_required = static_cast<size_t>(ceil(static_cast<double>(file_size) / PAGE_SIZE));
            init(std::max(num_pages_required, DEFAULT_NUM_PAGES));
            file_storage_->load(buffer_, 0, static_cast<size_t>(file_size));
        }
    }

    memory_storage_provider::~memory_storage_provider()
    {
        if (buffer_ != nullptr)
            free(buffer_);
    }

    bool memory_storage_provider::ok() const
    {
        if (has_file_storage_ && (file_storage_ == nullptr || !file_storage_->ok()))
            return false;

        return buffer_ != nullptr;
    }

    bool memory_storage_provider::load(void *buffer, offset offset, size_t size)
    {
        resize_if_needed(offset + size);
        memcpy(buffer, buffer_ + offset, size);
        return true;
    }

    bool memory_storage_provider::store(const void *value, offset offset, size_t size)
    {
        resize_if_needed(offset + size);
        memcpy(buffer_ + offset, value, size);
        dirty_blocks_.emplace_back(offset, size);
        return true;
    }

    bool memory_storage_provider::sync()
    {
        for (const auto& block : dirty_blocks_)
        {
            const auto *value = buffer_ + block.offset;
            file_storage_->store(value, block.offset, block.size);
        }

        dirty_blocks_.clear();

        return file_storage_->sync();
    }

    void memory_storage_provider::init(size_t num_pages)
    {
        size_ = PAGE_SIZE * num_pages;
        buffer_ = static_cast<u8*>(malloc(size_));
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

}
