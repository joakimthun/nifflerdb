#pragma once

#include "define.h"
#include "files.h"
#include "storage_provider.h"

namespace niffler {

    class file_storage_provider : public storage_provider
    {
    public:
        file_storage_provider(const char *file_path, file_mode file_mode);
        ~file_storage_provider();

        bool ok() const override;
        bool load(void *buffer, offset offset, size_t size) override;
        bool store(const void *value, offset offset, size_t size) override;
        bool sync() override;
        offset alloc_block(size_t size);
        void free_block(offset offset, size_t size);
        long size();
    private:
        file_handle file_handle_;
    };
}
