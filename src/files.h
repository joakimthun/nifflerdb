#pragma once

#include <stdio.h>

namespace niffler {

    enum class file_mode {
        read,
        write,
        append,
        read_update,
        write_update,
        append_update
    };

    constexpr size_t FILE_PATH_BUFFER_SIZE = 1024;

    struct file_handle {
        FILE *file = nullptr;
        char file_path[FILE_PATH_BUFFER_SIZE] = { 0 };

        file_handle(const char *path, file_mode mode);
        ~file_handle();
        bool ok() const;
    };

    int fsync(FILE *file);
}