#pragma once

#include <string>

namespace tess {

    class tile
    {
    private: 
        std::string name_;
    public:
        tile(const std::string& name = "");
    };

    class tile_component_declaration
    {
    private:
        std::string test_;
    public:
        tile_component_declaration(const std::string& test = "");
    };

}