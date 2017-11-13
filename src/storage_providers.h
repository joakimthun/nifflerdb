#pragma once

#include <vector>

#include "define.h"
#include "files.h"

namespace niffler {

    using std::size_t;

    struct storage_provider
    {
        virtual ~storage_provider() {};
        virtual bool ok() const = 0;
        virtual bool load(void *buffer, offset offset, size_t size) = 0;
        virtual bool store(void *value, offset offset, size_t size) = 0;
        virtual bool sync() = 0;
    };

    class memory_storage_provider : public storage_provider
    {
    public:
        memory_storage_provider();
        ~memory_storage_provider();
        bool ok() const override;
        bool load(void *buffer, offset offset, size_t size) override;
        bool store(void *value, offset offset, size_t size) override;
        bool sync() override;
    private:
        void resize_if_needed(size_t min_size);

        size_t size_ = 0;
        u8 *buffer_ = nullptr;
    };

    class file_storage_provider : public storage_provider
    {
    public:
        file_storage_provider(const char *file_path, file_mode file_mode);
        ~file_storage_provider();
        bool ok() const override;
        bool load(void *buffer, offset offset, size_t size) override;
        bool store(void *value, offset offset, size_t size) override;
        bool sync() override;
        offset alloc_block(size_t size);
        void free_block(offset offset, size_t size);
    private:
        file_handle file_handle_;
    };
}
