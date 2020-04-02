#include "tile_patch_def.h"
#include "parser/statement_parser.h"

tess::tile_patch_def::tile_patch_def(const std::string& name, std::vector<std::string> params, const text_range& source_code) :
    name_(name), params_(params)
{
    auto results = tess::parser::parse_single_statement_block(source_code);
    if (std::holds_alternative<tess::stmt_ptr>(results)) {
        statement_= std::get<tess::stmt_ptr>(results);
    } else {
        auto e = std::get<tess::parser::exception>(results);
        e.push_stack_item("patch " + name);
        if (!e.has_where())
            e.set_where(source_code.end());
        throw e;
    }
}

std::string tess::tile_patch_def::name() const
{
	return name_;
}

const std::vector<std::string>& tess::tile_patch_def::params() const
{
	return params_;
}

tess::expr_value tess::tile_patch_def::eval( execution_ctxt& ctxt ) const
{
    return statement_->execute(ctxt);
}
