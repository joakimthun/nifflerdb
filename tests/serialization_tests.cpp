#include <gtest\gtest.h>

#include "serialization.h"

using namespace niffler;

TEST(SERIALIZATION, HEADER)
{
    bp_tree_header h1 = { 0 };
    h1.order = 1;
    h1.value_size = 2;
    h1.key_size = 3;
    h1.num_internal_nodes = 4;
    h1.num_leaf_nodes = 5;
    h1.height = 6;
    h1.root_page = 7;
    h1.leaf_page = 8;

    u8 buffer[1024] = { 0 };
    serialize_bp_tree_header(buffer, h1);

    bp_tree_header h2 = { 0 };
    deserialize_bp_tree_header(buffer, h2);
    
    EXPECT_EQ(h2.order, 1);
    EXPECT_EQ(h2.value_size, 2);
    EXPECT_EQ(h2.key_size, 3);
    EXPECT_EQ(h2.num_internal_nodes, 4);
    EXPECT_EQ(h2.num_leaf_nodes, 5);
    EXPECT_EQ(h2.height, 6);
    EXPECT_EQ(h2.root_page, 7);
    EXPECT_EQ(h2.leaf_page, 8);
}

TEST(SERIALIZATION, NODE)
{
    bp_tree_node<10> n1 = { 0 };
    n1.page = 1;
    n1.parent_page = 2;
    n1.next_page = 3;
    n1.prev_page = 4;
    n1.num_children = 5;
    
    for (auto i = 0u; i < 10; i++)
    {
        n1.children[i].key = 1;
        n1.children[i].page = 1;
    }

    u8 buffer[1024] = { 0 };
    serialize_bp_tree_node(buffer, n1);

    bp_tree_node<10> n2 = { 0 };
    deserialize_bp_tree_node(buffer, n2);

    EXPECT_EQ(n1.page, 1);
    EXPECT_EQ(n1.parent_page, 2);
    EXPECT_EQ(n1.next_page, 3);
    EXPECT_EQ(n1.prev_page, 4);
    EXPECT_EQ(n1.num_children, 5);

    for (auto i = 0u; i < 10; i++)
    {
        EXPECT_EQ(n1.children[i].key, 1);
        EXPECT_EQ(n1.children[i].page, 1);
    }
}

TEST(SERIALIZATION, LEAF)
{
    bp_tree_leaf<10> l1 = { 0 };
    l1.page = 1;
    l1.parent_page = 2;
    l1.next_page = 3;
    l1.prev_page = 4;
    l1.num_children = 5;

    for (auto i = 0u; i < 10; i++)
    {
        l1.children[i].key = 1;
        l1.children[i].value = 1;
    }

    u8 buffer[1024] = { 0 };
    serialize_bp_tree_leaf(buffer, l1);

    bp_tree_leaf<10> l2 = { 0 };
    deserialize_bp_tree_leaf(buffer, l2);

    EXPECT_EQ(l1.page, 1);
    EXPECT_EQ(l1.parent_page, 2);
    EXPECT_EQ(l1.next_page, 3);
    EXPECT_EQ(l1.prev_page, 4);
    EXPECT_EQ(l1.num_children, 5);

    for (auto i = 0u; i < 10; i++)
    {
        EXPECT_EQ(l1.children[i].key, 1);
        EXPECT_EQ(l1.children[i].value, 1);
    }
}