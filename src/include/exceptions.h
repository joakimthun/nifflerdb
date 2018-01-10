#pragma once

#include <string>
#include <exception>

namespace niffler {

    class niffler_exception : public std::exception
    {
    public:
        inline niffler_exception(const std::string &message) : message_(message) {}

        inline const char* what() const override
        {
            return message_.c_str();
        }

    private:
        std::string message_;
    };

}
