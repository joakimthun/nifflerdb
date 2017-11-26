#include "pager.h"

#include <assert.h>

namespace niffler {

    pager::pager(const char *file_path, file_mode file_mode)
        :
        file_handle_(file_path, file_mode)
    {
        pages_.resize(DEFAULT_PAGER_SIZE);
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
        auto page_index = num_pages_++;
        assert(page_index < pages_.size());

        auto& page = pages_[page_index];
        page.index = page_index;

        return page;
    }

    page &pager::get_page(size_t page_index)
    {
        if (page_index >= pages_.size())
        {
            pages_.resize(pages_.size() * 2);
        }

        auto& page = pages_[page_index];

        if(!page.loaded)
        {
            const auto page_offset = base_offset + (page_size * page_index);
            fseek(file_handle_.file, page_offset, SEEK_SET);

            page.content = static_cast<u8*>(malloc(page_size));
            page.size = page_size;
            page.loaded = true;
            page.index = page_index;

            fread(page.content, page_size, 1, file_handle_.file);
        }

        return page;
    }

    void pager::save_page(size_t page_index)
    {
        auto& page = pages_[page_index];
        const auto page_offset = base_offset + (page_size * page.index);
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
