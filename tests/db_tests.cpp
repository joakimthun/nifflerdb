#include <gtest\gtest.h>
#include <stdlib.h>
#include <thread>

#include "include/db.h"

using namespace niffler;

char *db_test_value = "test value";
const auto db_test_value_size = strlen(db_test_value);

TEST(DB, BASIC)
{
    auto niffler = std::make_unique<db>("files/db_basic.ndb", true);

    auto exists = niffler->exists("test_key");
    EXPECT_FALSE(exists);

    auto insert = niffler->insert("test_key", db_test_value, db_test_value_size);
    EXPECT_TRUE(insert);

    exists = niffler->exists("test_key");
    EXPECT_TRUE(exists);

    auto remove = niffler->remove("test_key");
    EXPECT_TRUE(remove);

    remove = niffler->remove("test_key");
    EXPECT_FALSE(remove);
}

TEST(DB, MULTI_THREADED_FIND)
{
    auto niffler = std::make_unique<db>("files/db_threaded.ndb", true);
    const auto num_keys = 1000;
    
    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_TRUE(niffler->insert(i, db_test_value, db_test_value_size));
    }

    auto find = [&niffler, &num_keys]() {
        for (int i = 0; i < num_keys; i++) {
            auto find_result = niffler->find(i);
            EXPECT_TRUE(find_result->found);
            EXPECT_TRUE(find_result->size == db_test_value_size);
        }
    };

    std::thread thread1(find);
    std::thread thread2(find);
    std::thread thread3(find);

    thread1.join();
    thread2.join();
    thread3.join();
}

TEST(DB, MULTI_THREADED_FIND_REMOVE_INSERT)
{
    auto niffler = std::make_unique<db>("files/db_threaded.ndb", true);
    const auto num_keys = 1000;

    for (auto i = 0; i < num_keys; i++)
    {
        EXPECT_TRUE(niffler->insert(i, db_test_value, db_test_value_size));
    }

    auto find = [&niffler, &num_keys]() {
        for (int i = 500; i < num_keys; i++) {
            auto find_result = niffler->find(i);
            EXPECT_TRUE(find_result->found);
            EXPECT_TRUE(find_result->size == db_test_value_size);
        }
    };

    auto remove = [&niffler]() {
        for (int i = 0; i < 500; i++) {
            EXPECT_TRUE(niffler->remove(i));
            EXPECT_FALSE(niffler->remove(i));
        }
    };

    auto insert = [&niffler]() {
        for (int i = 1000; i < 1500; i++) {
            EXPECT_TRUE(niffler->insert(i, db_test_value, db_test_value_size));
            EXPECT_FALSE(niffler->insert(i, db_test_value, db_test_value_size));
        }
    };

    std::thread thread1(find);
    std::thread thread2(find);

    std::thread thread3(remove);
    std::thread thread4(insert);

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();
}
