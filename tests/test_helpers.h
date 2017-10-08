#pragma once

#include <string.h>
#include <memory>

#include "bp_tree.h"

using namespace niffler;

struct bp_tree_validation_result {
    inline bp_tree_validation_result(const char *msg)
        : valid(false)
    {
        strcpy_s(message, 64, msg);
    }

    inline bp_tree_validation_result(bool valid)
        : valid(valid)
    {
        strcpy_s(message, 64, "ok");
    }

    bool valid;
    char message[64];
};

bp_tree_validation_result validate_bp_tree(std::unique_ptr<bp_tree> &tree);

