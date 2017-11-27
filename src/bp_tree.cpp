#include "bp_tree.h"

#include <assert.h>
#include <stdlib.h>

#include "serialization.h"

namespace niffler {

    using std::tie;
    using std::stringstream;

    constexpr u32 HEADER_PAGE_INDEX = 1;

    template<u32 N>
    constexpr void bp_tree<N>::assert_sizes()
    {
        static_assert(sizeof(bp_tree_header) == 32, "sizeof(bp_tree_header) != 32");
        static_assert(sizeof(bp_tree_node_child) == 8, "sizeof(bp_tree_node_child) != 8");
        static_assert(sizeof(bp_tree_record) == 8, "sizeof(bp_tree_record) != 8");
        static_assert(sizeof(bp_tree_node<N>) == (20 + sizeof(bp_tree_node_child) * N), "wrong size: bp_tree_node<N>");
        static_assert(sizeof(bp_tree_leaf<N>) == (20 + sizeof(bp_tree_record) * N), "wrong size: bp_tree_leaf<N>");
    }

    template<u32 N>
    result<bp_tree<N>> bp_tree<N>::load(pager *pager)
    {
        bp_tree<N>::assert_sizes();
        
        auto t = std::make_unique<bp_tree<N>>(pager);
        t->load(t->header_, HEADER_PAGE_INDEX);
        
        return result<bp_tree<N>>(true, std::move(t));
    }

    template<u32 N>
    result<bp_tree<N>> bp_tree<N>::create(pager *pager)
    {
        bp_tree<N>::assert_sizes();

        auto t = std::make_unique<bp_tree<N>>(pager);

        auto header_page = pager->get_free_page();
        assert(header_page.index == HEADER_PAGE_INDEX);

        t->header_.order = N;
        t->header_.key_size = sizeof(key);
        t->header_.height = 1;

        auto root_page = pager->get_free_page();
        t->header_.num_internal_nodes = 1;
        bp_tree_node<N> root;
        t->header_.root_page = root_page.index;

        bp_tree_leaf<N> leaf;
        leaf.parent_page = t->header_.root_page;

        auto leaf_page = pager->get_free_page();
        t->header_.num_leaf_nodes = 1;
        t->header_.leaf_page = leaf_page.index;
        root.children[0].page = leaf_page.index;
        root.num_children = 1;

        // Save inital tree to the underlying storage
        t->save(t->header_, HEADER_PAGE_INDEX);
        t->save(root, root_page.index);
        t->save(leaf, leaf_page.index);

        auto sync_result = pager->sync();
        return result<bp_tree<N>>(sync_result, std::move(t));
    }

    template<u32 N>
    const bp_tree_header &bp_tree<N>::header() const
    {
        return header_;
    }

    template<u32 N>
    string bp_tree<N>::print() const
    {
        stringstream ss;
        auto height = header_.height;
        auto current_page = header_.root_page;

        do
        {
            bp_tree_node<N> n;
            load(n, current_page);
            print_node_level(ss, current_page);
            ss << std::endl;

            current_page = n.children[0].page;
            height--;
        } while (height > 0);

        print_leaf_level(ss, current_page);

        return ss.str();
    }

    template<u32 N>
    bool bp_tree<N>::exists(const key & key) const
    {
        auto parent_page = search_tree(key);
        assert(parent_page != 0);

        bp_tree_node<N> parent;
        load(parent, parent_page);

        auto leaf_page = search_node(parent, key);
        assert(leaf_page != 0);

        bp_tree_leaf<N> leaf;
        load(leaf, leaf_page);

        return binary_search_record(leaf, key) >= 0;
    }

    template<u32 N>
    bool bp_tree<N>::insert(const key &key, const value &value)
    {
        if (insert_internal(key, value))
            return pager_->sync();

        return false;
    }

    template<u32 N>
    bool bp_tree<N>::remove(const key &key)
    {
        if (remove_internal(key))
            return pager_->sync();

        return false;
    }

    template<u32 N>
    bp_tree<N>::bp_tree(pager *pager)
        :
        pager_(pager)
    {
    }

    template<u32 N>
    bool bp_tree<N>::insert_internal(const key &key, const value &value)
    {
        auto parent_page = search_tree(key);
        assert(parent_page != 0);

        auto leaf_page = search_node(parent_page, key);
        assert(leaf_page != 0);

        bp_tree_leaf<N> leaf;
        load(leaf, leaf_page);

        // Key already exists
        if (binary_search_record(leaf, key) >= 0)
            return false;

        if (leaf.num_children == header_.order)
        {
            bp_tree_leaf<N> new_leaf;
            insert_record_split(key, value, leaf_page, leaf, new_leaf);
            insert_key(parent_page, new_leaf.children[0].key, leaf_page, leaf.next_page);
        }
        else
        {
            insert_record_non_full(leaf, key, value);
            save(leaf, leaf_page);
        }

        return true;
    }

    template<u32 N>
    bool bp_tree<N>::remove_internal(const key &key)
    {
        auto parent_page = search_tree(key);
        assert(parent_page != 0);

        bp_tree_node<N> parent;
        load(parent, parent_page);

        auto leaf_page = search_node(parent, key);
        assert(leaf_page != 0);

        bp_tree_leaf<N> leaf;
        load(leaf, leaf_page);

        // Return false if the key does not exists
        if (!remove_record(leaf, key))
            return false;

        // If this is the only leaf we cant really borrow/merge so we accept any number of records
        const auto min_num_records = header_.num_leaf_nodes == 1 ? 0 : MIN_NUM_CHILDREN();

        if (leaf.num_children < min_num_records)
        {
            auto could_borrow = borrow_key(leaf);

            if (!could_borrow)
            {
                auto merge_result = merge_leaf(leaf, leaf_page, leaf.next_page == 0);

                if (merge_result.parent_page != parent_page)
                {
                    load(parent, merge_result.parent_page);
                    remove_by_page(merge_result.parent_page, parent, merge_result.page_to_delete);
                }
                else
                {
                    remove_by_page(parent_page, parent, merge_result.page_to_delete);
                }
            }
            else
            {
                save(leaf, leaf_page);
            }
        }
        else
        {
            save(leaf, leaf_page);
        }

        return true;
    }

    template<u32 N>
    void bp_tree<N>::insert_key(page_index node_page, const key &key, page_index left_page, page_index right_page)
    {
        // We have reached the root of the tree so we create a new root node
        if (node_page == 0)
        {
            bp_tree_node<N> root;
            header_.root_page = alloc_node(root);
            header_.height++;

            root.num_children = 2;
            root.children[0].key = key;
            root.children[0].page = left_page;
            root.children[1].page = right_page;

            save(header_, HEADER_PAGE_INDEX);
            save(root, header_.root_page);

            set_parent_ptr(root.children, root.num_children, header_.root_page);

            return;
        }

        bp_tree_node<N> node;
        load(node, node_page);

        if (node.num_children == header_.order)
        {
            bp_tree_node<N> new_node;
            auto new_node_page = create(node_page, node, new_node, [this](auto n) { return alloc_node(n); });

            bool key_greater_than_key_at_split;
            u32 split_index;
            std::tie(key_greater_than_key_at_split, split_index) = find_split_index(node.children, node.num_children - 1, key);

            /*
                Prevent edge case where the key would be inserted into the right node but is less
                than key@split_index and we would end up with an unsorted tree
                E.g.
                Insert key: 5528
                Wrong:
                Original: [748, 1535, 2713, 3757, 4629, 6637, 7327, 8507, 8874, 9718] -> Split index: 5 -> 6637
                Result:   [748, 1535, 2713, 3757, 4629, 6637] [5528, 7327, 8507, 8874, 9718]

                Right:
                Original: [748, 1535, 2713, 3757, 4629, 6637, 7327, 8507, 8874, 9718] -> Split index: 4 -> 4629
                Result:   [748, 1535, 2713, 3757, 4629] [5528, 6637, 7327, 8507, 8874, 9718]
            */
            if (key_greater_than_key_at_split && key < node.children[split_index].key)
                split_index--;

            // Key to transfer to parent
            auto middle_key = node.children[split_index].key;

            // Add 1 to the index to skip the "middle key" while transfering
            transfer_children(node, new_node, split_index + 1);

            if (key_greater_than_key_at_split)
            {
                insert_key_non_full(new_node, key, right_page);
            }
            else
            {
                insert_key_non_full(node, key, right_page);
            }

            save(node, node_page);
            save(new_node, new_node_page);
            set_parent_ptr(new_node.children, new_node.num_children, new_node_page);

            // Insert the middle key into the parent node
            insert_key(node.parent_page, middle_key, node_page, new_node_page);
        }
        else
        {
            insert_key_non_full(node, key, right_page);
            save(node, node_page);
        }
    }

    template<u32 N>
    void bp_tree<N>::insert_key_non_full(bp_tree_node<N> &node, const key &key, page_index next_page)
    {
        auto dest_index = find_insert_index(node, key);
        insert_key_at(node, key, next_page, dest_index);
    }

    template<u32 N>
    void bp_tree<N>::insert_key_at(bp_tree_node<N> &node, const key &key, page_index next_page, u32 index)
    {
        assert(index < (node.num_children + 1));

        // Inserting at the end
        if (index == node.num_children)
        {
            node.children[index].key = key;
            node.children[index].page = next_page;
        }
        else
        {
            // Use a signed index to prevent i from wrapping on 0--
            for (auto i = static_cast<int32_t>(node.num_children) - 1; i >= static_cast<int32_t>(index); i--)
            {
                node.children[i + 1] = node.children[i];
            }

            node.children[index].key = key;

            // Point the newly inserted key to the page of the previous key at this index and
            // point the key to the right(previous key) of the new key to the next_page
            node.children[index].page = node.children[index + 1].page;
            node.children[index + 1].page = next_page;
        }

        node.num_children++;
    }

    template<u32 N>
    void bp_tree<N>::remove_key_at(bp_tree_node<N>& source, u32 index)
    {
        assert(index < source.num_children);
        assert(source.num_children > 0);

        for (auto i = index; i < source.num_children - 1; i++)
        {
            source.children[i] = source.children[i + 1];
        }

        source.num_children--;
    }

    template<u32 N>
    void bp_tree<N>::set_parent_ptr(bp_tree_node_child *children, u32 c_length, page_index parent_page)
    {
        bp_tree_node<N> node;
        for (auto i = 0u; i < c_length; i++)
        {
            load(node, children[i].page);
            node.parent_page = parent_page;
            save(node, children[i].page);
        }
    }

    template<u32 N>
    void bp_tree<N>::remove_by_page(page_index node_page, bp_tree_node<N> &node, page_index page_to_delete)
    {
        const auto min_num_children = node.parent_page == 0 ? 1 : MIN_NUM_CHILDREN();

        auto delete_index = 0u;
        auto delete_index_set = false;
        for (auto i = 0u; i < node.num_children; i++)
        {
            if (node.children[i].page == page_to_delete)
            {
                delete_index = i;
                delete_index_set = true;
                break;
            }
        }

        assert(delete_index_set && "delete_index not set");
        assert(delete_index <= (node.num_children - 1));

        if (delete_index > 0)
        {
            node.children[delete_index - 1].key = node.children[delete_index].key;
        }

        remove_key_at(node, delete_index);


        // If we only have one child left we make that child the new root and free the old root
        if (node.num_children == 1 && header_.root_page == node_page && header_.num_internal_nodes != 1)
        {
            free(node, header_.root_page);
            header_.height--;
            header_.root_page = node.children[0].page;
            save(header_, HEADER_PAGE_INDEX);

            bp_tree_node<N> root;
            load(root, header().root_page);
            root.parent_page = 0;
            save(root, header().root_page);
            return;
        }

        if (node.num_children < min_num_children)
        {
            auto could_borrow = borrow_key(node, node_page);

            if (!could_borrow)
            {
                auto merge_result = merge_node(node, node_page, node.next_page == 0);

                bp_tree_node<N> parent;
                load(parent, merge_result.parent_page);
                remove_by_page(merge_result.parent_page, parent, merge_result.page_to_delete);
            }
            else
            {
                save(node, node_page);
            }
        }
        else
        {
            save(node, node_page);
        }
    }

    template<u32 N>
    bool bp_tree<N>::borrow_key(bp_tree_node<N> &borrower, page_index node_page)
    {
        auto could_borrow = borrow_key(lender_side::left, borrower, node_page);
        if (could_borrow)
            return true;

        return borrow_key(lender_side::right, borrower, node_page);
    }

    template<u32 N>
    bool bp_tree<N>::borrow_key(lender_side from_side, bp_tree_node<N> &borrower, page_index node_page)
    {
        assert(borrower.num_children < MIN_NUM_CHILDREN());
        const auto lender_page = from_side == lender_side::right ? borrower.next_page : borrower.prev_page;

        if (lender_page == 0)
        {
            // The borrower has no left/right neighbour
            return false;
        }

        bp_tree_node<N> lender;
        load(lender, lender_page);
        assert(lender.num_children >= MIN_NUM_CHILDREN());

        // If the lender don't have enough keys we can't borrow from it 
        if (lender.num_children == MIN_NUM_CHILDREN())
        {
            return false;
        }

        u32 src_index;
        u32 dest_index;
        bp_tree_node<N> parent;

        auto find_parent_node_index = [](const bp_tree_node<N> &n, const key &key) {
            for (auto i = 0u; i < n.num_children; i++)
            {
                if (n.children[i].key >= key)
                    return i;
            }

            return n.num_children - 1;
        };

        /*
        RIGHT:

        Before:
                            [ 36 ]
            [ 18, 24, 30, 36 ] [ 42, 48, 54, 60, 66 ]

        After:
                                [ 42 ]
            [ 18, 24, 30, 36, 42 ] [ 48, 54, 60, 66, 72 ]
        
        LEFT:

        Before:
                                [ 36 ]
            [ 12, 18, 24, 30, 36 ] [ 42, 48, 54, 60, 66 ]

        After:
                           [ 30 ]
            [ 12, 18, 24, 30] [36, 42, 48, 54 ]
        */
        if (from_side == lender_side::right)
        {
            src_index = 0;
            dest_index = borrower.num_children;

            const auto has_same_parent = lender.parent_page == borrower.parent_page;
            if (!has_same_parent)
            {
                promote_larger_key(lender.children[src_index].key, node_page, borrower.parent_page);
            }

            load(parent, borrower.parent_page);
            const auto parent_key_index = find_parent_node_index(parent, borrower.children[borrower.num_children - 1].key);
            parent.children[parent_key_index].key = lender.children[0].key;
            save(parent, borrower.parent_page);
        }
        else
        {
            src_index = lender.num_children - 1;
            dest_index = 0;

            const auto has_same_parent = lender.parent_page == borrower.parent_page;
            if (!has_same_parent)
            {
                promote_smaller_key(lender.children[src_index - 1].key, lender_page, lender.parent_page);
            }

            load(parent, lender.parent_page);
            const auto parent_key_index = find_insert_index(parent, lender.children[0].key);
            parent.children[parent_key_index].key = lender.children[src_index - 1].key;
            save(parent, lender.parent_page);
        }

        auto& src = lender.children[src_index];
        insert_node_at(borrower, src.key, src.page, dest_index);

        // Change the borrowed node's parent
        bp_tree_node<N> child;
        load(child, lender.children[src_index].page);
        child.parent_page = node_page;
        save(child, lender.children[src_index].page);

        // Remove the borrowed key from the lender
        remove_key_at(lender, src_index);
        save(lender, lender_page);

        return true;
    }

    template<u32 N>
    void bp_tree<N>::insert_node_at(bp_tree_node<N>& node, const key &key, page_index page, u32 index)
    {
        assert(index < (node.num_children + 1));

        // Use a signed index to prevent i from wrapping on 0--
        for (auto i = static_cast<int32_t>(node.num_children) - 1; i >= static_cast<int32_t>(index); i--)
        {
            node.children[i + 1] = node.children[i];
        }

        node.children[index].key = key;
        node.children[index].page = page;
        node.num_children++;
    }

    template<u32 N>
    merge_result bp_tree<N>::merge_node(bp_tree_node<N> &node, page_index node_page, bool is_last)
    {
        merge_result result = { 0 };

        if (is_last)
        {
            // Node has no right neighbour, merge with prev
            assert(node.prev_page != 0);

            result.parent_page = node.parent_page;
            result.page_to_delete = node_page;

            bp_tree_node<N> prev;
            load(prev, node.prev_page);

            set_parent_ptr(node.children, node.num_children, node.prev_page);
            merge_nodes(prev, node);
            remove(prev, node);
            save(prev, node.prev_page);
        }
        else
        {
            // Merge with next
            assert(node.next_page != 0);

            bp_tree_node<N> next;
            load(next, node.next_page);

            result.parent_page = next.parent_page;
            result.page_to_delete = node.next_page;

            const auto has_same_parent = node.parent_page == next.parent_page;
            if (!has_same_parent)
            {
                bp_tree_node<N> next_parent;
                load(next_parent, next.parent_page);
                promote_larger_key(next_parent.children[0].key, node_page, node.parent_page);
            }

            set_parent_ptr(next.children, next.num_children, node_page);
            merge_nodes(node, next);
            remove(node, next);
            save(node, node_page);
        }

        return result;
    }

    template<u32 N>
    void bp_tree<N>::merge_nodes(bp_tree_node<N> &first, bp_tree_node<N> &second)
    {
        auto total_num_children = first.num_children + second.num_children;
        assert(total_num_children <= MAX_NUM_CHILDREN());

        for (auto i = first.num_children; i < total_num_children; i++)
        {
            first.children[i] = second.children[i - first.num_children];
        }

        first.num_children = total_num_children;
        second.num_children = 0;
    }

    template<u32 N>
    void bp_tree<N>::change_parent(page_index parent_page, const key &old_key, const key &new_key)
    {
        /*
            Examples:
            old_key = 0, new_key = 5
            
            Parent: [ 6, 12, 18, ... ]
            child.key == 6, is_last_child == false
            child.key will be set to 5
            Parent: [ 5, 12, 18, ... ]

            ------------------------------------------------------

            old_key = 0, new_key = 7

            Parent: [ 6, 12, 18, ... ]
            child.key == 6, is_last_child == false
            child.key will be set to 7
            Parent: [ 7, 12, 18, ... ]
        */

        assert(parent_page != 0);
        bp_tree_node<N> parent;
        load(parent, parent_page);
        auto& child = find_node_child(parent, old_key);
        assert(child.key > old_key);
        const auto is_last_child = child.key == parent.children[parent.num_children - 1].key;

        child.key = new_key;
        save(parent, parent_page);

        if (is_last_child && parent.parent_page)
        {
            // Traverse up the tree if we change the last key
            change_parent(parent.parent_page, old_key, new_key);
        }
    }

    template<u32 N>
    bool bp_tree<N>::borrow_key(bp_tree_leaf<N> &borrower)
    {
        auto could_borrow = borrow_key(lender_side::left, borrower);
        if (could_borrow)
            return true;

        return borrow_key(lender_side::right, borrower);
    }

    template<u32 N>
    bool bp_tree<N>::borrow_key(lender_side from_side, bp_tree_leaf<N> &borrower)
    {
        /*
        Left example
        Start:
                        [ 6, 12, ... ]
            [ ..., 3, 4, 5]    [ 6, 7, 8, ... ]
        Result:
                        [ 5, 12, ... ]
            [ ..., 3, 4 ]    [ 5, 6, 7, 8, ... ]

        ------------------------------------------------------

        Right example
        Start:
                        [ 6, 12, ... ]
            [ ..., 3, 4, 5]    [ 6, 7, 8, ... ]
        Result:
                        [ 7, 12, ... ]
            [ ..., 4, 5, 6]    [ 7, 8, ... ]
        */

        assert(borrower.num_children < MIN_NUM_CHILDREN());
        const auto lender_page = from_side == lender_side::right ? borrower.next_page : borrower.prev_page;
        
        if (lender_page == 0)
        {
            // The borrower has no left/right neighbour
            return false;
        }

        bp_tree_leaf<N> lender;
        load(lender, lender_page);
        assert(lender.num_children >= MIN_NUM_CHILDREN());

        // If the lender don't have enough keys we can't borrow from it 
        if (lender.num_children == MIN_NUM_CHILDREN())
        {
            return false;
        }

        u32 src_index;
        u32 dest_index;

        if (from_side == lender_side::right)
        {
            src_index = 0;
            dest_index = borrower.num_children;
            change_parent(borrower.parent_page, borrower.children[0].key, lender.children[1].key);
        }
        else
        {
            src_index = lender.num_children - 1;
            dest_index = 0;
            change_parent(lender.parent_page, lender.children[0].key, lender.children[src_index].key);
        }

        auto& src = lender.children[src_index];
        insert_record_at(borrower, src.key, src.value, dest_index);

        remove_record_at(lender, src_index);
        save(lender, lender_page);

        return true;
    }

    template<u32 N>
    merge_result bp_tree<N>::merge_leaf(bp_tree_leaf<N> &leaf, page_index leaf_page, bool is_last)
    {
        merge_result result = { 0 };

        if (is_last)
        {
            // Leaf has no right neighbour, merge with prev
            assert(leaf.prev_page != 0);
            
            result.parent_page = leaf.parent_page;
            result.page_to_delete = leaf_page;

            bp_tree_leaf<N> prev;
            load(prev, leaf.prev_page);

            merge_leafs(prev, leaf);
            remove(prev, leaf);
            save(prev, leaf.prev_page);
        }
        else
        {
            // Leaf has a right neighbour, merge with next
            assert(leaf.next_page != 0);

            bp_tree_leaf<N> next;
            load(next, leaf.next_page);

            result.parent_page = next.parent_page;
            result.page_to_delete = leaf.next_page;

            const auto has_same_parent = leaf.parent_page == next.parent_page;
            if (!has_same_parent)
            {
                bp_tree_node<N> next_parent;
                load(next_parent, next.parent_page);
                promote_larger_key(next_parent.children[0].key, leaf_page, leaf.parent_page);
            }

            merge_leafs(leaf, next);
            remove(leaf, next);
            save(leaf, leaf_page);
        }

        return result;
    }

    template<u32 N>
    void bp_tree<N>::merge_leafs(bp_tree_leaf<N> &first, bp_tree_leaf<N> &second)
    {
        const auto total_num_records = first.num_children + second.num_children;
        assert(total_num_records <= MAX_NUM_CHILDREN());

        for (auto i = first.num_children; i < total_num_records; i++)
        {
            first.children[i] = second.children[i - first.num_children];
        }

        first.num_children = total_num_records;
        second.num_children = 0;
    }

    template<u32 N>
    void bp_tree<N>::transfer_children(bp_tree_node<N> &source, bp_tree_node<N> &target, u32 from_index)
    {
        assert(from_index < source.num_children);

        for (auto i = from_index; i < source.num_children; i++)
        {
            target.children[i - from_index] = source.children[i];
            target.num_children++;
            source.children[i] = { 0 };
        }

        source.num_children = from_index;
    }

    template<u32 N>
    void bp_tree<N>::insert_record_non_full(bp_tree_leaf<N> &leaf, const key &key, const value &value)
    {
        assert((leaf.num_children + 1) <= header_.order);
        auto dest_index = find_insert_index(leaf, key);
        insert_record_at(leaf, key, value, dest_index);
    }

    template<u32 N>
    void bp_tree<N>::insert_record_at(bp_tree_leaf<N> &leaf, const key &key, const value &value, u32 index)
    {
        assert(index < (leaf.num_children + 1));

        // Use a signed index to prevent i from wrapping on 0--
        for (auto i = static_cast<int32_t>(leaf.num_children) - 1; i >= static_cast<int32_t>(index); i--)
        {
            leaf.children[i + 1] = leaf.children[i];
        }

        leaf.children[index].key = key;
        leaf.children[index].value = value;
        leaf.num_children++;
    }

    template<u32 N>
    void bp_tree<N>::insert_record_split(const key& key, const value &value, page_index leaf_page, bp_tree_leaf<N> &leaf, bp_tree_leaf<N> &new_leaf)
    {
        assert(leaf.num_children == header_.order);

        auto new_leaf_page = create(leaf_page, leaf, new_leaf, [this](auto l) { return alloc_leaf(l); });

        bool key_greater_than_key_at_split;
        u32 split_index;
        std::tie(key_greater_than_key_at_split, split_index) = find_split_index(leaf.children, leaf.num_children, key);

        transfer_records(leaf, new_leaf, split_index);

        if (key_greater_than_key_at_split)
        {
            insert_record_non_full(new_leaf, key, value);
        }
        else
        {
            insert_record_non_full(leaf, key, value);
        }

        save(leaf, leaf_page);
        save(new_leaf, new_leaf_page);
    }

    template<u32 N>
    void bp_tree<N>::transfer_records(bp_tree_leaf<N> &source, bp_tree_leaf<N> &target, u32 from_index)
    {
        assert(from_index < source.num_children);

        for (auto i = from_index; i < source.num_children; i++)
        {
            target.children[i - from_index] = source.children[i];
            target.num_children++;
            source.children[i] = { 0 };
        }

        source.num_children = from_index;
    }

    template<u32 N>
    bool bp_tree<N>::remove_record(bp_tree_leaf<N> &source, const key &key)
    {
        auto remove_index = binary_search_record(source, key);
        if (remove_index < 0)
            return false;

        remove_record_at(source, static_cast<u32>(remove_index));
        return true;
    }

    template<u32 N>
    void bp_tree<N>::remove_record_at(bp_tree_leaf<N> &source, u32 index)
    {
        assert(index < source.num_children);

        for (auto i = index; i < source.num_children - 1; i++)
        {
            source.children[i] = source.children[i + 1];
        }

        source.num_children--;
    }

    template<u32 N>
    void bp_tree<N>::promote_larger_key(const key &key_to_promote, page_index node_page, page_index parent_page)
    {
        bp_tree_node<N> parent;
        load(parent, parent_page);

        bool set = false;

        for (auto i = 0u; i < parent.num_children; i++)
        {
            if (parent.children[i].page == node_page)
            {
                if (parent.children[i].key >= key_to_promote)
                {
                    return;
                }

                parent.children[i].key = key_to_promote;
                save(parent, parent_page);
                set = true;
                break;
            }
        }

        assert(set && "promote_key could not promote key");

        if (parent.parent_page)
        {
            promote_larger_key(key_to_promote, parent_page, parent.parent_page);
        }
    }

    template<u32 N>
    void bp_tree<N>::promote_smaller_key(const key & key_to_promote, page_index node_page, page_index parent_page)
    {
        bp_tree_node<N> parent;
        load(parent, parent_page);

        bool set = false;

        for (auto i = 0u; i < parent.num_children; i++)
        {
            if (parent.children[i].page == node_page)
            {
                if (parent.children[i].key <= key_to_promote)
                {
                    return;
                }

                parent.children[i].key = key_to_promote;
                save(parent, parent_page);
                set = true;

                if (parent.children[parent.num_children - 1].key > key_to_promote)
                {
                    /*
                        Stop the "key promotion" when we hit a node with keys larger than key_to_promote 
                        to prevent breaking nodes further up the tree.

                        Example when key_to_promote == 6517, keys larger than 6516 will be unreachable if we continue promoting:
                        [ 18256, 30710 ]
                        [ 6517, 12890, 18256]
                    */

                    return;
                }

                break;
            }
        }

        assert(set && "promote_key could not promote key");

        if (parent.parent_page)
        {
            promote_smaller_key(key_to_promote, parent_page, parent.parent_page);
        }
    }

    template<u32 N>
    template<class T>
    tuple<bool, u32> bp_tree<N>::find_split_index(const T *arr, u32 arr_len, const key & key)
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

    template<u32 N>
    page_index bp_tree<N>::search_tree(const key &key) const
    {
        page_index current_page = header_.root_page;
        auto height = header_.height;

        while (height > 1)
        {
            bp_tree_node<N> node;
            load(node, current_page);
            current_page = find_node_child(node, key).page;
            height -= 1;
        }

        return current_page;
    }

    template<u32 N>
    page_index bp_tree<N>::search_node(page_index page, const key &key) const
    {
        bp_tree_node<N> node;
        load(node, page);
        return find_node_child(node, key).page;
    }

    template<u32 N>
    page_index bp_tree<N>::search_node(bp_tree_node<N> &node, const key &key) const
    {
        return find_node_child(node, key).page;
    }

    template<u32 N>
    u32 bp_tree<N>::find_insert_index(const bp_tree_leaf<N> &leaf, const key &key) const
    {
        for (auto i = 0u; i < leaf.num_children; i++)
        {
            if (leaf.children[i].key > key)
            {
                return i;
            }
        }

        return leaf.num_children;
    }

    template<u32 N>
    u32 bp_tree<N>::find_insert_index(const bp_tree_node<N> &node, const key &key) const
    {
        assert(node.num_children > 0);

        for (auto i = 0u; i < node.num_children; i++)
        {
            if (node.children[i].key > key)
            {
                return i;
            }
        }

        return node.num_children - 1;
    }

    template<u32 N>
    bp_tree_node_child &bp_tree<N>::find_node_child(bp_tree_node<N> &node, const key &key) const
    {
        if (node.num_children == 0)
            return node.children[0];

        for (auto i = 0u; i < node.num_children; i++)
        {
            if (node.children[i].key > key)
            {
                return node.children[i];
            }
        }

        return node.children[node.num_children - 1];
    }

    template<u32 N>
    int64_t bp_tree<N>::binary_search_record(const bp_tree_leaf<N> &leaf, const key &key) const
    {
        if (leaf.num_children == 0)
            return -1;

        int64_t low = 0;
        int64_t high = static_cast<int64_t>(leaf.num_children - 1);

        while (low <= high)
        {
            auto mid = low + (high - low) / 2;
            auto current_key = leaf.children[mid].key;
            if (current_key == key)
                return mid;

            if (current_key < key)
            {
                low = mid + 1;
            }
            else
            {
                high = mid - 1;
            }
        }

        return -1;
    }

    template<u32 N>
    page_index bp_tree<N>::alloc_node(bp_tree_node<N> &node)
    {
        header_.num_internal_nodes++;
        return alloc(sizeof(bp_tree_node<N>));
    }

    template<u32 N>
    page_index bp_tree<N>::alloc_leaf(bp_tree_leaf<N> &leaf)
    {
        header_.num_leaf_nodes++;
        return alloc(sizeof(bp_tree_leaf<N>));
    }

    template<u32 N>
    page_index bp_tree<N>::alloc(u32 size)
    {
        assert(size <= PAGE_SIZE);
        return pager_->get_free_page().index;
    }

    template<u32 N>
    void bp_tree<N>::free(bp_tree_node<N> &node, page_index node_page)
    {
        header_.num_internal_nodes -= 1;
        free(sizeof(bp_tree_node<N>), node_page);
    }

    template<u32 N>
    void bp_tree<N>::free(bp_tree_leaf<N> &leaf, page_index leaf_page)
    {
        header_.num_leaf_nodes -= 1;
        free(sizeof(bp_tree_leaf<N>), leaf_page);
    }

    template<u32 N>
    void bp_tree<N>::free(u32 size, page_index page)
    {
        //TODO: Free this block in pager
    }

    template<u32 N>
    template<class T, class NodeAllocator>
    page_index bp_tree<N>::create(page_index node_page, T & node, T & new_node, NodeAllocator node_allocator)
    {
        static_assert(std::is_same<T, bp_tree_node<N>>::value || std::is_same<T, bp_tree_leaf<N>>::value, "T must be a node or a leaf");

        /*

        Result if node/leaf has a right neighbour:

                    Parent

        X <---> leaf <---> new_leaf <---> X

        */

        // Insert new_node to the right of node/leaf
        new_node.parent_page = node.parent_page;
        new_node.next_page = node.next_page;
        new_node.prev_page = node_page;
        node.next_page = node_allocator(new_node);

        if (new_node.next_page != 0)
        {
            T old_next;
            load(old_next, new_node.next_page);
            old_next.prev_page = node.next_page;
            save(old_next, new_node.next_page);
        }

        save(header_, HEADER_PAGE_INDEX);
        return node.next_page;
    }

    template<u32 N>
    void bp_tree<N>::print_node_level(stringstream &ss, page_index node_page) const
    {
        bp_tree_node<N> n;
        load(n, node_page);

        ss << "[PG:" << node_page << " P:" << n.parent_page << " PR:" << n.prev_page << " N:" << n.next_page << " {";

        for (auto i = 0u; i < n.num_children; i++)
        {
            ss << "{" << n.children[i].key << "," << n.children[i].page << "}";
            if (i != n.num_children - 1)
            {
                ss << ",";
            }
        }

        ss << "}]  ";

        if (n.next_page != 0)
        {
            print_node_level(ss, n.next_page);
        }
    }

    template<u32 N>
    void bp_tree<N>::print_leaf_level(stringstream &ss, page_index leaf_page) const
    {
        bp_tree_leaf<N> l;
        load(l, leaf_page);

        ss << "[PG:" << leaf_page << " P:" << l.parent_page << " PR:" << l.prev_page << " N:" << l.next_page << " {";

        for (auto i = 0u; i < l.num_children; i++)
        {
            ss << l.children[i].key;
            if (i != l.num_children - 1)
            {
                ss << ",";
            }
        }

        ss << "}]  ";

        if (l.next_page != 0)
        {
            print_leaf_level(ss, l.next_page);
        }
    }

    template<u32 N>
    template<class T>
    void bp_tree<N>::remove(T &prev, T &node)
    {
        static_assert(std::is_same<T, bp_tree_node<N>>::value || std::is_same<T, bp_tree_leaf<N>>::value, "T must be a node or a leaf");

        /*
            Start:
            X <--> prev <--> node <--> Y

            Result:
            X <--> prev <--> Y
        */

        free(node, prev.next_page);
        prev.next_page = node.next_page;
        if (node.next_page != 0) {
            // Point node's right neighbour's prev ptr to "prev"
            T next;
            load(next, node.next_page);
            next.prev_page = node.prev_page;
            save(next, node.next_page);
        }

        save(header_, HEADER_PAGE_INDEX);
    }

    template<u32 N>
    template<class T>
    void bp_tree<N>::load(T &t, page_index page) const
    {
        assert(sizeof(T) <= PAGE_SIZE);

        auto& page_to_load = pager_->get_page(page);

        if constexpr (std::is_same<T, bp_tree_node<N>>::value)
        {
            deserialize_bp_tree_node(page_to_load.content, t);
        }
        else if constexpr (std::is_same<T, bp_tree_leaf<N>>::value)
        {
            deserialize_bp_tree_leaf(page_to_load.content, t);
        }
        else if constexpr (std::is_same<T, bp_tree_header>::value)
        {
            deserialize_bp_tree_header(page_to_load.content, t);
        }
        else
        {
            static_assert(false, "bp_tree<N>::load: Unsupported type");
        }
    }

    template<u32 N>
    template<class T>
    void bp_tree<N>::save(const T &t, page_index page) const
    {
        assert(sizeof(T) <= PAGE_SIZE);

        auto& page_to_save = pager_->get_page(page);

        if constexpr (std::is_same<T, bp_tree_node<N>>::value)
        {
            serialize_bp_tree_node(page_to_save.content, t);
        }
        else if constexpr (std::is_same<T, bp_tree_leaf<N>>::value)
        {
            serialize_bp_tree_leaf(page_to_save.content, t);
        }
        else if constexpr (std::is_same<T, bp_tree_header>::value)
        {
            serialize_bp_tree_header(page_to_save.content, t);
        }
        else
        {
            static_assert(false, "bp_tree<N>::save: Unsupported type");
        }

        page_to_save.dirty = true;
    }

    template class bp_tree<4>;
    template class bp_tree<6>;
    template class bp_tree<10>;
    template class bp_tree<DEFAULT_TREE_ORDER>;
}