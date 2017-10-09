#include "files.h"

#include <string.h>

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
            return "r+b";
        case file_mode::write_update:
            return "w+b";
        case file_mode::append_update:
            return "a+b";
        default:
            return "";
        }
    }

    file_handle::file_handle(const char *path, file_mode mode)
    {
        strcpy_s(file_path, FILE_PATH_BUFFER_SIZE, file_path);
        fopen_s(&file, file_path, get_file_mode(mode));
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

}
