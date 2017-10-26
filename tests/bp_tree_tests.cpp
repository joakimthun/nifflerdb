#include <gtest\gtest.h>
#include <stdlib.h>

#include "storage_providers.h"
#include "bp_tree.h"
#include "test_helpers.h"

using namespace niffler;

TEST(BP_TREE, LEAF_BINARY_SEARCH)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;
    bp_tree_leaf<10> leaf;
    EXPECT_EQ(-1, t->binary_search_record(leaf, 0));
    
    leaf.children[0] = bp_tree_record{ 0, 0 };
    leaf.children[1] = bp_tree_record{ 1, 0 };
    leaf.children[2] = bp_tree_record{ 2, 0 };
    leaf.children[3] = bp_tree_record{ 3, 0 };
    leaf.children[4] = bp_tree_record{ 4, 0 };
    leaf.children[5] = bp_tree_record{ 5, 0 };
    leaf.children[6] = bp_tree_record{ 6, 0 };
    leaf.children[7] = bp_tree_record{ 7, 0 };
    leaf.children[8] = bp_tree_record{ 8, 0 };
    leaf.num_children = 9;

    EXPECT_EQ(0, t->binary_search_record(leaf, 0));
    EXPECT_EQ(1, t->binary_search_record(leaf, 1));
    EXPECT_EQ(2, t->binary_search_record(leaf, 2));
    EXPECT_EQ(3, t->binary_search_record(leaf, 3));
    EXPECT_EQ(4, t->binary_search_record(leaf, 4));
    EXPECT_EQ(5, t->binary_search_record(leaf, 5));
    EXPECT_EQ(6, t->binary_search_record(leaf, 6));
    EXPECT_EQ(7, t->binary_search_record(leaf, 7));
    EXPECT_EQ(8, t->binary_search_record(leaf, 8));

    EXPECT_EQ(-1, t->binary_search_record(leaf, -1));
    EXPECT_EQ(-1, t->binary_search_record(leaf, 11));
    EXPECT_EQ(-1, t->binary_search_record(leaf, 200));
}

TEST(BP_TREE, INSERT_RECORD_AT)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;
    bp_tree_leaf<10> leaf;

    t->insert_record_at(leaf, -1, 0, 0);
    EXPECT_EQ(1, leaf.num_children);
    EXPECT_EQ(-1, leaf.children[0].key);

    t->insert_record_at(leaf, -2, 0, 0);
    EXPECT_EQ(2, leaf.num_children);
    EXPECT_EQ(-2, leaf.children[0].key);

    t->insert_record_at(leaf, 1, 0, 2);
    EXPECT_EQ(3, leaf.num_children);
    EXPECT_EQ(1, leaf.children[2].key);

    t->insert_record_at(leaf, 0, 0, 2);
    EXPECT_EQ(4, leaf.num_children);
    EXPECT_EQ(-2, leaf.children[0].key);
    EXPECT_EQ(-1, leaf.children[1].key);
    EXPECT_EQ(0, leaf.children[2].key);
    EXPECT_EQ(1, leaf.children[3].key);

    t->insert_record_at(leaf, 2, 0, 4);
    EXPECT_EQ(5, leaf.num_children);
    EXPECT_EQ(-2, leaf.children[0].key);
    EXPECT_EQ(-1, leaf.children[1].key);
    EXPECT_EQ(0, leaf.children[2].key);
    EXPECT_EQ(1, leaf.children[3].key);
    EXPECT_EQ(2, leaf.children[4].key);

    t->insert_record_at(leaf, -3, 0, 0);
    EXPECT_EQ(6, leaf.num_children);
    EXPECT_EQ(-3, leaf.children[0].key);
    EXPECT_EQ(-2, leaf.children[1].key);
    EXPECT_EQ(-1, leaf.children[2].key);
    EXPECT_EQ(0, leaf.children[3].key);
    EXPECT_EQ(1, leaf.children[4].key);
    EXPECT_EQ(2, leaf.children[5].key);
}

TEST(BP_TREE, INSERT_KEY_AT)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;
    bp_tree_node<10> node;

    t->insert_key_at(node, -1, 1, 0);
    EXPECT_EQ(1, node.num_children);
    EXPECT_EQ(-1, node.children[0].key);
    EXPECT_EQ(1, node.children[0].offset);

    t->insert_key_at(node, 0, 1, 1);
    EXPECT_EQ(2, node.num_children);
    EXPECT_EQ(0, node.children[1].key);
    EXPECT_EQ(1, node.children[1].offset);

    t->insert_key_at(node, 2, 1, 2);
    EXPECT_EQ(3, node.num_children);
    EXPECT_EQ(2, node.children[2].key);
    EXPECT_EQ(1, node.children[2].offset);

    t->insert_key_at(node, 1, 1, 2);
    EXPECT_EQ(4, node.num_children);
    EXPECT_EQ(1, node.children[2].key);
    EXPECT_EQ(1, node.children[2].offset);
    EXPECT_EQ(2, node.children[3].key);
    EXPECT_EQ(1, node.children[3].offset);
}

TEST(BP_TREE, FIND_INSERT_INDEX_NODE)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;

    bp_tree_node<10> node;
    node.children[0] = bp_tree_node_child{ 0, 0 };
    node.children[1] = bp_tree_node_child{ 1, 0 };
    node.children[2] = bp_tree_node_child{ 2, 0 };
    node.children[3] = bp_tree_node_child{ 3, 0 };
    node.children[4] = bp_tree_node_child{ 4, 0 };
    node.children[5] = bp_tree_node_child{ 5, 0 };
    node.children[6] = bp_tree_node_child{ 6, 0 };
    node.children[7] = bp_tree_node_child{ 7, 0 };
    node.children[8] = bp_tree_node_child{ 9, 0 };
    node.num_children = 9;

    EXPECT_EQ(8, t->find_insert_index(node, 10));
    EXPECT_EQ(0, t->find_insert_index(node, -9));
    EXPECT_EQ(8, t->find_insert_index(node, 8));
}

TEST(BP_TREE, FIND_INSERT_INDEX_LEAF)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;

    bp_tree_leaf<10> leaf;
    leaf.children[0] = bp_tree_record{ 0, 0 };
    leaf.children[1] = bp_tree_record{ 1, 0 };
    leaf.children[2] = bp_tree_record{ 2, 0 };
    leaf.children[3] = bp_tree_record{ 3, 0 };
    leaf.children[4] = bp_tree_record{ 4, 0 };
    leaf.children[5] = bp_tree_record{ 5, 0 };
    leaf.children[6] = bp_tree_record{ 6, 0 };
    leaf.children[7] = bp_tree_record{ 7, 0 };
    leaf.children[8] = bp_tree_record{ 9, 0 };
    leaf.num_children = 9;

    EXPECT_EQ(9, t->find_insert_index(leaf, 10));
    EXPECT_EQ(0, t->find_insert_index(leaf, -9));
    EXPECT_EQ(8, t->find_insert_index(leaf, 8));
}

TEST(BP_TREE, INSERT_RECORD_NON_FULL)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;
    bp_tree_leaf<10> leaf;

    t->insert_record_non_full(leaf, -1, 0);
    EXPECT_EQ(1, leaf.num_children);
    EXPECT_EQ(-1, leaf.children[0].key);

    t->insert_record_non_full(leaf, -2, 0);
    EXPECT_EQ(2, leaf.num_children);
    EXPECT_EQ(-2, leaf.children[0].key);

    t->insert_record_non_full(leaf, 1, 0);
    EXPECT_EQ(3, leaf.num_children);
    EXPECT_EQ(1, leaf.children[2].key);

    t->insert_record_non_full(leaf, 0, 0);
    EXPECT_EQ(4, leaf.num_children);
    EXPECT_EQ(-2, leaf.children[0].key);
    EXPECT_EQ(-1, leaf.children[1].key);
    EXPECT_EQ(0, leaf.children[2].key);
    EXPECT_EQ(1, leaf.children[3].key);

    t->insert_record_non_full(leaf, 2, 0);
    EXPECT_EQ(5, leaf.num_children);
    EXPECT_EQ(-2, leaf.children[0].key);
    EXPECT_EQ(-1, leaf.children[1].key);
    EXPECT_EQ(0, leaf.children[2].key);
    EXPECT_EQ(1, leaf.children[3].key);
    EXPECT_EQ(2, leaf.children[4].key);

    t->insert_record_non_full(leaf, -3, 0);
    EXPECT_EQ(6, leaf.num_children);
    EXPECT_EQ(-3, leaf.children[0].key);
    EXPECT_EQ(-2, leaf.children[1].key);
    EXPECT_EQ(-1, leaf.children[2].key);
    EXPECT_EQ(0, leaf.children[3].key);
    EXPECT_EQ(1, leaf.children[4].key);
    EXPECT_EQ(2, leaf.children[5].key);
}

TEST(BP_TREE, TRANSFER_RECORDS)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;

    bp_tree_leaf<10> source;
    source.children[0] = bp_tree_record{ 1, 0 };
    source.children[1] = bp_tree_record{ 2, 0 };
    source.children[2] = bp_tree_record{ 3, 0 };
    source.children[3] = bp_tree_record{ 4, 0 };
    source.children[4] = bp_tree_record{ 5, 0 };
    source.children[5] = bp_tree_record{ 6, 0 };
    source.children[6] = bp_tree_record{ 7, 0 };
    source.children[7] = bp_tree_record{ 8, 0 };
    source.children[8] = bp_tree_record{ 9, 0 };
    source.children[9] = bp_tree_record{ 10, 0 };
    source.num_children = 10;

    bp_tree_leaf<10> target;
    t->transfer_records(source, target, 5);

    EXPECT_EQ(1, source.children[0].key);
    EXPECT_EQ(2, source.children[1].key);
    EXPECT_EQ(3, source.children[2].key);
    EXPECT_EQ(4, source.children[3].key);
    EXPECT_EQ(5, source.children[4].key);
    EXPECT_EQ(0, source.children[5].key);
    EXPECT_EQ(0, source.children[6].key);
    EXPECT_EQ(0, source.children[7].key);
    EXPECT_EQ(0, source.children[8].key);
    EXPECT_EQ(0, source.children[8].key);
    EXPECT_EQ(5, source.num_children);

    EXPECT_EQ(6, target.children[0].key);
    EXPECT_EQ(7, target.children[1].key);
    EXPECT_EQ(8, target.children[2].key);
    EXPECT_EQ(9, target.children[3].key);
    EXPECT_EQ(10, target.children[4].key);
    EXPECT_EQ(0, target.children[5].key);
    EXPECT_EQ(0, target.children[6].key);
    EXPECT_EQ(0, target.children[7].key);
    EXPECT_EQ(0, target.children[8].key);
    EXPECT_EQ(0, target.children[8].key);
    EXPECT_EQ(5, target.num_children);
}

TEST(BP_TREE, INSERT_NON_SPLIT)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;

    for (auto i = 0u; i < t->MAX_NUM_CHILDREN(); i++)
    {
        EXPECT_EQ(true, t->insert(i, i));
        EXPECT_EQ(false, t->insert(i, i));
    }
}

TEST(BP_TREE, INSERT_1000_KEYS)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024 * 20)).value;
    const auto num_keys = 1000;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->insert(i, i));
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
    }

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(false, t->insert(i, i));
    }
}

TEST(BP_TREE, INSERT_5000_RANDOM_KEYS)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024 * 100)).value;
    const auto num_keys = 5000;
    srand(99);

    for (auto i = 0; i < num_keys; i++)
    {
        auto key = rand();
        t->insert(key, i);
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << key << ", index: " << i;
        EXPECT_EQ(false, t->insert(key, i));
    }
}

TEST(BP_TREE, REMOVE_RECORD_AT)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;
    bp_tree_leaf<10> leaf;
    leaf.children[0] = bp_tree_record{ 0, 0 };
    leaf.children[1] = bp_tree_record{ 1, 0 };
    leaf.children[2] = bp_tree_record{ 2, 0 };
    leaf.children[3] = bp_tree_record{ 3, 0 };
    leaf.children[4] = bp_tree_record{ 4, 0 };
    leaf.children[5] = bp_tree_record{ 5, 0 };
    leaf.children[6] = bp_tree_record{ 6, 0 };
    leaf.children[7] = bp_tree_record{ 7, 0 };
    leaf.children[8] = bp_tree_record{ 8, 0 };
    leaf.children[9] = bp_tree_record{ 9, 0 };
    leaf.num_children = 10;

    t->remove_record_at(leaf, 0);
    EXPECT_EQ(9, leaf.num_children);
    EXPECT_EQ(1, leaf.children[0].key);
    EXPECT_EQ(2, leaf.children[1].key);
    EXPECT_EQ(9, leaf.children[8].key);

    t->remove_record_at(leaf, 3);
    EXPECT_EQ(8, leaf.num_children);
    EXPECT_EQ(1, leaf.children[0].key);
    EXPECT_EQ(5, leaf.children[3].key);
    EXPECT_EQ(9, leaf.children[7].key);

    t->remove_record_at(leaf, 7);
    EXPECT_EQ(7, leaf.num_children);
    EXPECT_EQ(1, leaf.children[0].key);
    EXPECT_EQ(5, leaf.children[3].key);
    EXPECT_EQ(8, leaf.children[6].key);
}

TEST(BP_TREE, REMOVE_RECORD)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;
    bp_tree_leaf<10> leaf;
    leaf.children[0] = bp_tree_record{ 0, 0 };
    leaf.children[1] = bp_tree_record{ 1, 0 };
    leaf.children[2] = bp_tree_record{ 2, 0 };
    leaf.children[3] = bp_tree_record{ 3, 0 };
    leaf.children[4] = bp_tree_record{ 4, 0 };
    leaf.children[5] = bp_tree_record{ 5, 0 };
    leaf.children[6] = bp_tree_record{ 6, 0 };
    leaf.children[7] = bp_tree_record{ 7, 0 };
    leaf.children[8] = bp_tree_record{ 8, 0 };
    leaf.children[9] = bp_tree_record{ 9, 0 };
    leaf.num_children = 10;

    t->remove_record(leaf, 0);
    EXPECT_EQ(9, leaf.num_children);
    EXPECT_EQ(1, leaf.children[0].key);
    EXPECT_EQ(2, leaf.children[1].key);
    EXPECT_EQ(9, leaf.children[8].key);

    t->remove_record(leaf, 4);
    EXPECT_EQ(8, leaf.num_children);
    EXPECT_EQ(1, leaf.children[0].key);
    EXPECT_EQ(5, leaf.children[3].key);
    EXPECT_EQ(9, leaf.children[7].key);

    t->remove_record(leaf, 9);
    EXPECT_EQ(7, leaf.num_children);
    EXPECT_EQ(1, leaf.children[0].key);
    EXPECT_EQ(5, leaf.children[3].key);
    EXPECT_EQ(8, leaf.children[6].key);
}

TEST(BP_TREE, REMOVE_NO_MERGE_BORROW)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024 * 10)).value;
    const auto num_keys = 250;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->insert(i, i));
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
    }

    // All keys removed are in non minimal leafs
    EXPECT_EQ(true, t->remove(197));
    auto result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 197;

    EXPECT_EQ(true, t->remove(203));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 203;

    EXPECT_EQ(true, t->remove(217));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 217;

    EXPECT_EQ(true, t->remove(230));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 230;

    EXPECT_EQ(true, t->remove(242));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 242;

    EXPECT_EQ(true, t->remove(248));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 248;
}

TEST(BP_TREE, REMOVE_NO_MERGE)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024 * 10)).value;
    const auto num_keys = 250;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->insert(i, i));
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
    }

    // No borrow
    EXPECT_EQ(true, t->remove(239));
    auto result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 239;

    // Borrow left
    EXPECT_EQ(true, t->remove(238));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 238;

    // Left is @ MIN_NUM_CHILDREN -> borrow right
    EXPECT_EQ(true, t->remove(237));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << 237;
}

TEST(BP_TREE, MERGE_LEAFS)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024)).value;
    bp_tree_leaf<10> first;
    bp_tree_leaf<10> second;

    first.children[0] = bp_tree_record{ 0, 0 };
    first.children[1] = bp_tree_record{ 1, 0 };
    first.children[2] = bp_tree_record{ 2, 0 };
    first.children[3] = bp_tree_record{ 3, 0 };
    first.children[4] = bp_tree_record{ 4, 0 };
    second.children[0] = bp_tree_record{ 5, 0 };
    second.children[1] = bp_tree_record{ 6, 0 };
    second.children[2] = bp_tree_record{ 7, 0 };
    second.children[3] = bp_tree_record{ 8, 0 };
    second.children[4] = bp_tree_record{ 9, 0 };
    first.num_children = 5;
    second.num_children = 5;

    t->merge_leafs(first, second);
    EXPECT_EQ(10, first.num_children);
    EXPECT_EQ(0, second.num_children);

    EXPECT_EQ(0, first.children[0].key);
    EXPECT_EQ(1, first.children[1].key);
    EXPECT_EQ(2, first.children[2].key);
    EXPECT_EQ(3, first.children[3].key);
    EXPECT_EQ(4, first.children[4].key);
    EXPECT_EQ(5, first.children[5].key);
    EXPECT_EQ(6, first.children[6].key);
    EXPECT_EQ(7, first.children[7].key);
    EXPECT_EQ(8, first.children[8].key);
    EXPECT_EQ(9, first.children[9].key);
}

TEST(BP_TREE, ADD_REMOVE_100_X2)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024 * 10)).value;
    const auto num_keys = 100;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->insert(i, i));
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
    }

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->remove(i)) << "removed key: " << i;
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << i;
    }

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->insert(i, i));
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
    }

    for (auto i = num_keys - 1; i >= 0; i--)
    {
        EXPECT_EQ(true, t->remove(i)) << "removed key: " << i;
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << i;
    }
}

TEST(BP_TREE, ADD_REMOVE_5000)
{
    auto t = bp_tree<10>::create(std::make_unique<mem_storage_provider>(1024 * 100)).value;
    const auto num_keys = 5000;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->insert(i, i));
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
    }

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->remove(i)) << "removed key: " << i;
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "removed key: " << i;
    }
}