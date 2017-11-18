#pragma once

#include <gtest\gtest.h>
#include <string.h>
#include <memory>
#include <unordered_set>
#include <iostream>

#include "files.h"
#include "bp_tree.h"
#include "memory_storage_provider.h"
#include "file_storage_provider.h"

using namespace niffler;

struct bp_tree_validation_result {
    inline bp_tree_validation_result(const char *msg)
        : valid(false)
    {
        strcpy_s(message, 64, msg);
    }

    inline bp_tree_validation_result(bool valid)
        : valid(valid)
    {
        strcpy_s(message, 64, "ok");
    }

    bool valid;
    char message[64];
};

template<size_t N>
bp_tree_validation_result validate_bp_tree(std::unique_ptr<bp_tree<N>> &tree);

inline std::unique_ptr<storage_provider> create_storage_provider(const char *file_path, file_mode fm = file_mode::write_update)
{
    //return std::make_unique<file_storage_provider>(file_path, file_mode::write_update);
    //return std::make_unique<memory_storage_provider>();
    return std::make_unique<memory_storage_provider>(std::make_unique<file_storage_provider>(file_path, fm));
}

inline void random_keys_test(std::size_t rand_seed, std::size_t num_keys)
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

        auto insert_result = t->insert(key, i);
        EXPECT_EQ(true, insert_result) << "insert key: " << key << ", index: " << i << ", seed:" << rand_seed;

        auto validate_result = validate_bp_tree(t);
        EXPECT_EQ(true, validate_result.valid) << validate_result.message << std::endl << "insert key: " << key << ", index: " << i << ", seed:" << rand_seed;

        insert_result = t->insert(key, i);

        EXPECT_EQ(false, insert_result);
        added_keys.insert(key);
    }

    auto added_keys_index = 0;
    auto keys_to_check = added_keys;
    for (const auto k : added_keys)
    {
        auto remove_result = t->remove(k);
        EXPECT_EQ(true, remove_result) << "remove key: " << k << " added_keys_index: " << added_keys_index << ", seed:" << rand_seed;
        if (!remove_result)
            return;

        auto validate_result = validate_bp_tree(t);
        EXPECT_EQ(true, validate_result.valid) << validate_result.message << std::endl << "key: " << k;

        keys_to_check.erase(k);
        auto keys_to_check_index = 0;
        for (auto key : keys_to_check)
        {
            auto exists_result = t->exists(key);
            EXPECT_EQ(true, exists_result) << "exists key: " << key << " remove key: " << k << " added_keys_index: " << added_keys_index << ", seed:" << rand_seed << " keys_to_check_index: " << keys_to_check_index;
            if (!exists_result)
                return;

            keys_to_check_index++;
        }

        added_keys_index++;
    }
}

