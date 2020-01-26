#pragma once

#include "tessera/error.h"
#include <string>

namespace tess {

    class text_range {
    private:
        std::string::const_iterator begin_;
        std::string::const_iterator end_;
    public:
		text_range() {}
        text_range(const std::string& str);
        text_range(std::string::const_iterator b, std::string::const_iterator e);
        std::string::const_iterator begin() const;
        std::string::const_iterator end() const;
        text_range left_range(std::string::const_iterator i) const;
		text_range right_range(std::string::const_iterator i) const;
		int get_line_count() const;
    };
}

