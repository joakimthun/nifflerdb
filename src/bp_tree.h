#pragma once

#include <memory>
#include <tuple>
#include <type_traits>
#include <string>
#include <sstream>
#include <stdint.h>

#include "define.h"
#include "util.h"
#include "pager.h"

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
        page_index page = 0;
        page_index root_page = 0;
        page_index leaf_page = 0;
    };

    struct bp_tree_node_child {
        key key;
        page_index page;
    };

    template<size_t N>
    struct bp_tree_node {
        page_index page = 0;
        page_index parent_page = 0;
        page_index next_page = 0;
        page_index prev_page = 0;
        size_t num_children = 0;
        bp_tree_node_child children[N] = { 0 };
    };

    struct bp_tree_record {
        key key;
        value value;
    };

    template<size_t N>
    struct bp_tree_leaf {
        page_index page = 0;
        page_index parent_page = 0;
        page_index next_page = 0;
        page_index prev_page = 0;
        size_t num_children = 0;
        bp_tree_record children[N] = { 0 };
    };

    constexpr size_t DEFAULT_TREE_ORDER = (PAGE_SIZE - 20) / sizeof(bp_tree_node_child);

    enum class lender_side : uint8_t {
        left,
        right
    };

    struct merge_result {
        page_index parent_page;
        page_index page_to_delete;
    };

    template<size_t N>
    class bp_tree {
    public:
        bp_tree() = delete;
        bp_tree(pager *pager);

        static constexpr void assert_sizes();
        static result<bp_tree<N>> load(pager *pager);
        static result<bp_tree<N>> create(pager *pager);

        const bp_tree_header &header() const;
        string print() const;
        bool exists(const key& key) const;
        bool insert(const key& key, const value &value);
        bool remove(const key& key);

        constexpr size_t MIN_NUM_CHILDREN() const { return N / 2; }
        constexpr size_t MAX_NUM_CHILDREN() const { return N; }

        // Use gtest friend stuff?
        //private:
        bool insert_internal(const key& key, const value &value);
        bool remove_internal(const key& key);

        void insert_key(page_index node_page, const key &key, page_index left_page, page_index right_page);
        void insert_key_non_full(bp_tree_node<N> &node, const key &key, page_index next_page);
        void insert_key_at(bp_tree_node<N> &node, const key &key, page_index next_page, size_t index);
        void remove_key_at(bp_tree_node<N> &source, size_t index);
        void set_parent_ptr(bp_tree_node_child *children, size_t c_length, page_index parent_page);
        void remove_by_page(page_index node_page, bp_tree_node<N> &node, page_index page_to_delete);
        bool borrow_key(bp_tree_node<N> &borrower, page_index node_page);
        bool borrow_key(lender_side from_side, bp_tree_node<N> &borrower, page_index node_page);
        void insert_node_at(bp_tree_node<N> &node, const key &key, page_index page, size_t index);
        merge_result merge_node(bp_tree_node<N> &node, page_index node_page, bool is_last);
        void merge_nodes(bp_tree_node<N> &first, bp_tree_node<N> &second);

        void change_parent(page_index parent_page, const key &old_key, const key &new_key);
        bool borrow_key(bp_tree_leaf<N> &borrower);
        bool borrow_key(lender_side from_side, bp_tree_leaf<N> &borrower);
        merge_result merge_leaf(bp_tree_leaf<N> &leaf, page_index leaf_page, bool is_last);
        void merge_leafs(bp_tree_leaf<N> &first, bp_tree_leaf<N> &second);

        void transfer_children(bp_tree_node<N>  &source, bp_tree_node<N>  &target, size_t from_index);

        void insert_record_non_full(bp_tree_leaf<N> &leaf, const key &key, const value &value);
        void insert_record_at(bp_tree_leaf<N> &leaf, const key &key, const value &value, size_t index);
        void insert_record_split(const key& key, const value &value, page_index leaf_page, bp_tree_leaf<N> &leaf, bp_tree_leaf<N> &new_leaf);
        void transfer_records(bp_tree_leaf<N> &source, bp_tree_leaf<N> &target, size_t from_index);
        bool remove_record(bp_tree_leaf<N> &source, const key &key);
        void remove_record_at(bp_tree_leaf<N> &source, size_t index);

        void promote_larger_key(const key &key_to_promote, page_index node_page, page_index parent_page);
        void promote_smaller_key(const key &key_to_promote, page_index node_page, page_index parent_page);

        template<class T>
        tuple<bool, size_t> find_split_index(const T *arr, size_t arr_len, const key &key);

        page_index search_tree(const key &key) const;
        page_index search_node(page_index page, const key &key) const;
        page_index search_node(bp_tree_node<N> &node, const key &key) const;
        size_t find_insert_index(const bp_tree_leaf<N> &leaf, const key &key) const;
        size_t find_insert_index(const bp_tree_node<N> &node, const key &key) const;
        bp_tree_node_child &find_node_child(bp_tree_node<N> &node, const key &key) const;
        int64_t binary_search_record(const bp_tree_leaf<N> &leaf, const key &key) const;

        page_index alloc_node(bp_tree_node<N> &node);
        page_index alloc_leaf(bp_tree_leaf<N> &leaf);
        page_index alloc(size_t size);
        void free(bp_tree_node<N> &node, page_index node_page);
        void free(bp_tree_leaf<N> &leaf, page_index leaf_page);
        void free(size_t size, page_index page);

        template<class T, class NodeAllocator>
        page_index create(page_index node_page, T &node, T &new_node, NodeAllocator node_allocator);

        void print_node_level(stringstream &ss, page_index node_page) const;
        void print_leaf_level(stringstream &ss, page_index leaf_page) const;

        template<class T>
        void remove(T &prev, T &node);

        template<class T>
        void load(T *buffer, page_index page) const;
        template<class T>
        void save(T *value, page_index page) const;

        pager *pager_;
        bp_tree_header header_;
    };
 
}
