#include "bp_tree.h"

#include <assert.h>

namespace niffler {

    using std::tie;
    using std::stringstream;

    constexpr size_t BASE_OFFSET_INFO_BLOCK = 0;
    constexpr size_t BASE_OFFSET_DATA_BLOCK = BASE_OFFSET_INFO_BLOCK + sizeof(bp_tree_info);

    result<bp_tree> bp_tree::load(std::unique_ptr<storage_provider> storage)
    {
        return false;
    }

    result<bp_tree> bp_tree::create(std::unique_ptr<storage_provider> storage)
    {
        auto t = std::unique_ptr<bp_tree>(new bp_tree(std::move(storage)));

        t->info_.order = BP_TREE_ORDER;
        t->info_.value_size = sizeof(value);
        t->info_.key_size = sizeof(key);
        t->info_.height = 1;
        t->info_.next_slot_offset = BASE_OFFSET_DATA_BLOCK;

        bp_tree_node root;
        t->info_.root_offset = t->alloc_node(root);

        bp_tree_leaf leaf;
        leaf.parent = t->info_.root_offset;

        auto leaf_offset = t->alloc_leaf(leaf);
        t->info_.leaf_offset = leaf_offset;
        root.children[0].offset = leaf_offset;
        root.num_children = 1;

        // Save inital tree to the underlying storage
        t->save_to_storage(&t->info_, BASE_OFFSET_INFO_BLOCK);
        t->save_to_storage(&root, t->info_.root_offset);
        t->save_to_storage(&leaf, leaf_offset);

        return result<bp_tree>(true, std::move(t));
    }

    const bp_tree_info &bp_tree::info() const
    {
        return info_;
    }

    string bp_tree::print() const
    {
        stringstream ss;
        auto height = info_.height;
        auto current_offset = info_.root_offset;

        do
        {
            bp_tree_node n;
            load_from_storage(&n, current_offset);
            print_node_level(ss, current_offset);
            ss << std::endl;

            current_offset = n.children[0].offset;
            height--;
        } while (height > 0);

        print_leaf_level(ss, current_offset);

        return ss.str();
    }

    bool bp_tree::insert(const key &key, const value &value)
    {
        auto parent_offset = search_tree(key);
        assert(parent_offset != 0);

        auto leaf_offset = search_node(parent_offset, key);
        assert(leaf_offset != 0);

        bp_tree_leaf leaf;
        load_from_storage(&leaf, leaf_offset);

        // Key already exists
        if (binary_search_record(leaf, key) >= 0)
            return false;

        if (leaf.num_records == info_.order)
        {
            bp_tree_leaf new_leaf;
            insert_record_split(key, value, leaf_offset, leaf, new_leaf);
            insert_key(parent_offset, new_leaf.records[0].key, leaf_offset, leaf.next);
        }
        else
        {
            insert_record_non_full(leaf, key, value);
            save_to_storage(&leaf, leaf_offset);
        }

        return true;
    }

    bool bp_tree::remove(const key &key)
    {
        auto parent_offset = search_tree(key);
        assert(parent_offset != 0);

        auto leaf_offset = search_node(parent_offset, key);
        assert(leaf_offset != 0);

        bp_tree_leaf leaf;
        load_from_storage(&leaf, leaf_offset);

        // Return false if the key does not exists
        if (!remove_record(leaf, key))
            return false;

        assert(leaf.num_records >= MIN_NUM_CHILDREN());

        return true;
    }

    bp_tree::bp_tree(std::unique_ptr<storage_provider> storage)
        :
        storage_(std::move(storage))
    {
    }

    void bp_tree::insert_key(offset node_offset, const key &key, offset left_offset, offset right_offset)
    {
        // We have reached the root of the tree so we create a new root node
        if (node_offset == 0)
        {
            bp_tree_node root;
            info_.root_offset = alloc_node(root);
            info_.height++;

            root.num_children = 2;
            root.children[0].key = key;
            root.children[0].offset = left_offset;
            root.children[1].offset = right_offset;

            save_to_storage(&info_, BASE_OFFSET_INFO_BLOCK);
            save_to_storage(&root, info_.root_offset);

            set_parent_ptr(root.children, root.num_children, info_.root_offset);

            return;
        }

        bp_tree_node node;
        load_from_storage(&node, node_offset);

        if (node.num_children == info_.order)
        {
            bp_tree_node new_node;
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

            save_to_storage(&node, node_offset);
            save_to_storage(&new_node, new_node_offset);
            set_parent_ptr(new_node.children, new_node.num_children, new_node_offset);

            // Insert the middle key into the parent node
            insert_key(node.parent, middle_key, node_offset, new_node_offset);
        }
        else
        {
            insert_key_non_full(node, key, right_offset);
            save_to_storage(&node, node_offset);
        }
    }

    void bp_tree::insert_key_non_full(bp_tree_node &node, const key &key, offset next_offset)
    {
        auto dest_index = find_insert_index(node, key);
        insert_key_at(node, key, next_offset, dest_index);
    }

    void bp_tree::insert_key_at(bp_tree_node &node, const key &key, offset next_offset, size_t index)
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

    void bp_tree::set_parent_ptr(bp_tree_node_child *children, size_t c_length, offset parent)
    {
        bp_tree_node node;
        for (auto i = 0; i < c_length; i++)
        {
            load_from_storage(&node, children[i].offset);
            node.parent = parent;
            save_to_storage(&node, children[i].offset);
        }
    }

    void bp_tree::transfer_children(bp_tree_node &source, bp_tree_node &target, size_t from_index)
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

    void bp_tree::insert_record_non_full(bp_tree_leaf &leaf, const key &key, const value &value)
    {
        assert((leaf.num_records + 1) <= info_.order);
        auto dest_index = find_insert_index(leaf, key);
        insert_record_at(leaf, key, value, dest_index);
    }

    void bp_tree::insert_record_at(bp_tree_leaf &leaf, const key &key, const value &value, size_t index)
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

    void bp_tree::insert_record_split(const key& key, const value &value, offset leaf_offset, bp_tree_leaf &leaf, bp_tree_leaf &new_leaf)
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

        save_to_storage(&leaf, leaf_offset);
        save_to_storage(&new_leaf, new_leaf_offset);
    }

    void bp_tree::transfer_records(bp_tree_leaf &source, bp_tree_leaf &target, size_t from_index)
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

    bool bp_tree::remove_record(bp_tree_leaf &source, const key &key)
    {
        auto remove_index = binary_search_record(source, key);
        if (remove_index < 0)
            return false;

        remove_record_at(source, static_cast<size_t>(remove_index));
        return true;
    }

    void bp_tree::remove_record_at(bp_tree_leaf &source, size_t index)
    {
        assert(index < source.num_records);

        for (auto i = index; i < source.num_records - 1; i++)
        {
            source.records[i] = source.records[i + 1];
        }

        source.num_records--;
    }

    offset bp_tree::search_tree(const key &key) const
    {
        offset current_offset = info_.root_offset;
        auto height = info_.height;

        while (height > 1)
        {
            bp_tree_node node;
            load_from_storage(&node, current_offset);
            current_offset = find_node_child(node, key).offset;
            height -= 1;
        }

        return current_offset;
    }

    offset bp_tree::search_node(offset offset, const key &key) const
    {
        bp_tree_node node;
        load_from_storage(&node, offset);
        return find_node_child(node, key).offset;
    }

    size_t bp_tree::find_insert_index(const bp_tree_leaf &leaf, const key& key) const
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

    size_t bp_tree::find_insert_index(const bp_tree_node &node, const key &key) const
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

    const bp_tree_node_child &bp_tree::find_node_child(const bp_tree_node &node, const key &key) const
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

    int64_t bp_tree::binary_search_record(const bp_tree_leaf &leaf, const key &key)
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

    offset bp_tree::alloc_node(bp_tree_node &node)
    {
        info_.num_internal_nodes++;
        return alloc(sizeof(bp_tree_node));
    }

    offset bp_tree::alloc_leaf(bp_tree_leaf &leaf)
    {
        info_.num_leaf_nodes++;
        return alloc(sizeof(bp_tree_leaf));
    }

    offset bp_tree::alloc(size_t size)
    {
        auto current_offset = info_.next_slot_offset;
        info_.next_slot_offset += size;
        return current_offset;
    }

    offset bp_tree::create_leaf(offset leaf_offset, bp_tree_leaf &leaf, bp_tree_leaf &new_leaf)
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
            bp_tree_leaf old_next;
            load_from_storage(&old_next, new_leaf.next);
            old_next.prev = leaf.next;
            save_to_storage(&old_next, new_leaf.next);
        }

        save_to_storage(&info_, BASE_OFFSET_INFO_BLOCK);
        return leaf.next;
    }

    offset bp_tree::create_node(offset node_offset, bp_tree_node &node, bp_tree_node &new_node)
    {
        // Same concept as create_leaf
        new_node.parent = node.parent;
        new_node.next = node.next;
        new_node.prev = node_offset;
        node.next = alloc_node(new_node);

        if (new_node.next != 0)
        {
            bp_tree_leaf old_next;
            load_from_storage(&old_next, new_node.next);
            old_next.prev = node.next;
            save_to_storage(&old_next, new_node.next);
        }

        save_to_storage(&info_, BASE_OFFSET_INFO_BLOCK);
        return node.next;
    }

    void bp_tree::print_node_level(stringstream &ss, offset node_offset) const
    {
        bp_tree_node n;
        load_from_storage(&n, node_offset);

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

    void bp_tree::print_leaf_level(stringstream &ss, offset leaf_offset) const
    {
        bp_tree_leaf l;
        load_from_storage(&l, leaf_offset);

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
}
