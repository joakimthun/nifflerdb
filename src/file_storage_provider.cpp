//#include "file_storage_provider.h"
//
//namespace niffler {
//
//    file_storage_provider::file_storage_provider(const char *file_path, file_mode file_mode)
//        :
//        file_handle_(file_path, file_mode)
//    {
//    }
//
//    file_storage_provider::~file_storage_provider()
//    {
//    }
//
//    bool file_storage_provider::ok() const
//    {
//        return file_handle_.ok();
//    }
//
//    bool file_storage_provider::load(void *buffer, offset offset, size_t size)
//    {
//        fseek(file_handle_.file, offset, SEEK_SET);
//        return fread(buffer, size, 1, file_handle_.file) == 1;
//    }
//
//    bool file_storage_provider::store(const void *value, offset offset, size_t size)
//    {
//        fseek(file_handle_.file, offset, SEEK_SET);
//        return fwrite(value, size, 1, file_handle_.file) == 1;
//    }
//
//    bool file_storage_provider::sync()
//    {
//        return fsync(file_handle_.file) == 0;
//    }
//
//    offset file_storage_provider::alloc_block(size_t size)
//    {
//        return 0;
//    }
//
//    void file_storage_provider::free_block(offset offset, size_t size)
//    {
//    }
//
//    long file_storage_provider::size()
//    {
//        fseek(file_handle_.file, 0, SEEK_END);
//        return ftell(file_handle_.file);
//    }
//}
