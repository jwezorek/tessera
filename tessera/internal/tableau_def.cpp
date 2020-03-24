#include "tableau_def.h"
#include "expr_value.h"
#include "execution_ctxt.h"
#include "parser/statement_parser.h"

tess::tableau_def::tableau_def(const std::vector<std::string>& params, const text_range& code) :
    params_(params)
{
    auto results = tess::parser::parse_statements(code);
    if (std::holds_alternative<tess::stmts>(results)) {
        statements_ = std::get<tess::stmts>(results);
    }
    else {
        auto e = std::get<tess::parser::exception>(results);
        e.push_stack_item("tableau");
        if (!e.has_where())
            e.set_where(code.end());
        throw e;
    }
}

tess::expr_value tess::tableau_def::eval(const tess::execution_ctxt& ctxt) const
{
    return expr_value();
}


