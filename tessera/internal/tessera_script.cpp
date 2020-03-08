#include "script_impl.h"
#include "tessera/tessera_script.h"
#include "./parser/util.h"
#include "./parser/keywords.h"
#include "tile_def.h"
#include "tile_patch_def.h"
#include "tableau_def.h"
#include <iostream>
#include <unordered_map>

tess::tessera_script::tessera_script() 
{
}

tess::tessera_script::tessera_script(std::vector<script_component_specifier> sections, tab_spec ts, global_vars globals)
{
    impl_ = std::make_shared<script_impl>();
    for (const auto& section : sections) {
        auto [kind, name, params, iterators] = section;
        auto [begin, end] = iterators;
        auto implementation = text_range( begin, end);

        if (kind == parser::keyword(parser::kw::tile))
            impl_->insert_tile_def( name, params, implementation );
        else if (kind == parser::keyword(parser::kw::patch))
            impl_->insert_patch_def( name, params, implementation );
    }

    auto [begin, end] = std::get<1>(ts);
    impl_->insert_tableau_def( text_range( begin, end) );

	auto ctxt = execution_ctxt(*this);
	impl_->insert_globals(ctxt, globals);
	impl_->build_tiles(ctxt);
}

std::vector<std::string> tess::tessera_script::parameters() const
{
	// TODO: parametrized tableau definitions
	return std::vector<std::string>();
}

tess::tableau tess::tessera_script::execute(const arguments& args) const
{
	return tableau();
}

