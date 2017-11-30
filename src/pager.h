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
        page_index last_free_list_page;
        u32 num_free_list_pages;

        static inline constexpr u32 DISK_SIZE()
        { 
            return sizeof(version) + sizeof(page_size) + sizeof(num_pages)
                + sizeof(last_free_list_page) + sizeof(num_free_list_pages);
        }
    };

    struct page_header
    {
        page_index next_page;
        page_index prev_page;

        static inline constexpr u32 DISK_SIZE() { return sizeof(next_page) + sizeof(prev_page); }
    };

    struct free_list_header
    {
        page_index next_page;
        page_index prev_page;
        u32 num_pages;

        static inline constexpr u32 DISK_SIZE() { return sizeof(next_page) + sizeof(prev_page) + sizeof(num_pages); }
        static inline constexpr u32 PAGES_SECTION_SIZE() { return PAGE_SIZE - free_list_header::DISK_SIZE(); }
        static inline constexpr u32 MAX_NUM_PAGES() { return free_list_header::PAGES_SECTION_SIZE() / sizeof(u32); }
        static inline constexpr u32 MAX_PAGES_SECTION_OFFSET() { return PAGE_SIZE - sizeof(u32); }
    };

    class pager
    {
    public:
        pager(const char *file_path, bool truncate_existing_file);
        ~pager();

        const file_header &header() const;
        page &get_free_page();
        void free_page(page_index page_index);
        page &get_page(page_index page_index);
        void save_page(page_index page_index);
        bool sync();

    private:
        page &alloc_page();
        page &get_page_internal(page_index page_index);
        bool sync(bool save_pages);
        void save_header(bool fsync = true);

        file_header header_;
        vector<page> pages_;
        file_handle file_handle_;
    };
}
