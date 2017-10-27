#include "bp_tree.h"

#include <assert.h>

namespace niffler {

    using std::tie;
    using std::stringstream;

    constexpr size_t BASE_OFFSET_HEADER_BLOCK = 0;
    constexpr size_t BASE_OFFSET_DATA_BLOCK = BASE_OFFSET_HEADER_BLOCK + sizeof(bp_tree_header);

    template<size_t N>
    result<bp_tree<N>> bp_tree<N>::load(std::unique_ptr<storage_provider> storage)
    {
        return false;
    }

    template<size_t N>
    result<bp_tree<N>> bp_tree<N>::create(std::unique_ptr<storage_provider> storage)
    {
        auto t = std::unique_ptr<bp_tree<N>>(new bp_tree<N>(std::move(storage)));

        t->header_.order = N;
        t->header_.value_size = sizeof(value);
        t->header_.key_size = sizeof(key);
        t->header_.height = 1;
        t->header_.next_slot_offset = BASE_OFFSET_DATA_BLOCK;

        bp_tree_node<N> root;
        t->header_.root_offset = t->alloc_node(root);

        bp_tree_leaf<N> leaf;
        leaf.parent = t->header_.root_offset;

        auto leaf_offset = t->alloc_leaf(leaf);
        t->header_.leaf_offset = leaf_offset;
        root.children[0].offset = leaf_offset;
        root.num_children = 1;

        // Save inital tree to the underlying storage
        t->save(&t->header_, BASE_OFFSET_HEADER_BLOCK);
        t->save(&root, t->header_.root_offset);
        t->save(&leaf, leaf_offset);

        return result<bp_tree<N>>(true, std::move(t));
    }

    template<size_t N>
    const bp_tree_header &bp_tree<N>::header() const
    {
        return header_;
    }

    template<size_t N>
    string bp_tree<N>::print() const
    {
        stringstream ss;
        auto height = header_.height;
        auto current_offset = header_.root_offset;

        do
        {
            bp_tree_node<N> n;
            load(&n, current_offset);
            print_node_level(ss, current_offset);
            ss << std::endl;

            current_offset = n.children[0].offset;
            height--;
        } while (height > 0);

        print_leaf_level(ss, current_offset);

        return ss.str();
    }

    template<size_t N>
    bool bp_tree<N>::insert(const key &key, const value &value)
    {
        auto parent_offset = search_tree(key);
        assert(parent_offset != 0);

        auto leaf_offset = search_node(parent_offset, key);
        assert(leaf_offset != 0);

        bp_tree_leaf<N> leaf;
        load(&leaf, leaf_offset);

        // Key already exists
        if (binary_search_record(leaf, key) >= 0)
            return false;

        if (leaf.num_children == header_.order)
        {
            bp_tree_leaf<N> new_leaf;
            insert_record_split(key, value, leaf_offset, leaf, new_leaf);
            insert_key(parent_offset, new_leaf.children[0].key, leaf_offset, leaf.next);
        }
        else
        {
            insert_record_non_full(leaf, key, value);
            save(&leaf, leaf_offset);
        }

        return true;
    }

    template<size_t N>
    bool bp_tree<N>::remove(const key &key)
    {
        auto parent_offset = search_tree(key);
        assert(parent_offset != 0);

        bp_tree_node<N> parent;
        load(&parent, parent_offset);

        auto leaf_offset = search_node(parent, key);
        assert(leaf_offset != 0);

        bp_tree_leaf<N> leaf;
        load(&leaf, leaf_offset);

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
                niffler::key index_key_to_remove;
                merge_leaf(leaf, leaf_offset, leaf.next == 0, index_key_to_remove);
                remove_key(parent_offset, parent, index_key_to_remove);
            }
            else
            {
                save(&leaf, leaf_offset);
            }
        }
        else
        {
            save(&leaf, leaf_offset);
        }

        return true;
    }

    template<size_t N>
    bp_tree<N>::bp_tree(std::unique_ptr<storage_provider> storage)
        :
        storage_(std::move(storage))
    {
    }

    template<size_t N>
    void bp_tree<N>::insert_key(offset node_offset, const key &key, offset left_offset, offset right_offset)
    {
        // We have reached the root of the tree so we create a new root node
        if (node_offset == 0)
        {
            bp_tree_node<N> root;
            header_.root_offset = alloc_node(root);
            header_.height++;

            root.num_children = 2;
            root.children[0].key = key;
            root.children[0].offset = left_offset;
            root.children[1].offset = right_offset;

            save(&header_, BASE_OFFSET_HEADER_BLOCK);
            save(&root, header_.root_offset);

            set_parent_ptr(root.children, root.num_children, header_.root_offset);

            return;
        }

        bp_tree_node<N> node;
        load(&node, node_offset);

        if (node.num_children == header_.order)
        {
            bp_tree_node<N> new_node;
            auto new_node_offset = create(node_offset, node, new_node, [this](auto n) { return alloc_node(n); });

            bool key_greater_than_key_at_split;
            size_t split_index;
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
                insert_key_non_full(new_node, key, right_offset);
            }
            else
            {
                insert_key_non_full(node, key, right_offset);
            }

            save(&node, node_offset);
            save(&new_node, new_node_offset);
            set_parent_ptr(new_node.children, new_node.num_children, new_node_offset);

            // Insert the middle key into the parent node
            insert_key(node.parent, middle_key, node_offset, new_node_offset);
        }
        else
        {
            insert_key_non_full(node, key, right_offset);
            save(&node, node_offset);
        }
    }

    template<size_t N>
    void bp_tree<N>::insert_key_non_full(bp_tree_node<N> &node, const key &key, offset next_offset)
    {
        auto dest_index = find_insert_index(node, key);
        insert_key_at(node, key, next_offset, dest_index);
    }

    template<size_t N>
    void bp_tree<N>::insert_key_at(bp_tree_node<N> &node, const key &key, offset next_offset, size_t index)
    {
        assert(index < (node.num_children + 1));

        // Inserting at the end
        if (index == node.num_children)
        {
            node.children[index].key = key;
            node.children[index].offset = next_offset;
        }
        else
        {
            // Use a signed index to prevent i from wrapping on 0--
            for (auto i = static_cast<int32_t>(node.num_children) - 1; i >= static_cast<int32_t>(index); i--)
            {
                node.children[i + 1] = node.children[i];
            }

            node.children[index].key = key;

            // Point the newly inserted key to the offset of the previous key at this index and
            // point the key to the right(previous key) of the new key to the next_offset
            node.children[index].offset = node.children[index + 1].offset;
            node.children[index + 1].offset = next_offset;
        }

        node.num_children++;
    }

    template<size_t N>
    void bp_tree<N>::remove_key_at(bp_tree_node<N>& source, size_t index)
    {
        assert(index < source.num_children);
        assert(source.num_children > 0);

        for (auto i = index; i < source.num_children - 1; i++)
        {
            source.children[i] = source.children[i + 1];
        }

        source.num_children--;
    }

    template<size_t N>
    void bp_tree<N>::set_parent_ptr(bp_tree_node_child *children, size_t c_length, offset parent)
    {
        bp_tree_node<N> node;
        for (auto i = 0u; i < c_length; i++)
        {
            load(&node, children[i].offset);
            node.parent = parent;
            save(&node, children[i].offset);
        }
    }

    template<size_t N>
    void bp_tree<N>::remove_key(offset node_offset, bp_tree_node<N> &node, const key &key)
    {
        const auto min_num_children = node.parent == 0 ? 1 : MIN_NUM_CHILDREN();
        auto index_key = node.children[0].key;

        const auto delete_index = find_insert_index(node, key);
        assert(delete_index <= (node.num_children - 1));

        // Remove if it's not the last child
        if (delete_index != (node.num_children - 1)) {
            /*
            After child merge(invalid tree):
                                            [ 7, 12, 18 ]
                [ 3, 4, 5, 6, 7, 8, 9 ] [ 12, 13, 14, 15, 16, 17 ] [ X, X, X, ... ]

            After key delete(valid tree):
                                            [ 12, 18, ... ]
                [ 3, 4, 5, 6, 7, 8, 9 ] [ 12, 13, 14, 15, 16, 17 ] [ X, X, X, ... ]
            */

            // Give the child of the deleted key to the next key
            node.children[delete_index + 1].offset = node.children[delete_index].offset;
            remove_key_at(node, delete_index);
        }
        else
        {
            node.num_children -= 1;
        }

        // If we only have one child left we make that child the new root and free the old root
        if (node.num_children == 1 && header_.root_offset == node_offset && header_.num_internal_nodes != 1)
        {
            free(node, header_.root_offset);
            header_.height--;
            header_.root_offset = node.children[0].offset;
            save(&header_, BASE_OFFSET_HEADER_BLOCK);

            bp_tree_node<N> root;
            load(&root, header().root_offset);
            root.parent = 0;
            save(&root, header().root_offset);
            return;
        }

        if (node.num_children < min_num_children)
        {
            auto could_borrow = borrow_key(node, node_offset);

            if (!could_borrow)
            {
                bp_tree_node<N> parent;
                load(&parent, node.parent);
                merge_node(node, node_offset, node_offset == parent.children[parent.num_children - 1].offset);
                remove_key(node.parent, parent, index_key);
            }
            else
            {
                save(&node, node_offset);
            }
        }
        else
        {
            save(&node, node_offset);
        }
    }

    template<size_t N>
    bool bp_tree<N>::borrow_key(bp_tree_node<N> &borrower, offset node_offset)
    {
        auto could_borrow = borrow_key(lender_side::left, borrower, node_offset);
        if (could_borrow)
            return true;

        return borrow_key(lender_side::right, borrower, node_offset);
    }

    template<size_t N>
    bool bp_tree<N>::borrow_key(lender_side from_side, bp_tree_node<N> &borrower, offset node_offset)
    {
        assert(borrower.num_children < MIN_NUM_CHILDREN());
        const auto lender_offset = from_side == lender_side::right ? borrower.next : borrower.prev;

        if (lender_offset == 0)
        {
            // The borrower has no left/right neighbour
            return false;
        }

        bp_tree_node<N> lender;
        load(&lender, lender_offset);
        assert(lender.num_children >= MIN_NUM_CHILDREN());

        // If the lender don't have enough keys we can't borrow from it 
        if (lender.num_children == MIN_NUM_CHILDREN())
        {
            return false;
        }

        size_t src_index;
        size_t dest_index;
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
            load(&parent, borrower.parent);
            const auto parent_key_index = find_parent_node_index(parent, borrower.children[borrower.num_children - 1].key);
            parent.children[parent_key_index].key = lender.children[0].key;
            save(&parent, borrower.parent);
        }
        else
        {
            src_index = lender.num_children - 1;
            dest_index = 0;
            load(&parent, lender.parent);
            const auto parent_key_index = find_insert_index(parent, lender.children[0].key);
            parent.children[parent_key_index].key = lender.children[src_index - 1].key;
            save(&parent, borrower.parent);
        }

        auto& src = lender.children[src_index];
        insert_node_at(borrower, src.key, src.offset, dest_index);

        // Change the borrowed node's parent
        bp_tree_node<N> child;
        load(&child, lender.children[src_index].offset);
        child.parent = node_offset;
        save(&child, lender.children[src_index].offset);

        // Remove the borrowed key from the lender
        remove_key_at(lender, src_index);
        save(&lender, lender_offset);

        return true;
    }

    template<size_t N>
    void bp_tree<N>::insert_node_at(bp_tree_node<N>& node, const key & key, offset offset, size_t index)
    {
        assert(index < (node.num_children + 1));

        // Use a signed index to prevent i from wrapping on 0--
        for (auto i = static_cast<int32_t>(node.num_children) - 1; i >= static_cast<int32_t>(index); i--)
        {
            node.children[i + 1] = node.children[i];
        }

        node.children[index].key = key;
        node.children[index].offset = offset;
        node.num_children++;
    }

    template<size_t N>
    void bp_tree<N>::merge_node(bp_tree_node<N> &node, offset node_offset, bool is_last)
    {
        if (is_last)
        {
            // Node has no right neighbour, merge with prev
            assert(node.prev != 0);
            bp_tree_node<N> prev;
            load(&prev, node.prev);

            set_parent_ptr(node.children, node.num_children, node.prev);
            merge_nodes(prev, node);
            remove(prev, node);
            save(&prev, node.prev);
        }
        else
        {
            // Merge with next
            assert(node.next != 0);
            bp_tree_node<N> next;
            load(&next, node.next);

            set_parent_ptr(next.children, next.num_children, node_offset);
            merge_nodes(node, next);
            remove(node, next);
            save(&node, node_offset);
        }
    }

    template<size_t N>
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

    template<size_t N>
    void bp_tree<N>::change_parent(offset parent_offset, const key &old_key, const key &new_key)
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

        assert(parent_offset != 0);
        bp_tree_node<N> parent;
        load(&parent, parent_offset);
        auto& child = find_node_child(parent, old_key);
        assert(child.key > old_key);
        const auto is_last_child = child.key == parent.children[parent.num_children - 1].key;

        child.key = new_key;
        save(&parent, parent_offset);

        if (is_last_child)
        {
            // Traverse up the tree if we change the last key
            change_parent(parent.parent, old_key, new_key);
        }
    }

    template<size_t N>
    bool bp_tree<N>::borrow_key(bp_tree_leaf<N> &borrower)
    {
        auto could_borrow = borrow_key(lender_side::left, borrower);
        if (could_borrow)
            return true;

        return borrow_key(lender_side::right, borrower);
    }

    template<size_t N>
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
        const auto lender_offset = from_side == lender_side::right ? borrower.next : borrower.prev;
        
        if (lender_offset == 0)
        {
            // The borrower has no left/right neighbour
            return false;
        }

        bp_tree_leaf<N> lender;
        load(&lender, lender_offset);
        assert(lender.num_children >= MIN_NUM_CHILDREN());

        // If the lender don't have enough keys we can't borrow from it 
        if (lender.num_children == MIN_NUM_CHILDREN())
        {
            return false;
        }

        size_t src_index;
        size_t dest_index;

        if (from_side == lender_side::right)
        {
            src_index = 0;
            dest_index = borrower.num_children;
            change_parent(borrower.parent, borrower.children[0].key, lender.children[1].key);
        }
        else
        {
            src_index = lender.num_children - 1;
            dest_index = 0;
            change_parent(lender.parent, lender.children[0].key, lender.children[src_index].key);
        }

        auto& src = lender.children[src_index];
        insert_record_at(borrower, src.key, src.value, dest_index);

        remove_record_at(lender, src_index);
        save(&lender, lender_offset);

        return true;
    }

    template<size_t N>
    void bp_tree<N>::merge_leaf(bp_tree_leaf<N> &leaf, offset leaf_offset, bool is_last, key &index_key_to_remove)
    {
        if (is_last)
        {
            // Leaf has no right neighbour, merge with prev
            assert(leaf.prev != 0);
            bp_tree_leaf<N> prev;
            load(&prev, leaf.prev);
            index_key_to_remove = prev.children[0].key;

            merge_leafs(prev, leaf);
            remove(prev, leaf);
            save(&prev, leaf.prev);
        }
        else
        {
            // Leaf has a right neighbour, merge with next
            assert(leaf.next != 0);
            bp_tree_leaf<N> next;
            load(&next, leaf.next);
            index_key_to_remove = leaf.children[0].key;

            merge_leafs(leaf, next);
            remove(leaf, next);
            save(&leaf, leaf_offset);
        }
    }

    template<size_t N>
    void bp_tree<N>::merge_leafs(bp_tree_leaf<N> &first, bp_tree_leaf<N> &second)
    {
        auto total_num_records = first.num_children + second.num_children;
        assert(total_num_records <= MAX_NUM_CHILDREN());

        for (auto i = first.num_children; i < total_num_records; i++)
        {
            first.children[i] = second.children[i - first.num_children];
        }

        first.num_children = total_num_records;
        second.num_children = 0;
    }

    template<size_t N>
    void bp_tree<N>::transfer_children(bp_tree_node<N> &source, bp_tree_node<N> &target, size_t from_index)
    {
        assert(from_index < source.num_children);

        for (auto i = from_index; i < source.num_children; i++)
        {
            target.children[i - from_index] = source.children[i];
            target.num_children++;
            // Not really needed, might remove
            source.children[i] = { 0 };
        }

        source.num_children = from_index;
    }

    template<size_t N>
    void bp_tree<N>::insert_record_non_full(bp_tree_leaf<N> &leaf, const key &key, const value &value)
    {
        assert((leaf.num_children + 1) <= header_.order);
        auto dest_index = find_insert_index(leaf, key);
        insert_record_at(leaf, key, value, dest_index);
    }

    template<size_t N>
    void bp_tree<N>::insert_record_at(bp_tree_leaf<N> &leaf, const key &key, const value &value, size_t index)
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

    template<size_t N>
    void bp_tree<N>::insert_record_split(const key& key, const value &value, offset leaf_offset, bp_tree_leaf<N> &leaf, bp_tree_leaf<N> &new_leaf)
    {
        assert(leaf.num_children == header_.order);

        auto new_leaf_offset = create(leaf_offset, leaf, new_leaf, [this](auto l) { return alloc_leaf(l); });

        bool key_greater_than_key_at_split;
        size_t split_index;
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

        save(&leaf, leaf_offset);
        save(&new_leaf, new_leaf_offset);
    }

    template<size_t N>
    void bp_tree<N>::transfer_records(bp_tree_leaf<N> &source, bp_tree_leaf<N> &target, size_t from_index)
    {
        assert(from_index < source.num_children);

        for (auto i = from_index; i < source.num_children; i++)
        {
            target.children[i - from_index] = source.children[i];
            target.num_children++;
            // Not really needed, might remove
            source.children[i] = { 0 };
        }

        source.num_children = from_index;
    }

    template<size_t N>
    bool bp_tree<N>::remove_record(bp_tree_leaf<N> &source, const key &key)
    {
        auto remove_index = binary_search_record(source, key);
        if (remove_index < 0)
            return false;

        remove_record_at(source, static_cast<size_t>(remove_index));
        return true;
    }

    template<size_t N>
    void bp_tree<N>::remove_record_at(bp_tree_leaf<N> &source, size_t index)
    {
        assert(index < source.num_children);

        for (auto i = index; i < source.num_children - 1; i++)
        {
            source.children[i] = source.children[i + 1];
        }

        source.num_children--;
    }

    template<size_t N>
    template<class T>
    tuple<bool, size_t> bp_tree<N>::find_split_index(const T *arr, size_t arr_len, const key & key)
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

    template<size_t N>
    offset bp_tree<N>::search_tree(const key &key) const
    {
        offset current_offset = header_.root_offset;
        auto height = header_.height;

        while (height > 1)
        {
            bp_tree_node<N> node;
            load(&node, current_offset);
            current_offset = find_node_child(node, key).offset;
            height -= 1;
        }

        return current_offset;
    }

    template<size_t N>
    offset bp_tree<N>::search_node(offset offset, const key &key) const
    {
        bp_tree_node<N> node;
        load(&node, offset);
        return find_node_child(node, key).offset;
    }

    template<size_t N>
    offset bp_tree<N>::search_node(bp_tree_node<N> &node, const key &key) const
    {
        return find_node_child(node, key).offset;
    }

    template<size_t N>
    size_t bp_tree<N>::find_insert_index(const bp_tree_leaf<N> &leaf, const key& key) const
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

    template<size_t N>
    size_t bp_tree<N>::find_insert_index(const bp_tree_node<N> &node, const key &key) const
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

    template<size_t N>
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

    template<size_t N>
    int64_t bp_tree<N>::binary_search_record(const bp_tree_leaf<N> &leaf, const key &key)
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

    template<size_t N>
    offset bp_tree<N>::alloc_node(bp_tree_node<N> &node)
    {
        header_.num_internal_nodes++;
        return alloc(sizeof(bp_tree_node<N>));
    }

    template<size_t N>
    offset bp_tree<N>::alloc_leaf(bp_tree_leaf<N> &leaf)
    {
        header_.num_leaf_nodes++;
        return alloc(sizeof(bp_tree_leaf<N>));
    }

    template<size_t N>
    offset bp_tree<N>::alloc(size_t size)
    {
        auto current_offset = header_.next_slot_offset;
        header_.next_slot_offset += size;
        return current_offset;
    }

    template<size_t N>
    void bp_tree<N>::free(bp_tree_node<N> &node, offset node_offset)
    {
        header_.num_internal_nodes -= 1;
        free(sizeof(bp_tree_node<N>), node_offset);
    }

    template<size_t N>
    void bp_tree<N>::free(bp_tree_leaf<N> &leaf, offset leaf_offset)
    {
        header_.num_leaf_nodes -= 1;
        free(sizeof(bp_tree_leaf<N>), leaf_offset);
    }

    template<size_t N>
    void bp_tree<N>::free(size_t size, offset offset)
    {
        //TODO: Free this block in storage provider
    }

    template<size_t N>
    template<class T, class NodeAllocator>
    offset bp_tree<N>::create(offset node_offset, T & node, T & new_node, NodeAllocator node_allocator)
    {
        static_assert(std::is_same<T, bp_tree_node<N>>::value || std::is_same<T, bp_tree_leaf<N>>::value, "T must be a node or a leaf");

        /*

        Result if node/leaf has a right neighbour:

                    Parent

        X <---> leaf <---> new_leaf <---> X

        */

        // Insert new_node to the right of node/leaf
        new_node.parent = node.parent;
        new_node.next = node.next;
        new_node.prev = node_offset;
        node.next = node_allocator(new_node);

        if (new_node.next != 0)
        {
            T old_next;
            load(&old_next, new_node.next);
            old_next.prev = node.next;
            save(&old_next, new_node.next);
        }

        save(&header_, BASE_OFFSET_HEADER_BLOCK);
        return node.next;
    }

    template<size_t N>
    void bp_tree<N>::print_node_level(stringstream &ss, offset node_offset) const
    {
        bp_tree_node<N> n;
        load(&n, node_offset);

        ss << "[";

        for (auto i = 0u; i < n.num_children; i++)
        {
            ss << n.children[i].key;
            if (i != n.num_children - 1)
            {
                ss << ",";
            }
        }

        ss << "]";

        if (n.next != 0)
        {
            print_node_level(ss, n.next);
        }
    }

    template<size_t N>
    void bp_tree<N>::print_leaf_level(stringstream &ss, offset leaf_offset) const
    {
        bp_tree_leaf<N> l;
        load(&l, leaf_offset);

        ss << "[";

        for (auto i = 0u; i < l.num_children; i++)
        {
            ss << l.children[i].key;
            if (i != l.num_children - 1)
            {
                ss << ",";
            }
        }

        ss << "]";

        if (l.next != 0)
        {
            print_leaf_level(ss, l.next);
        }
    }

    template<size_t N>
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

        free(node, prev.next);
        prev.next = node.next;
        if (node.next != 0) {
            // Point node's right neighbour's prev ptr to "prev"
            T next;
            load(&next, node.next);
            next.prev = node.prev;
            save(&next, node.next);
        }

        save(&header_, BASE_OFFSET_HEADER_BLOCK);
    }

    template<size_t N>
    template<class T>
    void bp_tree<N>::load(T *buffer, offset offset) const
    {
        static_assert(std::is_same<T, bp_tree_node<N>>::value || std::is_same<T, bp_tree_leaf<N>>::value || std::is_same<T, bp_tree_header>::value, "T must be a node, leaf or header");
        storage_->load(buffer, offset, sizeof(T));
    }

    template<size_t N>
    template<class T>
    void bp_tree<N>::save(T *value, offset offset) const
    {
        static_assert(std::is_same<T, bp_tree_node<N>>::value || std::is_same<T, bp_tree_leaf<N>>::value || std::is_same<T, bp_tree_header>::value, "T must be a node, leaf or header");
        storage_->store(value, offset, sizeof(T));
    }

    template class bp_tree<10>;
}
