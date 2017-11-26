#pragma once

#include <vector>
#include <stdlib.h>

#include "define.h"
#include "files.h"

namespace niffler {

    using std::vector;

    struct page
    {
        u8 *content = nullptr;
        size_t size = 0;
        bool dirty = false;
        bool loaded = false;
        size_t index = 0;
    };

    class pager
    {
    public:
        pager(const char *file_path, file_mode file_mode);
        ~pager();

        page &get_free_page();
        page &get_page(size_t page_index);
        void save_page(size_t page_index);
        bool sync();

    private:
        const size_t base_offset = 0;
        const size_t page_size = PAGE_SIZE;
        size_t num_pages_ = 1;
        vector<page> pages_;
        file_handle file_handle_;
    };
}
