#include <gtest\gtest.h>
#include <stdlib.h>

#include "bp_tree.h"
#include "test_helpers.h"

using namespace niffler;

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