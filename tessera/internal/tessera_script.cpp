#include "old_script_impl.h"
#include "tessera/tessera_script.h"
#include "./parser/util.h"
#include "./parser/keywords.h"
#include "./parser/expr_parser.h"
#include "tile_def.h"
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

    auto tableau_params = std::get<1>(ts);
    auto [begin, end] = std::get<2>(ts);
    impl_->insert_tableau_def(tableau_params, text_range( begin, end) );

	auto ctxt = execution_ctxt(*this);
	impl_->insert_globals(ctxt, globals);
//	impl_->build_tiles(ctxt);
}

const std::vector<std::string>& tess::tessera_script::parameters() const
{
    return impl_->tableau().params();
}

tess::result tess::tessera_script::execute(const std::vector<std::string>& argument_strings) const
{
    if (argument_strings.size() != parameters().size()) {
        return  { error("too few/many arguments") };
    }

    execution_ctxt ctxt(*this);
    
    std::vector<expr_value> args(argument_strings.size());
    std::transform( argument_strings.begin(), argument_strings.end(), args.begin(),
        [&ctxt](const std::string& str)->expr_value {
            expr_ptr expr = tess::parser::parse_expression(str);
            if (!expr)
                return { error("error parsing argument: " + str) };
            return expr->eval(ctxt);
        }
    );

    if (auto err_iter = std::find_if(args.begin(), args.end(), 
            [](const auto& v) {return std::holds_alternative<error>(v); }); err_iter != args.end()) {
        return std::get<error>(*err_iter);
    }
        
    auto output = ctxt.call(parser::keyword(parser::kw::tableau), args);
    if (std::holds_alternative<error>(output))
        return { std::get<error>(output) };
    if (!std::holds_alternative<tile_patch>(output))
        return { error("tableau does not evaulate to a tile patch.") };

    const auto& tiles = std::get<tile_patch>(output).tiles();
    return { tiles };
}

