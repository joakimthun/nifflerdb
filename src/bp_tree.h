#pragma once

#include <memory>
#include <tuple>
#include <type_traits>
#include <string>
#include <sstream>
#include <stdint.h>

#include "define.h"
#include "util.h"
#include "storage_provider.h"

namespace niffler {

    using std::size_t;
    using std::tuple;
    using std::string;
    using std::stringstream;

    using key = int;
    using value = int;

    struct bp_tree_header {
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
        size_t num_children = 0;
        bp_tree_record children[N] = { 0 };
    };

    constexpr size_t DEFAULT_TREE_ORDER = (PAGE_SIZE - 16) / sizeof(bp_tree_node_child);

    enum class lender_side : uint8_t {
        left,
        right
    };

    struct merge_result {
        offset parent_offset;
        offset offset_to_delete;
    };

    template<size_t N>
    class bp_tree {
    public:
        bp_tree() = delete;
        bp_tree(std::unique_ptr<storage_provider> storage);

        static constexpr void assert_sizes();
        static result<bp_tree<N>> load(std::unique_ptr<storage_provider> storage);
        static result<bp_tree<N>> create(std::unique_ptr<storage_provider> storage);

        const bp_tree_header &header() const;
        string print() const;
        storage_provider &storage();
        bool exists(const key& key) const;
        bool insert(const key& key, const value &value);
        bool remove(const key& key);

        constexpr size_t MIN_NUM_CHILDREN() const { return N / 2; }
        constexpr size_t MAX_NUM_CHILDREN() const { return N; }

        // Use gtest friend stuff?
        //private:
        bool insert_internal(const key& key, const value &value);
        bool remove_internal(const key& key);

        void insert_key(offset node_offset, const key &key, offset left_offset, offset right_offset);
        void insert_key_non_full(bp_tree_node<N> &node, const key &key, offset next_offset);
        void insert_key_at(bp_tree_node<N> &node, const key &key, offset next_offset, size_t index);
        void remove_key_at(bp_tree_node<N> &source, size_t index);
        void set_parent_ptr(bp_tree_node_child *children, size_t c_length, offset parent);
        void remove_by_offset(offset node_offset, bp_tree_node<N> &node, offset offset_to_delete);
        bool borrow_key(bp_tree_node<N> &borrower, offset node_offset);
        bool borrow_key(lender_side from_side, bp_tree_node<N> &borrower, offset node_offset);
        void insert_node_at(bp_tree_node<N> &node, const key &key, offset offset, size_t index);
        merge_result merge_node(bp_tree_node<N> &node, offset node_offset, bool is_last);
        void merge_nodes(bp_tree_node<N> &first, bp_tree_node<N> &second);

        void change_parent(offset parent_offset, const key &old_key, const key &new_key);
        bool borrow_key(bp_tree_leaf<N> &borrower);
        bool borrow_key(lender_side from_side, bp_tree_leaf<N> &borrower);
        merge_result merge_leaf(bp_tree_leaf<N> &leaf, offset leaf_offset, bool is_last);
        void merge_leafs(bp_tree_leaf<N> &first, bp_tree_leaf<N> &second);

        void transfer_children(bp_tree_node<N>  &source, bp_tree_node<N>  &target, size_t from_index);

        void insert_record_non_full(bp_tree_leaf<N> &leaf, const key &key, const value &value);
        void insert_record_at(bp_tree_leaf<N> &leaf, const key &key, const value &value, size_t index);
        void insert_record_split(const key& key, const value &value, offset leaf_offset, bp_tree_leaf<N> &leaf, bp_tree_leaf<N> &new_leaf);
        void transfer_records(bp_tree_leaf<N> &source, bp_tree_leaf<N> &target, size_t from_index);
        bool remove_record(bp_tree_leaf<N> &source, const key &key);
        void remove_record_at(bp_tree_leaf<N> &source, size_t index);

        void promote_larger_key(const key &key_to_promote, offset node_offset, offset parent_offset, bool promote_parent);
        void promote_smaller_key(const key &key_to_promote, offset node_offset, offset parent_offset, bool promote_parent);

        template<class T>
        tuple<bool, size_t> find_split_index(const T *arr, size_t arr_len, const key &key);

        offset search_tree(const key &key) const;
        offset search_node(offset offset, const key &key) const;
        offset search_node(bp_tree_node<N> &node, const key &key) const;
        size_t find_insert_index(const bp_tree_leaf<N> &leaf, const key &key) const;
        size_t find_insert_index(const bp_tree_node<N> &node, const key &key) const;
        bp_tree_node_child &find_node_child(bp_tree_node<N> &node, const key &key) const;
        int64_t binary_search_record(const bp_tree_leaf<N> &leaf, const key &key) const;

        offset alloc_node(bp_tree_node<N> &node);
        offset alloc_leaf(bp_tree_leaf<N> &leaf);
        offset alloc(size_t size);
        void free(bp_tree_node<N> &node, offset node_offset);
        void free(bp_tree_leaf<N> &leaf, offset leaf_offset);
        void free(size_t size, offset offset);

        template<class T, class NodeAllocator>
        offset create(offset node_offset, T &node, T &new_node, NodeAllocator node_allocator);

        void print_node_level(stringstream &ss, offset node_offset) const;
        void print_leaf_level(stringstream &ss, offset leaf_offset) const;

        template<class T>
        void remove(T &prev, T &node);

        template<class T>
        void load(T *buffer, offset offset) const;
        template<class T>
        void save(T *value, offset offset) const;

        std::unique_ptr<storage_provider> storage_;
        bp_tree_header header_;
    };
 
}
