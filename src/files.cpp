#include "files.h"

#include <string.h>
#include <assert.h>

#if (defined _WIN32 || defined __WIN32__)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>

#endif

namespace niffler {

    const char *get_file_mode(file_mode mode)
    {
        switch (mode)
        {
        case file_mode::read:
            return "rb";
        case file_mode::write:
            return "wb";
        case file_mode::append:
            return "ab";
        case file_mode::read_update:
            return "rb+";
        case file_mode::write_update:
            return "wb+";
        case file_mode::append_update:
            return "ab+";
        default:
            return "";
        }
    }

    file_handle::file_handle(const char *path, file_mode mode)
    {
        strcpy_s(file_path, FILE_PATH_BUFFER_SIZE, path);
        fopen_s(&file, file_path, get_file_mode(mode));
        assert(file != nullptr);
    }

    file_handle::~file_handle()
    {
        if (file != nullptr)
            fclose(file);
    }

    bool file_handle::ok() const
    {
        return file != nullptr;
    }

#if (defined _WIN32 || defined __WIN32__)

    int fsync(FILE *file)
    {
        auto handle = (HANDLE)_get_osfhandle(_fileno(file));
        if (handle == INVALID_HANDLE_VALUE)
            return -1;

        if (!FlushFileBuffers(handle))
            return -1;

        return 0;
    }

    int ftruncate(FILE *file, size_t length)
    {
        assert(length >= 0);
        return _chsize(_fileno(file), static_cast<long>(length));
    }

#else
# error "niffler::fsync and niffler::ftruncate not implemented"
#endif

}
