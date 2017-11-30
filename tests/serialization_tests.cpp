#include <gtest\gtest.h>
#include <string.h>

#include "serialization.h"

using namespace niffler;

TEST(SERIALIZATION, FILE_HEADER)
{
    file_header h1 = { 0 };
    strcpy_s(h1.version, sizeof(h1.version), "NifflerDB 0.1");
    h1.page_size = 1;
    h1.num_pages = 2;
    h1.last_free_list_page = 3;
    h1.num_free_list_pages = 4;

    u8 buffer[1024] = { 0 };
    serialize_file_header(buffer, h1);

    file_header h2 = { 0 };
    deserialize_file_header(buffer, h2);

    ASSERT_STREQ(h2.version, "NifflerDB 0.1");
    EXPECT_EQ(h2.page_size, 1);
    EXPECT_EQ(h2.num_pages, 2);
    EXPECT_EQ(h2.last_free_list_page, 3);
    EXPECT_EQ(h2.num_free_list_pages, 4);
}

TEST(SERIALIZATION, PAGE_HEADER)
{
    page_header h1 = { 0 };
    h1.next_page = 1;
    h1.prev_page = 2;

    u8 buffer[1024] = { 0 };
    serialize_page_header(buffer, h1);

    page_header h2 = { 0 };
    deserialize_page_header(buffer, h2);

    EXPECT_EQ(h2.next_page, 1);
    EXPECT_EQ(h2.prev_page, 2);
}

TEST(SERIALIZATION, FREE_LIST_HEADER)
{
    free_list_header h1 = { 0 };
    h1.next_page = 1;
    h1.prev_page = 2;
    h1.num_pages = 3;

    u8 buffer[1024] = { 0 };
    serialize_free_list_header(buffer, h1);

    free_list_header h2 = { 0 };
    deserialize_free_list_header(buffer, h2);

    EXPECT_EQ(h2.next_page, 1);
    EXPECT_EQ(h2.prev_page, 2);
    EXPECT_EQ(h2.num_pages, 3);
}

TEST(SERIALIZATION, FREE_LIST_READ_WRITE_PAGE_INDEX)
{
    u8 buffer[PAGE_SIZE] = { 0 };

    write_free_list_page_index(buffer, 0, 1);
    write_free_list_page_index(buffer, 3, 2);
    write_free_list_page_index(buffer, 77, 3);
    write_free_list_page_index(buffer, 999, 4);
    write_free_list_page_index(buffer, 1020, 5);

    auto index_0 = read_free_list_page_index(buffer, 0);
    auto index_3 = read_free_list_page_index(buffer, 3);
    auto index_77 = read_free_list_page_index(buffer, 77);
    auto index_999 = read_free_list_page_index(buffer, 999);
    auto index_1020 = read_free_list_page_index(buffer, 1020);

    EXPECT_EQ(index_0, 1);
    EXPECT_EQ(index_3, 2);
    EXPECT_EQ(index_77, 3);
    EXPECT_EQ(index_999, 4);
    EXPECT_EQ(index_1020, 5);
}

TEST(SERIALIZATION, BP_TREE_HEADER)
{
    bp_tree_header h1 = { 0 };
    h1.order = 1;
    h1.key_size = 2;
    h1.num_internal_nodes = 3;
    h1.num_leaf_nodes = 4;
    h1.height = 5;
    h1.root_page = 6;
    h1.leaf_page = 7;

    u8 buffer[1024] = { 0 };
    serialize_bp_tree_header(buffer, h1);

    bp_tree_header h2 = { 0 };
    deserialize_bp_tree_header(buffer, h2);
    
    EXPECT_EQ(h2.order, 1);
    EXPECT_EQ(h2.key_size, 2);
    EXPECT_EQ(h2.num_internal_nodes, 3);
    EXPECT_EQ(h2.num_leaf_nodes, 4);
    EXPECT_EQ(h2.height, 5);
    EXPECT_EQ(h2.root_page, 6);
    EXPECT_EQ(h2.leaf_page, 7);
}

TEST(SERIALIZATION, BP_TREE_NODE)
{
    bp_tree_node<10> n1 = { 0 };
    n1.parent_page = 2;
    n1.next_page = 3;
    n1.prev_page = 4;
    n1.num_children = 10;
    
    for (auto i = 0u; i < 10; i++)
    {
        n1.children[i].key = 1;
        n1.children[i].page = 1;
    }

    u8 buffer[1024] = { 0 };
    serialize_bp_tree_node(buffer, n1);

    bp_tree_node<10> n2 = { 0 };
    deserialize_bp_tree_node(buffer, n2);

    EXPECT_EQ(n2.parent_page, 2);
    EXPECT_EQ(n2.next_page, 3);
    EXPECT_EQ(n2.prev_page, 4);
    EXPECT_EQ(n2.num_children, 10);

    for (auto i = 0u; i < 10; i++)
    {
        EXPECT_EQ(n2.children[i].key, 1);
        EXPECT_EQ(n2.children[i].page, 1);
    }
}

TEST(SERIALIZATION, BP_TREE_LEAF)
{
    bp_tree_leaf<10> l1 = { 0 };
    l1.parent_page = 2;
    l1.next_page = 3;
    l1.prev_page = 4;
    l1.num_children = 10;

    for (auto i = 0u; i < 10; i++)
    {
        l1.children[i].key = 1;
        l1.children[i].value = 1;
    }

    u8 buffer[1024] = { 0 };
    serialize_bp_tree_leaf(buffer, l1);

    bp_tree_leaf<10> l2 = { 0 };
    deserialize_bp_tree_leaf(buffer, l2);

    EXPECT_EQ(l2.parent_page, 2);
    EXPECT_EQ(l2.next_page, 3);
    EXPECT_EQ(l2.prev_page, 4);
    EXPECT_EQ(l2.num_children, 10);

    for (auto i = 0u; i < 10; i++)
    {
        EXPECT_EQ(l2.children[i].key, 1);
        EXPECT_EQ(l2.children[i].value, 1);
    }
}