#include "tessera/tesserascript.h"
#include "./parser/util.h"
#include "tile.h"
#include "patch.h"
#include "tableau.h"
#include <iostream>
#include <unordered_map>

namespace tess {

    class tessera_script::script_impl_ {
        public:
            std::vector<tile> tiles_;
            std::vector<tile_patch> tile_patches_;
            tableau tableau_;
    };

}

tess::tessera_script::tessera_script() 
{
}

tess::tessera_script::tessera_script(const std::string& script_code, std::vector<script_component_specifier> sections, tab_spec ts)
{
    impl_ = std::make_shared<script_impl_>();
    for (const auto& section : sections) {
        auto [kind, name, params, iterators] = section;
        auto [begin, end] = iterators;
        auto implementation = text_range(script_code, begin, end);

        if (kind == parser::kw_tile)
            impl_->tiles_.emplace_back(name, params, implementation);
        else if (kind == parser::kw_patch)
            impl_->tile_patches_.emplace_back(name, params, implementation);
            
    }
    auto [begin, end] = std::get<1>(ts);
    impl_->tableau_ = tableau(text_range(script_code, begin, end));
}

