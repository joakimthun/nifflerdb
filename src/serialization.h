#pragma once

#include "define.h"
#include "bp_tree.h"

namespace niffler {

    void serialize_bp_tree_header(u8 *buffer, const bp_tree_header &header);
    void deserialize_bp_tree_header(const u8 *buffer, bp_tree_header &header);

    template<u32 N>
    void serialize_bp_tree_node(u8 *buffer, const bp_tree_node<N> &node);
    template<u32 N>
    void deserialize_bp_tree_node(const u8 *buffer, bp_tree_node<N> &node);

    template<u32 N>
    void serialize_bp_tree_leaf(u8 *buffer, const bp_tree_leaf<N> &leaf);
    template<u32 N>
    void deserialize_bp_tree_leaf(const u8 *buffer, bp_tree_leaf<N> &leaf);

}
