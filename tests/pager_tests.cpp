#include <gtest\gtest.h>
#include <vector>

#include "pager.h"

using namespace niffler;

TEST(PAGER, CREATE_PAGER)
{
    pager pager("files/test_pager.ndb", true);
    const auto &h = pager.header();

    ASSERT_STREQ(h.version, "NifflerDB 0.1");
    EXPECT_EQ(h.page_size, PAGE_SIZE);
    EXPECT_EQ(h.num_pages, 1);
    EXPECT_EQ(h.last_free_list_page, 0);
    EXPECT_EQ(h.num_free_list_pages, 0);
}

TEST(PAGER, FREE_PAGE_LIST)
{
    pager pager("files/test_pager.ndb", true);
    const auto &h = pager.header();

    EXPECT_EQ(h.num_free_list_pages, 0);
    EXPECT_EQ(h.num_pages, 1);

    constexpr auto num_pages_to_create = free_list_header::MAX_NUM_PAGES() * 3;
    std::vector<page_index> page_indices;

    for (auto i = 0; i < num_pages_to_create; i++)
    {
        auto &p = pager.get_free_page();
        page_indices.push_back(p.index);
    }

    EXPECT_EQ(h.num_pages, num_pages_to_create + 1);

    EXPECT_EQ(h.num_free_list_pages, 0);
    auto last = page_indices.back();
    page_indices.pop_back();
    pager.free_page(last);
    EXPECT_EQ(h.num_free_list_pages, 1);
    EXPECT_EQ(h.last_free_list_page, num_pages_to_create + 1);

    for (auto i = 0u; i < free_list_header::MAX_NUM_PAGES() - 1; i++)
    {
        last = page_indices.back();
        page_indices.pop_back();
        pager.free_page(last);
    }

    EXPECT_EQ(h.num_free_list_pages, 1);

    for (auto i = 0u; i < free_list_header::MAX_NUM_PAGES(); i++)
    {
        last = page_indices.back();
        page_indices.pop_back();
        pager.free_page(last);
    }
    
    EXPECT_EQ(h.num_free_list_pages, 2);

    for (auto i = 0u; i < free_list_header::MAX_NUM_PAGES(); i++)
    {
        last = page_indices.back();
        page_indices.pop_back();
        pager.free_page(last);
    }

    EXPECT_EQ(h.num_free_list_pages, 3);
    EXPECT_EQ(h.num_pages, h.num_free_list_pages + 1 + num_pages_to_create);

    EXPECT_EQ(page_indices.size(), 0);
    for (auto i = 0; i < num_pages_to_create; i++)
    {
        auto &p = pager.get_free_page();
        page_indices.push_back(p.index);
    }

    // This should fail once actual file truncation is implemented since we have a bunch of pages that can be used in the free pages list
    EXPECT_EQ(h.num_free_list_pages, 3);
    EXPECT_EQ(h.num_pages, 1 + (num_pages_to_create * 2 - free_list_header::MAX_NUM_PAGES()) + h.num_free_list_pages);

    last = page_indices.back();
    page_indices.pop_back();
    pager.free_page(last);

    EXPECT_EQ(h.num_free_list_pages, 3);
    EXPECT_EQ(h.num_pages, 1 + (num_pages_to_create * 2 - free_list_header::MAX_NUM_PAGES()) + h.num_free_list_pages);
}