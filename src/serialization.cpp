#include "serialization.h"

#include <stdlib.h>
#include <assert.h>

namespace niffler {

    static 
    void write_u16(u8 **buffer, u16 value)
    {
        memcpy(*buffer, &value, sizeof(u16));
        *buffer += sizeof(u16);
    }

    static
    u16 read_u16(const u8 **buffer)
    {
        const auto value = (u16*)*buffer;
        *buffer += sizeof(u16);
        return *value;
    }

    static
    void write_u32(u8 **buffer, u32 value)
    {
        memcpy(*buffer, &value, sizeof(u32));
        *buffer += sizeof(u32);
    }

    static
    u32 read_u32(const u8 **buffer)
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
    int read_int(const u8 **buffer)
    {
        const auto value = (int*)*buffer;
        *buffer += sizeof(int);
        return *value;
    }

    static
    void write_key(u8 **buffer, const key &k)
    {
        static_assert(sizeof(char) == 1);

        memcpy(*buffer, k.data, KEY_SIZE);
        *buffer += KEY_SIZE;
    }

    static
    void read_key(const u8 **buffer, key &k)
    {
        memcpy(k.data, *buffer, KEY_SIZE);
        *buffer += KEY_SIZE;
    }

    void serialize_file_header(u8 *buffer, const file_header &header)
    {
        static_assert(sizeof(page_index) == sizeof(u32));

        memcpy(buffer, header.version, sizeof(header.version));
        buffer += sizeof(header.version);

        write_u16(&buffer, header.page_size);
        write_u32(&buffer, header.num_pages);
        write_u32(&buffer, header.last_free_list_page);
        write_u32(&buffer, header.num_free_list_pages);
    }

    void deserialize_file_header(const u8 *buffer, file_header &header)
    {
        memcpy(header.version, buffer, sizeof(header.version));
        buffer += sizeof(header.version);

        header.page_size = read_u16(&buffer);
        header.num_pages = read_u32(&buffer);
        header.last_free_list_page = read_u32(&buffer);
        header.num_free_list_pages = read_u32(&buffer);
    }

    void serialize_page_header(u8 *buffer, const page_header &header)
    {
        static_assert(sizeof(page_index) == sizeof(u32));

        write_u32(&buffer, header.next_page);
        write_u32(&buffer, header.prev_page);
    }

    void deserialize_page_header(const u8 *buffer, page_header &header)
    {
        header.next_page = read_u32(&buffer);
        header.prev_page = read_u32(&buffer);
    }

    void serialize_free_list_header(u8 * buffer, const free_list_header &header)
    {
        write_u32(&buffer, header.next_page);
        write_u32(&buffer, header.prev_page);
        write_u32(&buffer, header.num_pages);
    }

    void deserialize_free_list_header(const u8 * buffer, free_list_header &header)
    {
        header.next_page = read_u32(&buffer);
        header.prev_page = read_u32(&buffer);
        header.num_pages = read_u32(&buffer);
    }

    void write_free_list_page_index(u8 * buffer, u32 index, page_index page_index)
    {
        const auto ptr_offset = free_list_header::DISK_SIZE() + (sizeof(u32) * index);
        assert(ptr_offset <= free_list_header::MAX_PAGES_SECTION_OFFSET());
        assert(ptr_offset >= free_list_header::DISK_SIZE());

        const auto page_index_ptr = buffer + ptr_offset;
        *((u32*)page_index_ptr) = page_index;
    }

    u32 read_free_list_page_index(const u8 *buffer, u32 index)
    {
        const auto ptr_offset = free_list_header::DISK_SIZE() + (sizeof(u32) * index);
        assert(ptr_offset <= free_list_header::MAX_PAGES_SECTION_OFFSET());
        assert(ptr_offset >= free_list_header::DISK_SIZE());

        const auto page_index_ptr = buffer + ptr_offset;
        return *((u32*)page_index_ptr);
    }

    void serialize_bp_tree_header(u8 *buffer, const bp_tree_header &header)
    {
        static_assert(sizeof(page_index) == sizeof(u32));

        write_u32(&buffer, header.order);
        write_u32(&buffer, header.key_size);
        write_u32(&buffer, header.num_internal_nodes);
        write_u32(&buffer, header.num_leaf_nodes);
        write_u32(&buffer, header.height);
        write_u32(&buffer, header.root_page);
        write_u32(&buffer, header.leaf_page);
    }

    void deserialize_bp_tree_header(const u8 *buffer, bp_tree_header &header)
    {
        static_assert(sizeof(page_index) == sizeof(u32));

        header.order = read_u32(&buffer);
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
        write_u32(&buffer, node.parent_page);
        write_u32(&buffer, node.next_page);
        write_u32(&buffer, node.prev_page);
        write_u32(&buffer, node.num_children);
        
        for (auto i = 0u; i < node.num_children; i++)
        {
            write_key(&buffer, node.children[i].key);
            write_u32(&buffer, node.children[i].page);
        }
    }

    template<u32 N>
    void deserialize_bp_tree_node(const u8 *buffer, bp_tree_node<N> &node)
    {
        node.parent_page = read_u32(&buffer);
        node.next_page = read_u32(&buffer);
        node.prev_page = read_u32(&buffer);
        node.num_children = read_u32(&buffer);

        for (auto i = 0u; i < node.num_children; i++)
        {
            read_key(&buffer, node.children[i].key);
            node.children[i].page = read_u32(&buffer);
        }
    }

    template<u32 N>
    void serialize_bp_tree_leaf(u8 *buffer, const bp_tree_leaf<N> &leaf)
    {
        static_assert(sizeof(value) == sizeof(int));

        write_u32(&buffer, leaf.parent_page);
        write_u32(&buffer, leaf.next_page);
        write_u32(&buffer, leaf.prev_page);
        write_u32(&buffer, leaf.num_children);

        for (auto i = 0u; i < leaf.num_children; i++)
        {
            write_key(&buffer, leaf.children[i].key);
            write_int(&buffer, leaf.children[i].value);
        }
    }

    template<u32 N>
    void deserialize_bp_tree_leaf(const u8 *buffer, bp_tree_leaf<N> &leaf)
    {
        leaf.parent_page = read_u32(&buffer);
        leaf.next_page = read_u32(&buffer);
        leaf.prev_page = read_u32(&buffer);
        leaf.num_children = read_u32(&buffer);

        for (auto i = 0u; i < leaf.num_children; i++)
        {
            read_key(&buffer, leaf.children[i].key);
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

    template void deserialize_bp_tree_node(const u8 *buffer, bp_tree_node<4> &node);
    template void deserialize_bp_tree_node(const u8 *buffer, bp_tree_node<6> &node);
    template void deserialize_bp_tree_node(const u8 *buffer, bp_tree_node<10> &node);
    template void deserialize_bp_tree_node(const u8 *buffer, bp_tree_node<DEFAULT_TREE_ORDER> &node);
    template void deserialize_bp_tree_leaf(const u8 *buffer, bp_tree_leaf<4> &node);
    template void deserialize_bp_tree_leaf(const u8 *buffer, bp_tree_leaf<6> &node);
    template void deserialize_bp_tree_leaf(const u8 *buffer, bp_tree_leaf<10> &node);
    template void deserialize_bp_tree_leaf(const u8 *buffer, bp_tree_leaf<DEFAULT_TREE_ORDER> &node);

}
