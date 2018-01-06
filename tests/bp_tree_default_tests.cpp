#include <gtest\gtest.h>
#include <stdlib.h>

#include "bp_tree.h"
#include "test_helpers.h"

using namespace niffler;

char *test_value = "test value";
const auto test_value_size = strlen(test_value);

TEST(BP_TREE_DEFAULT, ADD_REMOVE_1000)
{
    auto p = create_pager("files/test_default.ndb");
    auto t = bp_tree<DEFAULT_TREE_ORDER>::create(p.get()).value;
    const auto num_keys = 1000;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, t->insert(i, test_value, test_value_size));
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
    random_keys_test(499, 1000);
}

TEST(BP_TREE_DEFAULT, ADD_REMOVE_5000_RANDOM_KEYS)
{
    random_keys_test(29, 5000);
}

TEST(BP_TREE_DEFAULT, LOAD_1000)
{
    const auto num_keys = 1000;

    {
        auto p = create_pager("files/test_default.ndb");
        auto t = bp_tree<DEFAULT_TREE_ORDER>::create(p.get()).value;

        for (auto i = 0; i < num_keys; i++)
        {
            EXPECT_EQ(true, t->insert(i, test_value, test_value_size));
            auto result = validate_bp_tree(t);
            EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
        }
    }
    
    auto loaded_p = create_pager("files/test_default.ndb", false);
    auto loaded_t = bp_tree<DEFAULT_TREE_ORDER>::load(loaded_p.get()).value;
    
    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, loaded_t->exists(i));
    }
}

TEST(BP_TREE_DEFAULT, LOAD_5000)
{
    const auto num_keys = 5000;

    {
        auto p = create_pager("files/test_default.ndb");
        auto t = bp_tree<DEFAULT_TREE_ORDER>::create(p.get()).value;

        for (auto i = 0; i < num_keys; i++)
        {
            EXPECT_EQ(true, t->insert(i, test_value, test_value_size));
            auto result = validate_bp_tree(t);
            EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << i;
        }
    }

    auto loaded_p = create_pager("files/test_default.ndb", false);
    auto loaded_t = bp_tree<DEFAULT_TREE_ORDER>::load(loaded_p.get()).value;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_EQ(true, loaded_t->exists(i));
    }
}

TEST(BP_TREE_DEFAULT, BASIC_FIND)
{
    auto p = create_pager("files/test_default.ndb");
    auto t = bp_tree<DEFAULT_TREE_ORDER>::create(p.get()).value;

    auto key = 10;
    EXPECT_EQ(true, t->insert(key, test_value, test_value_size));
    auto result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << key;

    key = 55;
    EXPECT_EQ(true, t->insert(key, test_value, test_value_size));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << key;

    key = 99;
    EXPECT_EQ(true, t->insert(key, test_value, test_value_size));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << key;

    key = 1231;
    EXPECT_EQ(true, t->insert(key, test_value, test_value_size));
    result = validate_bp_tree(t);
    EXPECT_EQ(true, result.valid) << result.message << std::endl << "key: " << key;

    auto r10 = t->find(10);
    EXPECT_EQ(true, r10->found) << "not found" << std::endl << "key: " << key;
    EXPECT_EQ(test_value_size, r10->size) << "wrong size" << std::endl << "key: " << key;
    EXPECT_TRUE(0 == std::memcmp(test_value, r10->data, test_value_size));

    auto r55 = t->find(55);
    EXPECT_EQ(true, r55->found) << "not found" << std::endl << "key: " << key;
    EXPECT_EQ(test_value_size, r55->size) << "wrong size" << std::endl << "key: " << key;
    EXPECT_TRUE(0 == std::memcmp(test_value, r55->data, test_value_size));

    auto r99 = t->find(99);
    EXPECT_EQ(true, r99->found) << "not found" << std::endl << "key: " << key;
    EXPECT_EQ(test_value_size, r99->size) << "wrong size" << std::endl << "key: " << key;
    EXPECT_TRUE(0 == std::memcmp(test_value, r99->data, test_value_size));

    auto r1231 = t->find(1231);
    EXPECT_EQ(true, r1231->found) << "not found" << std::endl << "key: " << key;
    EXPECT_EQ(test_value_size, r1231->size) << "wrong size" << std::endl << "key: " << key;
    EXPECT_TRUE(0 == std::memcmp(test_value, r1231->data, test_value_size));

    auto r77 = t->find(77);
    EXPECT_EQ(false, r77->found) << "found" << std::endl << "key: " << key;
    EXPECT_EQ(0, r77->size) << "wrong size" << std::endl << "key: " << key;
    EXPECT_TRUE(r77->data == nullptr);
}