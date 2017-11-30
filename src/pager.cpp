#include "pager.h"

#include <assert.h>
#include <string.h>

#include "serialization.h"

namespace niffler {

    pager::pager(const char *file_path, bool truncate_existing_file)
        :
        file_handle_(file_path, truncate_existing_file ? file_mode::write_update : file_mode::read_update)
    {
        pages_.resize(DEFAULT_PAGER_SIZE);

        if (truncate_existing_file)
        {
            strcpy_s(header_.version, sizeof(header_.version), "NifflerDB 0.1");
            header_.page_size = PAGE_SIZE;
            header_.num_pages = 1;
            header_.last_free_list_page = 0;
            header_.num_free_list_pages = 0;
            save_header();
        }
        else
        {
            // Don't use get_page here beacuse it relies on header.page_size which has not been loaded yet
            u8 buffer[file_header::DISK_SIZE()];
            fseek(file_handle_.file, 0, SEEK_SET);
            fread(buffer, file_header::DISK_SIZE(), 1, file_handle_.file);
            deserialize_file_header(buffer, header_);
        }
    }

    pager::~pager()
    {
        for (auto& p : pages_)
        {
            if (p.content != nullptr)
            {
                free(p.content);
                p.content = nullptr;
            }
        }
    }

    const file_header &pager::header() const
    {
        return header_;
    }

    page &pager::get_free_page()
    {
        // No free pages, allocate a new one
        if (header_.last_free_list_page == 0)
        {
            return alloc_page();
        }

        auto& last_free_list_page = get_page(header_.last_free_list_page);
        free_list_header free_list_header;
        deserialize_free_list_header(last_free_list_page.content, free_list_header);

        if (free_list_header.num_pages == 0)
        {
            // TODO: Implement actual truncation of the file to keep the file from growing and growing
            return alloc_page();
        }

        assert(free_list_header.num_pages > 0);
        const auto next_free_page_index = read_free_list_page_index(last_free_list_page.content, free_list_header.num_pages - 1);

        // Update num pages and save changes
        free_list_header.num_pages--;
        serialize_free_list_header(last_free_list_page.content, free_list_header);
        save_page(last_free_list_page.index);
        sync(false);

        return get_page(next_free_page_index);
    }

    void pager::free_page(page_index page_index)
    {
        // Allocate a new page to keep the list of free pages
        if (header_.last_free_list_page == 0)
        {
            auto &free_list_page = alloc_page();
            free_list_header first_free_list_header = { 0 };
            first_free_list_header.num_pages = 1;
            write_free_list_page_index(free_list_page.content, 0, page_index);
            serialize_free_list_header(free_list_page.content, first_free_list_header);
            save_page(free_list_page.index);

            // Update the file header
            header_.last_free_list_page = free_list_page.index;
            header_.num_free_list_pages++;
            save_header(false);

            sync(false);
            return;
        }

        auto& last_free_list_page = get_page(header_.last_free_list_page);
        free_list_header current_free_list_header;
        deserialize_free_list_header(last_free_list_page.content, current_free_list_header);

        // Space left on last_free_list_page?
        if (current_free_list_header.num_pages < free_list_header::MAX_NUM_PAGES())
        {
            write_free_list_page_index(last_free_list_page.content, current_free_list_header.num_pages, page_index);
            current_free_list_header.num_pages++;
            serialize_free_list_header(last_free_list_page.content, current_free_list_header);
            save_page(last_free_list_page.index);
            sync(false);
            return;
        }

        // last_free_list_page is full, allocate a new page
        auto &new_free_list_page = alloc_page();
        free_list_header new_free_list_header = { 0 };
        new_free_list_header.num_pages = 1;
        new_free_list_header.prev_page = header_.last_free_list_page;
        serialize_free_list_header(new_free_list_page.content, new_free_list_header);
        write_free_list_page_index(new_free_list_page.content, 0, page_index);

        save_page(new_free_list_page.index);

        // Update the file header
        header_.last_free_list_page = new_free_list_page.index;
        header_.num_free_list_pages++;
        save_header(false);

        sync(false);
    }

    page &pager::get_page(page_index page_index)
    {
        auto& page = get_page_internal(page_index);

        if(!page.loaded)
        {
            const auto page_offset = header_.page_size * page_index;
            fseek(file_handle_.file, page_offset, SEEK_SET);

            page.content = static_cast<u8*>(malloc(header_.page_size));
            page.size = header_.page_size;
            page.loaded = true;
            page.index = page_index;
            page.dirty = false;

            fread(page.content, header_.page_size, 1, file_handle_.file);
        }

        return page;
    }

    void pager::save_page(page_index page_index)
    {
        assert(page_index < pages_.size());

        auto& page = pages_[page_index];
        const auto page_offset = header_.page_size * page.index;
        fseek(file_handle_.file, page_offset, SEEK_SET);

        fwrite(page.content, page.size, 1, file_handle_.file);
        page.dirty = false;
    }

    bool pager::sync()
    {
        return sync(true);
    }

    page &pager::alloc_page()
    {
        auto new_page_index = header_.num_pages++;
        auto& page = get_page_internal(new_page_index);
        save_header();

        page.content = static_cast<u8*>(malloc(header_.page_size));
        page.size = header_.page_size;
        page.loaded = true;
        page.index = new_page_index;
        page.dirty = false;

        return page;
    }

    page &pager::get_page_internal(page_index page_index)
    {
        if (page_index >= pages_.size())
        {
            pages_.resize(pages_.size() * 2);
        }

        return pages_[page_index];
    }

    bool pager::sync(bool save_pages)
    {
        if (save_pages)
        {
            for (const auto& page : pages_)
            {
                if (page.dirty)
                {
                    save_page(page.index);
                }
            }
        }

        return fsync(file_handle_.file) == 0;
    }

    void pager::save_header(bool fsync)
    {
        auto& header_page = get_page(0);
        serialize_file_header(header_page.content, header_);
        save_page(header_page.index);

        if(fsync)
            sync(false);
    }
}
