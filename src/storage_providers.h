#pragma once

#include <cstddef>

namespace niffler {

    using std::size_t;
    using offset = std::size_t;

    struct storage_provider
    {
        virtual ~storage_provider() {}
        //virtual offset alloc(size_t size) = 0;
        //virtual offset dealloc(size_t size) = 0;
        virtual bool load(void *buffer, offset offset, size_t size) const = 0;
        virtual bool store(void *value, offset offset, size_t size) const = 0;
    };

    class mem_storage_provider : public storage_provider
    {
    public:
        mem_storage_provider(size_t initial_size);
        ~mem_storage_provider() override;
        bool load(void *buffer, offset offset, size_t size) const override;
        bool store(void *value, offset offset, size_t size) const override;
    private:
        void *buffer_ = nullptr;
        size_t size_ = 0;
    };

}
