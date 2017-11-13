#include <gtest\gtest.h>
#include <stdlib.h>
#include <unordered_set>
#include <iostream>

#include "storage_providers.h"
#include "bp_tree.h"
#include "test_helpers.h"

using namespace niffler;

void random_keys_test(std::size_t rand_seed, std::size_t num_keys)
{
    auto t = bp_tree<DEFAULT_TREE_ORDER>::create(create_storage_provider("files/random_keys_test.ndb")).value;
    srand(rand_seed);
    auto added_keys = std::unordered_set<int>();

    for (auto i = 0u; i < num_keys; i++)
    {
        auto key = rand();

        // Key already added
        if (added_keys.find(key) != added_keys.end())
            continue;

        auto i_res = t->insert(key, i);
        EXPECT_EQ(true, i_res) << "insert key: " << key << ", index: " << i << ", seed:" << rand_seed;
        auto result = validate_bp_tree(t);
        EXPECT_EQ(true, result.valid) << result.message << std::endl << "insert key: " << key << ", index: " << i << ", seed:" << rand_seed;
        i_res = t->insert(key, i);
        EXPECT_EQ(false, i_res);
        added_keys.insert(key);
    }

    auto i = 0;
    auto keys_to_check = added_keys;
    for (const auto k : added_keys)
    {
        auto rem = t->remove(k);
        EXPECT_EQ(true, rem) << "remove key: " << k << " i: " << i << ", seed:" << rand_seed;
        if (!rem)
            return;

        keys_to_check.erase(k);
        auto ii = 0;
        for (auto key : keys_to_check)
        {
            auto exists = t->exists(key);
            EXPECT_EQ(true, exists) << "exists key: " << key << " remove key: " << k << " i: " << i << ", seed:" << rand_seed << " ii: " << ii;
            if (!exists)
                return;

            ii++;
        }

        i++;
    }
}

//TEST(BP_TREE_DEFAULT, TEMP)
//{
//    /*for (auto i = 0; i < 50; i++)
//    {
//        random_keys_test(i, 50);
//        std::cout << i << " done" << std::endl;
//    }*/
//
//    //c:\dev\nifflerdb\tests\bp_tree_default_tests.cpp(64) : error:       Expected: true
//    //To be equal to : exi
//    //Which is : false
//    //exists key : 14794 remove key : 10777 i : 22, seed : 29
//
//
//    /*for (auto i = 0; i < 100; i++)
//    {
//        random_keys_test(i, 100);
//        std::cout << i << " done" << std::endl;
//    }*/
//
//    //random_keys_test(0, 200);
//
//    /*for (auto i = 0; i < 5; i++)
//    {
//        random_keys_test(i, 500);
//        std::cout << std::endl << std::endl << i << " done ---------------- " << std::endl;
//    }*/
//
//    //random_keys_test(2, 500);
//    //random_keys_test(3, 500);
//
//    //random_keys_test(29, 100);
//    //random_keys_test(30, 800);
//
//    //random_keys_test(77, 30);
//    //random_keys_test(29, 100);
//
//    /*for (auto i = 0; i < 100; i++)
//    {
//        random_keys_test(117, i);
//    }*/
//
//    //random_keys_test(28, 50);
//    //random_keys_test(28, 100);
//
//    for (auto i = 0; i < 500; i++)
//    {
//        random_keys_test(i, 100);
//        std::cout << i << " done" << std::endl;
//    }
//
//    //random_keys_test(77, 100);
//    //random_keys_test(176, 100);
//    //random_keys_test(77, 100);
//}

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