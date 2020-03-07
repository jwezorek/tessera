#include "tessera/tesserascript.h"
#include "./parser/util.h"
#include "./parser/keywords.h"
#include "tile_def.h"
#include "tile_patch_def.h"
#include "tableau_def.h"
#include <iostream>
#include <unordered_map>

namespace tess {

    class tessera_script::script_impl_ {
        public:
            std::vector<tile_def> tiles_;
            std::vector<tile_patch_def> tile_patches_;
            tableau_def tableau_;
    };

}

tess::tessera_script::tessera_script() 
{
}

tess::tessera_script::tessera_script(std::vector<script_component_specifier> sections, tab_spec ts)
{
    impl_ = std::make_shared<script_impl_>();
    for (const auto& section : sections) {
        auto [kind, name, params, iterators] = section;
        auto [begin, end] = iterators;
        auto implementation = text_range( begin, end);

        if (kind == parser::keyword(parser::kw::tile))
            impl_->tiles_.emplace_back(name, params, implementation);
        else if (kind == parser::keyword(parser::kw::patch))
            impl_->tile_patches_.emplace_back(name, params, implementation);
            
    }
    auto [begin, end] = std::get<1>(ts);
    impl_->tableau_ = tableau_def(text_range( begin, end));
}

