#include <gtest\gtest.h>
#include <stdlib.h>
#include <unordered_set>

#include "storage_provider.h"
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