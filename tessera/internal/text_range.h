#pragma once

#include "tessera/error.h"
#include <string>

namespace tess {

    class text_range {
    private:
        const std::string* input_;
        std::string::const_iterator begin_;
        std::string::const_iterator end_;
    public:
        text_range(const std::string& inp="");
        text_range(const std::string& inp, std::string::const_iterator b, std::string::const_iterator e);
        std::string::const_iterator begin() const;
        std::string::const_iterator end() const;
        text_range sub_range(std::string::const_iterator i1, std::string::const_iterator i2) const;
        text_range left_range(std::string::const_iterator i) const;
        tess::error make_error(const std::string& msg) const;
        const std::string& str() const;
    };
}

