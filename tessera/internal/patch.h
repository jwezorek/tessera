#pragma once

#include <string>
#include <vector>
#include "text_range.h"
#include "statement.h"

namespace tess {

    class tile_patch
    {
    private:
        std::string name_;
        std::vector<std::string> params_;
        stmts statements_;
    public:
        tile_patch(const std::string& name, std::vector<std::string> params, const text_range& source_code);
    };

}