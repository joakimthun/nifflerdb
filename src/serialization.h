#pragma once

#include "include/define.h"
#include "pager.h"
#include "bp_tree.h"

namespace niffler {

    void serialize_file_header(u8 *buffer, const file_header &header);
    void deserialize_file_header(const u8 *buffer, file_header &header);

    void serialize_page_header(u8 *buffer, const page_header &header);
    void deserialize_page_header(const u8 *buffer, page_header &header);

    void serialize_free_list_header(u8 *buffer, const free_list_header &header);
    void deserialize_free_list_header(const u8 *buffer, free_list_header &header);
    void write_free_list_page_index(u8 *buffer, u32 index, page_index page_index);
    u32 read_free_list_page_index(const u8 *buffer, u32 index);

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
