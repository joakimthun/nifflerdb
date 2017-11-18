#include <gtest\gtest.h>
#include <stdlib.h>

#include "bp_tree.h"
#include "test_helpers.h"

using namespace niffler;

TEST(BP_TREE_DEFAULT, ADD_REMOVE_1000)
{
    auto t = bp_tree<DEFAULT_TREE_ORDER>::create(create_storage_provider("files/test_default.ndb")).value;
    const auto num_keys = 1000;

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

TEST(BP_TREE_DEFAULT, ADD_REMOVE_1000_RANDOM_KEYS)
{
    auto t = bp_tree<DEFAULT_TREE_ORDER>::create(create_storage_provider("files/test_default.ndb")).value;
    random_keys_test(499, 1000);
}

TEST(BP_TREE_DEFAULT, ADD_REMOVE_5000_RANDOM_KEYS)
{
    auto t = bp_tree<DEFAULT_TREE_ORDER>::create(create_storage_provider("files/test_default.ndb")).value;
    random_keys_test(29, 5000);
}

TEST(BP_TREE_DEFAULT, LOAD_1000)
{
    const auto num_keys = 1000;

    {
        auto t = bp_tree<DEFAULT_TREE_ORDER>::create(create_storage_provider("files/test_default.ndb")).value;

        for (auto i = 0; i < num_keys; i++)
        {
            EXPECT_EQ(true, t->insert(i, i));
            auto result = validate_bp_tree(t);
            EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
        }
    }
    
    auto loaded_t = bp_tree<DEFAULT_TREE_ORDER>::load(create_storage_provider("files/test_default.ndb", file_mode::read_update)).value;
    
    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, loaded_t->exists(i));
    }
}

TEST(BP_TREE_DEFAULT, LOAD_5000)
{
    const auto num_keys = 5000;

    {
        auto t = bp_tree<DEFAULT_TREE_ORDER>::create(create_storage_provider("files/test_default.ndb")).value;

        for (auto i = 0; i < num_keys; i++)
        {
            EXPECT_EQ(true, t->insert(i, i));
            auto result = validate_bp_tree(t);
            EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
        }
    }

    auto loaded_t = bp_tree<DEFAULT_TREE_ORDER>::load(create_storage_provider("files/test_default.ndb", file_mode::read_update)).value;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, loaded_t->exists(i));
    }
}