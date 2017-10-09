#pragma once

#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <string>
#include <sstream>

#include "files.h"
#include "util.h"
#include "storage_providers.h"

namespace niffler {

    using std::size_t;
    using std::tuple;
    using std::string;
    using std::stringstream;

    using offset = std::size_t;
    using key = int;
    using value = int;

    constexpr size_t BP_TREE_ORDER = 10;

    struct bp_tree_info {
        size_t order = 0;
        size_t value_size = 0;
        size_t key_size = 0;
        size_t num_internal_nodes = 0;
        size_t num_leaf_nodes = 0;
        size_t height = 0;
        offset next_slot_offset = 0;
        offset root_offset = 0;
        offset leaf_offset = 0;
    };

    struct bp_tree_node_child {
        key key;
        offset offset;
    };

    struct bp_tree_node {
        offset parent = 0;
        offset next = 0;
        offset prev = 0;
        size_t num_children = 0;
        bp_tree_node_child children[BP_TREE_ORDER] = { 0 };
    };

    struct bp_tree_record {
        key key;
        value value;
    };

    struct bp_tree_leaf {
        offset parent = 0;
        offset next = 0;
        offset prev = 0;
        size_t num_records = 0;
        bp_tree_record records[BP_TREE_ORDER] = { 0 };
    };

    class bp_tree {
    public:
        bp_tree() = delete;
        static result<bp_tree> load(std::unique_ptr<storage_provider> storage);
        static result<bp_tree> create(std::unique_ptr<storage_provider> storage);

        const bp_tree_info &info() const;
        string print() const;
        bool insert(const key& key, const value &value);

        // Use gtest friend stuff?
        //private:
        bp_tree(std::unique_ptr<storage_provider> storage);

        void insert_key(offset node_offset, const key &key, offset left_offset, offset right_offset);
        void insert_key_non_full(bp_tree_node &node, const key &key, offset next_offset);
        void insert_key_at(bp_tree_node &node, const key &key, offset next_offset, size_t index);
        void set_parent_ptr(bp_tree_node_child *children, size_t c_length, offset parent);

        void transfer_children(bp_tree_node  &source, bp_tree_node  &target, size_t from_index);

        void insert_record_non_full(bp_tree_leaf &leaf, const key &key, const value &value);
        void insert_record_at(bp_tree_leaf &leaf, const key &key, const value &value, size_t index);
        void insert_record_split(const key& key, const value &value, offset leaf_offset, bp_tree_leaf &leaf, bp_tree_leaf &new_leaf);
        void transfer_records(bp_tree_leaf &source, bp_tree_leaf &target, size_t from_index);

        template<class T>
        tuple<bool, size_t> find_split_index(const T *arr, size_t arr_len, const key &key)
        {
            static_assert(std::is_same<T, bp_tree_record>::value || std::is_same<T, bp_tree_node_child>::value, "T must be a record or a node child");

            auto split_index = arr_len / 2;
            auto key_greater_than_key_at_split = key > arr[split_index].key;

            // If the key we are inserting is greater than the key at split_index we make sure
            // the left node/leaf is bigger after the split since we will be inserting into the right node/leaf
            if (key_greater_than_key_at_split)
                split_index++;

            return std::make_tuple(key_greater_than_key_at_split, split_index);
        }

        offset search_tree(const key &key) const;
        offset search_node(offset offset, const key &key) const;
        size_t find_insert_index(const bp_tree_leaf &leaf, const key &key) const;
        size_t find_insert_index(const bp_tree_node &node, const key &key) const;
        const bp_tree_node_child &find_node_child(const bp_tree_node &node, const key &key) const;
        bool binary_search(const bp_tree_leaf &leaf, const key &key);

        offset alloc_node(bp_tree_node &node);
        offset alloc_leaf(bp_tree_leaf &leaf);
        offset alloc(size_t size);

        offset create_leaf(offset leaf_offset, bp_tree_leaf &leaf, bp_tree_leaf &new_leaf);
        offset create_node(offset node_offset, bp_tree_node &node, bp_tree_node &new_node);

        void print_node_level(stringstream &ss, offset node_offset) const;
        void print_leaf_level(stringstream &ss, offset leaf_offset) const;

        template<class T>
        void load_from_storage(T *buffer, offset offset) const
        {
            static_assert(std::is_same<T, bp_tree_node>::value || std::is_same<T, bp_tree_leaf>::value || std::is_same<T, bp_tree_info>::value, "T must be a node, leaf or info");
            storage_->load(buffer, offset, sizeof(T));
        }

        template<class T>
        void save_to_storage(T *value, offset offset) const
        {
            static_assert(std::is_same<T, bp_tree_node>::value || std::is_same<T, bp_tree_leaf>::value || std::is_same<T, bp_tree_info>::value, "T must be a node, leaf or info");
            storage_->store(value, offset, sizeof(T));
        }

        std::unique_ptr<storage_provider> storage_;
        bp_tree_info info_;

    };

}
