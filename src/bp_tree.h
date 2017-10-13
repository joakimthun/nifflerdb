#pragma once

#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <string>
#include <sstream>
#include <stdint.h>

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

    template<size_t N>
    struct bp_tree_node {
        offset parent = 0;
        offset next = 0;
        offset prev = 0;
        size_t num_children = 0;
        bp_tree_node_child children[N] = { 0 };
    };

    struct bp_tree_record {
        key key;
        value value;
    };

    template<size_t N>
    struct bp_tree_leaf {
        offset parent = 0;
        offset next = 0;
        offset prev = 0;
        size_t num_records = 0;
        bp_tree_record records[N] = { 0 };
    };

    template<size_t N>
    class bp_tree {
    public:
        bp_tree() = delete;
        bp_tree(std::unique_ptr<storage_provider> storage);

        static result<bp_tree<N>> load(std::unique_ptr<storage_provider> storage);
        static result<bp_tree<N>> create(std::unique_ptr<storage_provider> storage);

        const bp_tree_info &info() const;
        string print() const;
        bool insert(const key& key, const value &value);
        bool remove(const key& key);

        constexpr size_t MIN_NUM_CHILDREN() const { return N / 2; }
        constexpr size_t MAX_NUM_CHILDREN() const { return N; }

        // Use gtest friend stuff?
        //private:
        void insert_key(offset node_offset, const key &key, offset left_offset, offset right_offset);
        void insert_key_non_full(bp_tree_node<N> &node, const key &key, offset next_offset);
        void insert_key_at(bp_tree_node<N> &node, const key &key, offset next_offset, size_t index);
        void set_parent_ptr(bp_tree_node_child *children, size_t c_length, offset parent);

        void transfer_children(bp_tree_node<N>  &source, bp_tree_node<N>  &target, size_t from_index);

        void insert_record_non_full(bp_tree_leaf<N> &leaf, const key &key, const value &value);
        void insert_record_at(bp_tree_leaf<N> &leaf, const key &key, const value &value, size_t index);
        void insert_record_split(const key& key, const value &value, offset leaf_offset, bp_tree_leaf<N> &leaf, bp_tree_leaf<N> &new_leaf);
        void transfer_records(bp_tree_leaf<N> &source, bp_tree_leaf<N> &target, size_t from_index);
        bool remove_record(bp_tree_leaf<N> &source, const key &key);
        void remove_record_at(bp_tree_leaf<N> &source, size_t index);

        template<class T>
        tuple<bool, size_t> find_split_index(const T *arr, size_t arr_len, const key &key);

        offset search_tree(const key &key) const;
        offset search_node(offset offset, const key &key) const;
        size_t find_insert_index(const bp_tree_leaf<N> &leaf, const key &key) const;
        size_t find_insert_index(const bp_tree_node<N> &node, const key &key) const;
        const bp_tree_node_child &find_node_child(const bp_tree_node<N> &node, const key &key) const;
        int64_t binary_search_record(const bp_tree_leaf<N> &leaf, const key &key);

        offset alloc_node(bp_tree_node<N> &node);
        offset alloc_leaf(bp_tree_leaf<N> &leaf);
        offset alloc(size_t size);

        offset create_leaf(offset leaf_offset, bp_tree_leaf<N> &leaf, bp_tree_leaf<N> &new_leaf);
        offset create_node(offset node_offset, bp_tree_node<N> &node, bp_tree_node<N> &new_node);

        void print_node_level(stringstream &ss, offset node_offset) const;
        void print_leaf_level(stringstream &ss, offset leaf_offset) const;

        template<class T>
        void load(T *buffer, offset offset) const;
        template<class T>
        void save(T *value, offset offset) const;

        std::unique_ptr<storage_provider> storage_;
        bp_tree_info info_;

    };
 
}
