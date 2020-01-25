#pragma once

#include <string>
#include <vector>
#include "text_range.h"

namespace tess {

    class tile
    {
    private: 
        std::string name_;
        std::vector<std::string> params_;
        text_range code_;
    public:
        tile(const std::string& name , std::vector<std::string> params, const text_range& source_code);
    };

    class tile_component_declaration
    {
    private:
        std::string test_;
    public:
        tile_component_declaration(const std::string& test = "");
    };

}