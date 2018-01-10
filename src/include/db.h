#pragma once

#include <memory>
#include <shared_mutex>

#include "define.h"

namespace niffler {

    struct find_result {
        bool found = false;
        u32 size = 0;
        void *data = nullptr;

        inline ~find_result()
        {
            if (data != nullptr)
            {
                free(data);
                data = nullptr;
            }
        }
    };

    constexpr u32 KEY_SIZE = 16;

    struct key {
        char data[KEY_SIZE] = { 0 };

        inline key() {}

        inline key(int key) {
            _itoa_s(key, data, 10);
        }

        inline key(const char *key) {
            strcpy_s(data, sizeof(data), key);
        }
    };

    inline int key_cmp(const key &lhs, const key &rhs) {
        const auto len_diff = strlen(lhs.data) - strlen(rhs.data);
        if (len_diff == 0)
            return strcmp(lhs.data, rhs.data);

        return len_diff;
    }

    inline bool operator==(const key& lhs, const key& rhs) { return key_cmp(lhs, rhs) == 0; }
    inline bool operator!=(const key& lhs, const key& rhs) { return !(lhs == rhs); }
    inline bool operator< (const key& lhs, const key& rhs) { return key_cmp(lhs, rhs) < 0; }
    inline bool operator> (const key& lhs, const key& rhs) { return rhs < lhs; }
    inline bool operator<=(const key& lhs, const key& rhs) { return !(lhs > rhs); }
    inline bool operator>=(const key& lhs, const key& rhs) { return !(lhs < rhs); }

    class pager;
    template<u32 N> class bp_tree;

    class db
    {
    public:
        db() = delete;
        db(const char *file_path, bool truncate_existing_file);
        ~db();

        std::unique_ptr<find_result> find(const key& key) const;
        bool exists(const key& key) const;
        bool insert(const key& key, const void *data, u32 data_size);
        bool remove(const key& key);

    private:
        pager *pager_ = nullptr;
        bp_tree<DEFAULT_TREE_ORDER> *bp_tree_ = nullptr;
        mutable std::shared_mutex mutex_;
    };
}
