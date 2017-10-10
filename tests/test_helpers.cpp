#include "test_helpers.h"

bool find_key_for_node_offset(const bp_tree_node &node, offset target, key *key)
{
    for (auto i = 0u; i < node.num_children; i++)
    {
        if (node.children[i].offset == target)
        {
            *key = node.children[i].key;
            return true;
        }
    }

    return false;
}

bp_tree_validation_result validate_bp_tree_leaf(std::unique_ptr<bp_tree> &tree, bp_tree_leaf &leaf, offset current_offset, offset prev_offset)
{
    if (leaf.prev != prev_offset)
        return "leaf points to wrong left neighbour";

    bp_tree_node leaf_parent;
    tree->load_from_storage(&leaf_parent, leaf.parent);
    auto is_root_descendant = leaf_parent.parent == 0;

    // If this leaf is a direct child of the root node it cant be within the valid children range
    // e.g. only 1 key in the tree
    if (!is_root_descendant)
    {
        if (leaf.num_records < MIN_NUM_CHILDREN())
            return "leaf has to few children";

        if (leaf.num_records > MAX_NUM_CHILDREN())
            return "leaf has to many children";
    }

    key parent_key;
    if (!find_key_for_node_offset(leaf_parent, current_offset, &parent_key))
        return "leaf points to wrong parent";

    // If this leaf is a direct child of the root node and the root only has 1 key, keys will not be in a valid state
    // which is fine since we only have 1 key and will traverse to the correct leaf anyway
    if (is_root_descendant && leaf_parent.num_children == 1)
        return true;

    for (auto i = 0u; i < leaf.num_records - 1; i++)
    {
        if (leaf.records[i].key > parent_key)
        {
            return "record key to large for parent key";
        }
    }

    return true;
}

bp_tree_validation_result validate_bp_tree_keys(std::unique_ptr<bp_tree> &tree, bp_tree_node &node, bool last_nlevel)
{
    if (last_nlevel)
    {
        for (auto i = 0u; i < node.num_children - 1; i++)
        {
            auto& child = node.children[i];
            bp_tree_leaf leaf;
            tree->load_from_storage(&leaf, child.offset);

            for (auto j = 0u; j < leaf.num_records; j++)
            {
                if (leaf.records[j].key > child.key)
                {
                    return "child key to large for parent key";
                }
            }
        }
    }
    else
    {
        for (auto i = 0u; i < node.num_children - 1; i++)
        {
            auto& child = node.children[i];
            bp_tree_node child_node;
            tree->load_from_storage(&child_node, child.offset);

            for (auto j = 0u; j < child_node.num_children; j++)
            {
                if (child_node.children[j].key > child.key)
                {
                    return "child key to large for parent key";
                }
            }
        }
    }

    return true;
}

bp_tree_validation_result validate_bp_tree_node(std::unique_ptr<bp_tree> &tree, bp_tree_node &node, offset current_offset, offset prev_offset, bool last_nlevel)
{
    if (node.prev != prev_offset)
        return "node points to wrong left neighbour";

    if (node.num_children < MIN_NUM_CHILDREN())
        return "node has to few children";

    if (node.num_children > MAX_NUM_CHILDREN())
        return "node has to many children";

    auto result = validate_bp_tree_keys(tree, node, last_nlevel);
    if (!result.valid)
        return result;

    if (node.next)
    {
        bp_tree_node next_node;
        tree->load_from_storage(&next_node, node.next);
        return validate_bp_tree_node(tree, next_node, node.next, current_offset, last_nlevel);
    }

    return true;
}

bp_tree_validation_result validate_bp_tree(std::unique_ptr<bp_tree> &tree)
{
    bp_tree_node root;
    tree->load_from_storage(&root, tree->info().root_offset);

    if (tree->info().height == 0)
        return "tree has no height";

    if (root.parent != 0)
        return "root has parent";

    if (root.num_children == 0)
        return "root has no children";

    auto result = validate_bp_tree_keys(tree, root, tree->info().height <= 2);
    if (!result.valid)
        return result;

    auto height = tree->info().height;
    auto current_parent_offset = tree->info().root_offset;
    auto current_prev_offset = 0;
    auto current_offset = root.children[0].offset;
    auto next_row_first_offset = root.children[0].offset;

    while (height > 1)
    {
        bp_tree_node node;
        tree->load_from_storage(&node, current_offset);
        next_row_first_offset = node.children[0].offset;

        result = validate_bp_tree_node(tree, node, current_offset, current_prev_offset, height <= 2);
        if (!result.valid)
            return result;

        current_parent_offset = current_offset;
        current_prev_offset = 0;
        current_offset = next_row_first_offset;
        height--;
    }

    // Validate leaf level
    auto current_leaf_offset = next_row_first_offset;
    bp_tree_leaf leaf;
    tree->load_from_storage(&leaf, current_leaf_offset);
    current_prev_offset = 0;

    result = validate_bp_tree_leaf(tree, leaf, current_leaf_offset, current_prev_offset);
    if (!result.valid)
        return result;

    while (leaf.next)
    {
        result = validate_bp_tree_leaf(tree, leaf, current_leaf_offset, current_prev_offset);
        if (!result.valid)
            return result;

        current_prev_offset = current_leaf_offset;
        current_leaf_offset = leaf.next;
        tree->load_from_storage(&leaf, current_leaf_offset);
    }

    return true;
}
