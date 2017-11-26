#include "serialization.h"

#include <stdlib.h>

namespace niffler {

    static 
    void write_u32(u8 **buffer, u32 value)
    {
        memcpy(*buffer, &value, sizeof(u32));
        *buffer += sizeof(u32);
    }

    static
    u32 read_u32(u8 **buffer)
    {
        const auto value = (u32*)*buffer;
        *buffer += sizeof(u32);
        return *value;
    }

    static
    void write_int(u8 **buffer, int value)
    {
        memcpy(*buffer, &value, sizeof(int));
        *buffer += sizeof(int);
    }

    static
    int read_int(u8 **buffer)
    {
        const auto value = (int*)*buffer;
        *buffer += sizeof(int);
        return *value;
    }

    void serialize_bp_tree_header(u8 *buffer, const bp_tree_header &header)
    {
        static_assert(sizeof(page_index) == sizeof(u32));

        write_u32(&buffer, header.order);
        write_u32(&buffer, header.value_size);
        write_u32(&buffer, header.key_size);
        write_u32(&buffer, header.num_internal_nodes);
        write_u32(&buffer, header.num_leaf_nodes);
        write_u32(&buffer, header.height);
        write_u32(&buffer, header.root_page);
        write_u32(&buffer, header.leaf_page);
    }

    void deserialize_bp_tree_header(u8 *buffer, bp_tree_header &header)
    {
        static_assert(sizeof(page_index) == sizeof(u32));

        header.order = read_u32(&buffer);
        header.value_size = read_u32(&buffer);
        header.key_size = read_u32(&buffer);
        header.num_internal_nodes = read_u32(&buffer);
        header.num_leaf_nodes = read_u32(&buffer);
        header.height = read_u32(&buffer);
        header.root_page = read_u32(&buffer);
        header.leaf_page = read_u32(&buffer);
    }

    template<u32 N>
    void serialize_bp_tree_node(u8 *buffer, const bp_tree_node<N> &node)
    {
        static_assert(sizeof(key) == sizeof(u32));

        write_u32(&buffer, node.parent_page);
        write_u32(&buffer, node.next_page);
        write_u32(&buffer, node.prev_page);
        write_u32(&buffer, node.num_children);
        
        for (auto i = 0u; i < node.num_children; i++)
        {
            write_u32(&buffer, node.children[i].key);
            write_u32(&buffer, node.children[i].page);
        }
    }

    template<u32 N>
    void deserialize_bp_tree_node(u8 *buffer, bp_tree_node<N> &node)
    {
        node.parent_page = read_u32(&buffer);
        node.next_page = read_u32(&buffer);
        node.prev_page = read_u32(&buffer);
        node.num_children = read_u32(&buffer);

        for (auto i = 0u; i < node.num_children; i++)
        {
            node.children[i].key = read_u32(&buffer);
            node.children[i].page = read_u32(&buffer);
        }
    }

    template<u32 N>
    void serialize_bp_tree_leaf(u8 *buffer, const bp_tree_leaf<N> &leaf)
    {
        static_assert(sizeof(key) == sizeof(u32));
        static_assert(sizeof(value) == sizeof(int));

        write_u32(&buffer, leaf.parent_page);
        write_u32(&buffer, leaf.next_page);
        write_u32(&buffer, leaf.prev_page);
        write_u32(&buffer, leaf.num_children);

        for (auto i = 0u; i < leaf.num_children; i++)
        {
            write_u32(&buffer, leaf.children[i].key);
            write_int(&buffer, leaf.children[i].value);
        }
    }

    template<u32 N>
    void deserialize_bp_tree_leaf(u8 *buffer, bp_tree_leaf<N> &leaf)
    {
        leaf.parent_page = read_u32(&buffer);
        leaf.next_page = read_u32(&buffer);
        leaf.prev_page = read_u32(&buffer);
        leaf.num_children = read_u32(&buffer);

        for (auto i = 0u; i < leaf.num_children; i++)
        {
            leaf.children[i].key = read_u32(&buffer);
            leaf.children[i].value = read_int(&buffer);
        }
    }

    template void serialize_bp_tree_node(u8 *buffer, const bp_tree_node<4> &node);
    template void serialize_bp_tree_node(u8 *buffer, const bp_tree_node<6> &node);
    template void serialize_bp_tree_node(u8 *buffer, const bp_tree_node<10> &node);
    template void serialize_bp_tree_node(u8 *buffer, const bp_tree_node<DEFAULT_TREE_ORDER> &node);
    template void serialize_bp_tree_leaf(u8 *buffer, const bp_tree_leaf<4> &node);
    template void serialize_bp_tree_leaf(u8 *buffer, const bp_tree_leaf<6> &node);
    template void serialize_bp_tree_leaf(u8 *buffer, const bp_tree_leaf<10> &node);
    template void serialize_bp_tree_leaf(u8 *buffer, const bp_tree_leaf<DEFAULT_TREE_ORDER> &node);

    template void deserialize_bp_tree_node(u8 *buffer, bp_tree_node<4> &node);
    template void deserialize_bp_tree_node(u8 *buffer, bp_tree_node<6> &node);
    template void deserialize_bp_tree_node(u8 *buffer, bp_tree_node<10> &node);
    template void deserialize_bp_tree_node(u8 *buffer, bp_tree_node<DEFAULT_TREE_ORDER> &node);
    template void deserialize_bp_tree_leaf(u8 *buffer, bp_tree_leaf<4> &node);
    template void deserialize_bp_tree_leaf(u8 *buffer, bp_tree_leaf<6> &node);
    template void deserialize_bp_tree_leaf(u8 *buffer, bp_tree_leaf<10> &node);
    template void deserialize_bp_tree_leaf(u8 *buffer, bp_tree_leaf<DEFAULT_TREE_ORDER> &node);

}
