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
        u16 size = 0;
        bool dirty = false;
        bool loaded = false;
        size_t index = 0;
    };

    struct file_header
    {
        char version[24];
        u16 page_size;
        u32 num_pages;
        page_index first_free_list_page;
        u32 num_free_list_pages;

        static inline constexpr u32 DISK_SIZE()
        { 
            return sizeof(version) + sizeof(page_size) + sizeof(num_pages)
                + sizeof(first_free_list_page) + sizeof(num_free_list_pages);
        }
    };

    struct page_header
    {
        page_index next_page;
        page_index prev_page;

        static inline constexpr u32 DISK_SIZE() { return sizeof(next_page) + sizeof(prev_page); }
    };

    class pager
    {
    public:
        pager(const char *file_path, bool truncate_existing_file);
        ~pager();

        page &get_free_page();
        void free_page(page_index page_index);
        page &get_page(page_index page_index);
        void save_page(page_index page_index);
        bool sync();

    private:


        file_header header_;
        vector<page> pages_;
        file_handle file_handle_;
    };
}
