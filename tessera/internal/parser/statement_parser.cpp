#include "statement_parser.h"
#include "cond_expr_parser.h"
#include "object_ref_expr_parser.h"
#include "../expression.h"
#include "../statement.h"
#include "expr_parser.h"
#include "util.h"
#include "keywords.h"
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>

namespace x3 = boost::spirit::x3;

BOOST_FUSION_ADAPT_STRUCT(tess::lay_params,
    tiles, such_that_clauses
)

BOOST_FUSION_ADAPT_STRUCT(tess::if_params,
    condition, then_clause, else_clause
)

namespace tess {
    namespace parser
    {
        template<typename T>
        auto make_ = 
			[&](auto& ctx) { _val(ctx) = std::make_shared<T>(_attr(ctx)); };

		auto make_obj_ref = 
			[](auto& ctx) { _val(ctx) = std::static_pointer_cast<object_ref_expr>( _attr(ctx) ); };

        const auto expr = expression_();
        const auto obj_ref_generic = object_ref_expr_();
        const auto cond_expr = cond_expr_();

        x3::rule<class obj_ref_, obj_ref_ptr> const obj_ref = "obj_ref";
        x3::rule<class obj_ref_pair_, std::tuple< obj_ref_ptr, obj_ref_ptr> > obj_ref_pair = "obj_ref_pair";
        x3::rule<class full_lay_stmt_aux_, lay_params> const full_lay_stmt_aux = "full_lay_stmt_aux";
        x3::rule<class full_lay_stmt_, stmt_ptr> const full_lay_stmt = "full_lay_stmt";
        x3::rule<class partial_lay_stmt_aux_, std::vector<obj_ref_ptr>> const partial_lay_stmt_aux = "partial_lay_stmt_aux";
        x3::rule<class partial_lay_stmt_, stmt_ptr> const partial_lay_stmt = "partial_lay_stmt";
        x3::rule<class lay_stmt_, stmt_ptr> const lay_stmt = "lay_stmt";
        x3::rule<class if_stmt_aux_, if_params> const if_stmt_aux = "if_stmt_aux";
        x3::rule<class if_stmt_, stmt_ptr> const if_stmt = "if_stmt";
        x3::rule<class stmt, stmt_ptr> const stmt = "statement";
		x3::rule<class statements_, std::vector<stmt_ptr>> const statements = "statements";

        auto const obj_ref_def = obj_ref_generic [make_obj_ref];
        auto const obj_ref_pair_def = obj_ref >> "<->" >> obj_ref;
        auto const full_lay_stmt_aux_def = kw_lit<kw::lay>() >> (obj_ref% x3::lit("->")) >> kw_lit<kw::such_that>() >> (obj_ref_pair% x3::lit(',')) > x3::lit(';');
        auto const full_lay_stmt_def = full_lay_stmt_aux [make_<tess::lay_statement>];
        auto const partial_lay_stmt_aux_def = kw_lit<kw::lay>() >> (obj_ref% x3::lit("->")) > x3::lit(';');
        auto const partial_lay_stmt_def = partial_lay_stmt_aux [make_<tess::lay_statement>];
        auto const lay_stmt_def = full_lay_stmt | partial_lay_stmt;
        auto const if_stmt_aux_def = kw_lit<kw::if_>() > cond_expr > stmt > kw_lit<kw::else_>() > stmt;
        auto const if_stmt_def = if_stmt_aux [make_<tess::if_statement>];
        auto const stmt_def = lay_stmt | if_stmt;
		auto const statements_def = +(stmt);

        BOOST_SPIRIT_DEFINE(
            obj_ref,
            obj_ref_pair,
            full_lay_stmt_aux,
            full_lay_stmt,
            partial_lay_stmt_aux,
            partial_lay_stmt,
            lay_stmt,
            if_stmt_aux,
            if_stmt,
            stmt,
			statements
        );
    }
}

std::tuple<tess::stmts, std::string::const_iterator> tess::parser::parse_statements_aux(const text_range& input)
{
    
    tess::stmts output;
    auto iter = input.begin();
    bool success = false;

    success = x3::phrase_parse(iter, input.end(), tess::parser::statements, x3::space, output);

    if (success)
        return { output, iter };
    else
		return { std::vector<stmt_ptr>(), iter };
}

std::variant<tess::stmts, tess::parser::exception> tess::parser::parse_statements(const tess::text_range& input)
{
    tess::stmts output;
    bool success = false;
    auto iter = input.begin();

    try {
        success = x3::phrase_parse(iter, input.end(), x3::lit('{') >> tess::parser::statements >> x3::lit('}'), x3::space, output);
    }
    catch (x3::expectation_failure<std::string::const_iterator> ex_fail) {
        return exception("", ex_fail);
    }

    if (!success || iter != input.end())
        return exception("", "syntax error", iter);

    return output;
}
