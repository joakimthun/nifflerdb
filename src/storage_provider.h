//#pragma once
//
//#include "define.h"
//
//namespace niffler {
//
//    using std::size_t;
//
//    constexpr size_t DEFAULT_NUM_PAGES = 10;
//
//    struct block
//    {
//        block(offset offset, size_t size) : offset(offset), size(size) {}
//        const offset offset;
//        const size_t size;
//    };
//
//    struct storage_provider
//    {
//        virtual ~storage_provider() {};
//        virtual bool ok() const = 0;
//        virtual bool load(void *buffer, offset offset, size_t size) = 0;
//        virtual bool store(const void *value, offset offset, size_t size) = 0;
//        virtual bool sync() = 0;
//    };
//}
