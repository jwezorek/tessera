#pragma once

#include <string>
#include <vector>
#include "text_range.h"

namespace tess {

    class tile_patch
    {
    private:
        std::string name_;
        std::vector<std::string> params_;
        text_range code_;
    public:
        tile_patch(const std::string& name, std::vector<std::string> params, const text_range& source_code);
    };

}