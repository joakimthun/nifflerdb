#include "bp_tree.h"

#include <assert.h>

namespace niffler {

    using std::tie;
    using std::stringstream;

    constexpr size_t BASE_OFFSET_INFO_BLOCK = 0;
    constexpr size_t BASE_OFFSET_DATA_BLOCK = BASE_OFFSET_INFO_BLOCK + sizeof(bp_tree_info);

    template<size_t N>
    result<bp_tree<N>> bp_tree<N>::load(std::unique_ptr<storage_provider> storage)
    {
        return false;
    }

    template<size_t N>
    result<bp_tree<N>> bp_tree<N>::create(std::unique_ptr<storage_provider> storage)
    {
        auto t = std::unique_ptr<bp_tree<N>>(new bp_tree<N>(std::move(storage)));

        t->info_.order = N;
        t->info_.value_size = sizeof(value);
        t->info_.key_size = sizeof(key);
        t->info_.height = 1;
        t->info_.next_slot_offset = BASE_OFFSET_DATA_BLOCK;

        bp_tree_node<N> root;
        t->info_.root_offset = t->alloc_node(root);

        bp_tree_leaf<N> leaf;
        leaf.parent = t->info_.root_offset;

        auto leaf_offset = t->alloc_leaf(leaf);
        t->info_.leaf_offset = leaf_offset;
        root.children[0].offset = leaf_offset;
        root.num_children = 1;

        // Save inital tree to the underlying storage
        t->save(&t->info_, BASE_OFFSET_INFO_BLOCK);
        t->save(&root, t->info_.root_offset);
        t->save(&leaf, leaf_offset);

        return result<bp_tree<N>>(true, std::move(t));
    }

    template<size_t N>
    const bp_tree_info &bp_tree<N>::info() const
    {
        return info_;
    }

    template<size_t N>
    string bp_tree<N>::print() const
    {
        stringstream ss;
        auto height = info_.height;
        auto current_offset = info_.root_offset;

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

        if (leaf.num_records == info_.order)
        {
            bp_tree_leaf<N> new_leaf;
            insert_record_split(key, value, leaf_offset, leaf, new_leaf);
            insert_key(parent_offset, new_leaf.records[0].key, leaf_offset, leaf.next);
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

        auto leaf_offset = search_node(parent_offset, key);
        assert(leaf_offset != 0);

        bp_tree_leaf<N> leaf;
        load(&leaf, leaf_offset);

        // Return false if the key does not exists
        if (!remove_record(leaf, key))
            return false;

        // If this is the only leaf we cant really borrow/merge so we accept any number of records
        const auto min_num_records = info_.num_leaf_nodes == 1 ? 0 : MIN_NUM_CHILDREN();

        if (leaf.num_records < min_num_records)
        {
            auto could_borrow = borrow_key(leaf);

            if (!could_borrow)
            {
                assert(false && "merge needed");
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
            info_.root_offset = alloc_node(root);
            info_.height++;

            root.num_children = 2;
            root.children[0].key = key;
            root.children[0].offset = left_offset;
            root.children[1].offset = right_offset;

            save(&info_, BASE_OFFSET_INFO_BLOCK);
            save(&root, info_.root_offset);

            set_parent_ptr(root.children, root.num_children, info_.root_offset);

            return;
        }

        bp_tree_node<N> node;
        load(&node, node_offset);

        if (node.num_children == info_.order)
        {
            bp_tree_node<N> new_node;
            auto new_node_offset = create_node(node_offset, node, new_node);

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

        assert(borrower.num_records < MIN_NUM_CHILDREN());
        const auto lender_offset = from_side == lender_side::right ? borrower.next : borrower.prev;
        
        if (lender_offset == 0)
        {
            // The borrower has no left/right neighbour
            return false;
        }

        bp_tree_leaf<N> lender;
        load(&lender, lender_offset);
        assert(lender.num_records >= MIN_NUM_CHILDREN());

        // If the lender don't have enough keys we can't borrow from it 
        if (lender.num_records == MIN_NUM_CHILDREN())
        {
            return false;
        }

        size_t src_index;
        size_t dest_index;

        if (from_side == lender_side::right)
        {
            src_index = 0;
            dest_index = borrower.num_records;
            change_parent(borrower.parent, borrower.records[0].key, lender.records[1].key);
        }
        else
        {
            src_index = lender.num_records - 1;
            dest_index = 0;
            change_parent(lender.parent, lender.records[0].key, lender.records[src_index].key);
        }

        auto& src = lender.records[src_index];
        insert_record_at(borrower, src.key, src.value, dest_index);

        remove_record_at(lender, src_index);
        save(&lender, lender_offset);

        return true;
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
        assert((leaf.num_records + 1) <= info_.order);
        auto dest_index = find_insert_index(leaf, key);
        insert_record_at(leaf, key, value, dest_index);
    }

    template<size_t N>
    void bp_tree<N>::insert_record_at(bp_tree_leaf<N> &leaf, const key &key, const value &value, size_t index)
    {
        assert(index < (leaf.num_records + 1));

        // Use a signed index to prevent i from wrapping on 0--
        for (auto i = static_cast<int32_t>(leaf.num_records) - 1; i >= static_cast<int32_t>(index); i--)
        {
            leaf.records[i + 1] = leaf.records[i];
        }

        leaf.records[index].key = key;
        leaf.records[index].value = value;
        leaf.num_records++;
    }

    template<size_t N>
    void bp_tree<N>::insert_record_split(const key& key, const value &value, offset leaf_offset, bp_tree_leaf<N> &leaf, bp_tree_leaf<N> &new_leaf)
    {
        assert(leaf.num_records == info_.order);

        auto new_leaf_offset = create_leaf(leaf_offset, leaf, new_leaf);

        bool key_greater_than_key_at_split;
        size_t split_index;
        std::tie(key_greater_than_key_at_split, split_index) = find_split_index(leaf.records, leaf.num_records, key);

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
        assert(from_index < source.num_records);

        for (auto i = from_index; i < source.num_records; i++)
        {
            target.records[i - from_index] = source.records[i];
            target.num_records++;
            // Not really needed, might remove
            source.records[i] = { 0 };
        }

        source.num_records = from_index;
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
        assert(index < source.num_records);

        for (auto i = index; i < source.num_records - 1; i++)
        {
            source.records[i] = source.records[i + 1];
        }

        source.num_records--;
    }

    template<size_t N>
    template<class T>
    tuple<bool, size_t> bp_tree<N>::find_split_index(const T * arr, size_t arr_len, const key & key)
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
        offset current_offset = info_.root_offset;
        auto height = info_.height;

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
    size_t bp_tree<N>::find_insert_index(const bp_tree_leaf<N> &leaf, const key& key) const
    {
        for (auto i = 0u; i < leaf.num_records; i++)
        {
            if (leaf.records[i].key > key)
            {
                return i;
            }
        }

        return leaf.num_records;
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
        if (leaf.num_records == 0)
            return -1;

        int64_t low = 0;
        int64_t high = static_cast<int64_t>(leaf.num_records - 1);

        while (low <= high)
        {
            auto mid = low + (high - low) / 2;
            auto current_key = leaf.records[mid].key;
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
        info_.num_internal_nodes++;
        return alloc(sizeof(bp_tree_node<N>));
    }

    template<size_t N>
    offset bp_tree<N>::alloc_leaf(bp_tree_leaf<N> &leaf)
    {
        info_.num_leaf_nodes++;
        return alloc(sizeof(bp_tree_leaf<N>));
    }

    template<size_t N>
    offset bp_tree<N>::alloc(size_t size)
    {
        auto current_offset = info_.next_slot_offset;
        info_.next_slot_offset += size;
        return current_offset;
    }

    template<size_t N>
    offset bp_tree<N>::create_leaf(offset leaf_offset, bp_tree_leaf<N> &leaf, bp_tree_leaf<N> &new_leaf)
    {
        /*
        Result if leaf has a right neighbour:

                         Parent

            X <---> leaf <---> new_leaf <---> X
        */

        // Insert new_leaf to the right of leaf
        new_leaf.parent = leaf.parent;
        new_leaf.next = leaf.next;
        new_leaf.prev = leaf_offset;

        // Create space for new_leaf and point leaf to it
        leaf.next = alloc_leaf(new_leaf);

        // If we have a right neighbour point its prev pointer to new_leaf
        if (new_leaf.next != 0)
        {
            bp_tree_leaf<N> old_next;
            load(&old_next, new_leaf.next);
            old_next.prev = leaf.next;
            save(&old_next, new_leaf.next);
        }

        save(&info_, BASE_OFFSET_INFO_BLOCK);
        return leaf.next;
    }

    template<size_t N>
    offset bp_tree<N>::create_node(offset node_offset, bp_tree_node<N> &node, bp_tree_node<N> &new_node)
    {
        // Same concept as create_leaf
        new_node.parent = node.parent;
        new_node.next = node.next;
        new_node.prev = node_offset;
        node.next = alloc_node(new_node);

        if (new_node.next != 0)
        {
            bp_tree_leaf<N> old_next;
            load(&old_next, new_node.next);
            old_next.prev = node.next;
            save(&old_next, new_node.next);
        }

        save(&info_, BASE_OFFSET_INFO_BLOCK);
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

        for (auto i = 0u; i < l.num_records; i++)
        {
            ss << l.records[i].key;
            if (i != l.num_records - 1)
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
    void bp_tree<N>::load(T *buffer, offset offset) const
    {
        static_assert(std::is_same<T, bp_tree_node<N>>::value || std::is_same<T, bp_tree_leaf<N>>::value || std::is_same<T, bp_tree_info>::value, "T must be a node, leaf or info");
        storage_->load(buffer, offset, sizeof(T));
    }

    template<size_t N>
    template<class T>
    void bp_tree<N>::save(T *value, offset offset) const
    {
        static_assert(std::is_same<T, bp_tree_node<N>>::value || std::is_same<T, bp_tree_leaf<N>>::value || std::is_same<T, bp_tree_info>::value, "T must be a node, leaf or info");
        storage_->store(value, offset, sizeof(T));
    }

    template class bp_tree<10>;
}
