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
            header_.first_free_list_page = 0;
            header_.num_free_list_pages = 0;

            auto& header_page = get_page(0);
            serialize_file_header(header_page.content, header_);
            header_page.dirty = true;
            sync();
        }
        else
        {
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

    page &pager::get_free_page()
    {
        // TODO: Grab from free list or allocate
        auto page_index = header_.num_pages++;
        assert(page_index < pages_.size());

        auto& page = pages_[page_index];
        page.index = page_index;

        return page;
    }

    void pager::free_page(page_index page_index)
    {

    }

    page &pager::get_page(page_index page_index)
    {
        if (page_index >= pages_.size())
        {
            pages_.resize(pages_.size() * 2);
        }

        auto& page = pages_[page_index];

        if(!page.loaded)
        {
            const auto page_offset = header_.page_size * page_index;
            fseek(file_handle_.file, page_offset, SEEK_SET);

            page.content = static_cast<u8*>(malloc(header_.page_size));
            page.size = header_.page_size;
            page.loaded = true;
            page.index = page_index;

            fread(page.content, header_.page_size, 1, file_handle_.file);
        }

        return page;
    }

    void pager::save_page(page_index page_index)
    {
        auto& page = pages_[page_index];
        const auto page_offset = header_.page_size * page.index;
        fseek(file_handle_.file, page_offset, SEEK_SET);

        fwrite(page.content, page.size, 1, file_handle_.file);
        page.dirty = false;
    }

    bool pager::sync()
    {
        for (const auto& page : pages_)
        {
            if (page.dirty)
            {
                save_page(page.index);
            }
        }

        return fsync(file_handle_.file) == 0;
    }
}
