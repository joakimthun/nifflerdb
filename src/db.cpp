#include "include/db.h"

#include <mutex>

#include "include/exceptions.h"
#include "pager.h"
#include "bp_tree.h"

namespace niffler {

    using factory_fn = result<bp_tree<DEFAULT_TREE_ORDER>>(*)(pager*);

    db::db(const char *file_path, bool truncate_existing_file)
    {
        pager_ = new pager(file_path, truncate_existing_file);
        if (!pager_->ok())
            throw niffler_exception("Could not create or load db file");

        factory_fn db_factory = bp_tree<DEFAULT_TREE_ORDER>::load;

        if (truncate_existing_file)
        {
            db_factory = bp_tree<DEFAULT_TREE_ORDER>::create;
        }

        auto result = db_factory(pager_);
        if (!result.ok)
            throw niffler_exception("could not create database");

        bp_tree_ = result.value.release();
    }

    db::~db()
    {
        if (bp_tree_ != nullptr)
        {
            delete bp_tree_;
            bp_tree_ = nullptr;
        }

        if (pager_ != nullptr)
        {
            delete pager_;
            pager_ = nullptr;
        }
    }

    std::unique_ptr<find_result> db::find(const key &key) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return bp_tree_->find(key);
    }

    bool db::exists(const key &key) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return bp_tree_->exists(key);
    }

    bool db::insert(const key &key, const void *data, u32 data_size)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return bp_tree_->insert(key, data, data_size);
    }

    bool db::remove(const key &key)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return bp_tree_->remove(key);
    }
}
