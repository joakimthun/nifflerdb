//#pragma once
//
//#include <memory>
//#include <vector>
//
//#include "storage_provider.h"
//
//namespace niffler {
//
//    using std::unique_ptr;
//    using std::vector;
//
//    class file_storage_provider;
//
//    class memory_storage_provider : public storage_provider
//    {
//    public:
//        memory_storage_provider();
//        memory_storage_provider(unique_ptr<file_storage_provider> file_storage);
//        ~memory_storage_provider();
//
//        bool ok() const override;
//        bool load(void *buffer, offset offset, size_t size) override;
//        bool store(const void *value, offset offset, size_t size) override;
//        bool sync() override;
//    private:
//        void init(size_t num_pages);
//        void resize_if_needed(size_t min_size);
//
//        size_t size_ = 0;
//        u8 *buffer_ = nullptr;
//        bool has_file_storage_ = false;
//        unique_ptr<file_storage_provider> file_storage_;
//        vector<block> dirty_blocks_;
//    };
//
//}
