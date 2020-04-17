#include "tile_parser.h"
#include "expr_parser.h"
#include "keywords.h"
#include "util.h"
#include "../tile_def.h"
#include <tuple>
#include <unordered_map>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <variant>

namespace x3 = boost::spirit::x3;

namespace tess {
    namespace parser {

        struct class_field {
            std::string tag;
            std::string str;
        };

        struct edge_field {
            std::string u;
            std::string v;
        };

        struct angle_field {
            std::string tag;
            expr_ptr theta;
        };

        struct length_field {
            std::string tag;
            expr_ptr len;
        };

        using vert_field_var = std::variant<angle_field, class_field>;
        using vert_fields = std::vector<vert_field_var>;

        struct vert_definition
        {
            std::string name;
            vert_fields fields;

            vert_definition()
            {}

            vert_definition(std::string s, vert_fields v) :
                name(s), fields(v)
            {}

            vert_definition(std::string s) :
                name(s)
            {}
        };

        using edge_field_var = std::variant<edge_field, length_field, class_field>;
        using edge_fields = std::vector<edge_field_var>;

        struct edge_definition {
            std::string name;
            edge_fields fields;

            edge_definition()
            {}

            edge_definition(std::string s, edge_fields f) :
                name(s), fields(f)
            {}

            edge_definition(std::string s) :
                name(s)
            {}
        };

        using boost::fusion::operator<<;
        
    }
}

BOOST_FUSION_ADAPT_STRUCT(tess::parser::class_field,
    tag, str
)

BOOST_FUSION_ADAPT_STRUCT(tess::parser::edge_field,
    u, v
)

BOOST_FUSION_ADAPT_STRUCT(tess::parser::angle_field,
    tag, theta
)

BOOST_FUSION_ADAPT_STRUCT(tess::parser::length_field,
    tag, len
)

BOOST_FUSION_ADAPT_STRUCT(tess::parser::vert_definition,
    name, fields
)

BOOST_FUSION_ADAPT_STRUCT(tess::parser::edge_definition,
    name, fields
)

namespace tess {
    namespace parser {

        using edges_definition = std::vector<edge_definition>;
        using verts_definition = std::vector<vert_definition>;
        using ve_defs_var = std::variant<verts_definition, edges_definition>;
        using ve_definitions = std::vector<ve_defs_var>;

        x3::rule<class class_field_rule, tess::parser::class_field> const class_field_ = "class_field";
        x3::rule<class edge_field_rule, tess::parser::edge_field> const edge_field_ = "edge_field";
        x3::rule<class angle_field_rule, tess::parser::angle_field> const angle_field_ = "angle_field";
        x3::rule<class length_field_rule, tess::parser::length_field> const length_field_ = "length_field";
        x3::rule<class basic_edge_def_1_, edge_definition> basic_edge_def_1 = "basic_edge_def_1";
        x3::rule<class edge_definition_tag, edge_definition> edge_definition_ = "edge_definition_";
        x3::rule<class edges_definition_tag, edges_definition> edges_definition_ = "edges_definition_";
        x3::rule<class basic_vert_def_, vert_definition> basic_vert_def = "basic_vert_def";
        x3::rule<class vert_definition__, vert_definition> vert_definition_ = "vert_definition_";
        x3::rule<class verts_definition__, verts_definition>  verts_definition_ = "verts_definition_";
        x3::rule<class tile_definition__, std::tuple<std::vector<std::string>, ve_definitions>>  tile_definition = "tile_definition";

        const auto expr = expression_();
        const auto identifier_str = indentifier_str_();
        auto const class_field__def = kw_<kw::class_>() > x3::lit(':') > identifier_str;
        auto const edge_field__def = identifier_str >> x3::lit("->") > identifier_str;
        auto const angle_field__def = (kw_<kw::angle>() > x3::lit(':') > expr) | (x3::string("") >> expr);
        auto const length_field__def = (kw_<kw::length>() > x3::lit(':') > expr) | (x3::string("") >> expr);
        auto const edge_field_var_ = edge_field_ | length_field_ | class_field_ ;
        auto const edge_fields_ = edge_field_var_ % x3::lit(',');
        auto const basic_edge_def_1_def = identifier_str;
        auto const edge_definition__def = (identifier_str >> x3::lit('{') >> edge_fields_ >> x3::lit('}')) | basic_edge_def_1;
        auto const edges_definition__def = kw_lit<kw::edge>() >> (edge_definition_ % ',') >> ';';
        auto const vert_field_var_ = angle_field_ | class_field_;
        auto const vert_fields_ = vert_field_var_ % x3::lit(',');
        auto const basic_vert_def_def = identifier_str;
        auto const vert_definition__def = (identifier_str >> x3::lit('{') >> vert_fields_ >> x3::lit('}')) | basic_vert_def;
        auto const verts_definition__def = kw_lit<kw::vertex>() >> (vert_definition_ % ',') >> ';';
        auto const ve_defs_var_ = verts_definition_ | edges_definition_ ;
        auto const ve_definitions_ = *(ve_defs_var_);

        auto const parameters = -(x3::lit('(') >> (identifier_str % x3::lit(',')) >> x3::lit(')'));
        auto const tile_definition_def = kw_lit<kw::tile>() >> parameters >> x3::lit('{') > ve_definitions_ > x3::lit('}');

        BOOST_SPIRIT_DEFINE(
            class_field_,
            edge_field_,
            angle_field_,
            length_field_,
            basic_edge_def_1,
            edge_definition_,
            edges_definition_,
            basic_vert_def,
            vert_definition_,
            verts_definition_,
            tile_definition
        );

        std::variant<tile_verts_and_edges, exception> unpack(const ve_definitions& defs) {
            using v_map = std::unordered_map<std::string, vertex_def>;
            using e_map = std::unordered_map<std::string, tess::edge_def>;
            v_map v;
            e_map e;

            struct visit_vert_field {
                const std::string& name;
                v_map& v_;

                visit_vert_field(v_map& vm, const std::string& n) : v_(vm), name(n) {}

                void operator()(const angle_field& af) {
                    v_[name].angle = af.theta;
                }

                void operator()(const class_field& cf) {
                    v_[name].class_ = cf.str;
                }
            };

            struct visit_edge_field {
                const std::string& name;
                e_map& e_;

                visit_edge_field(e_map& vm, const std::string& n) : e_(vm), name(n) {}

                void operator()(const length_field& lf) {
                    e_[name].length = lf.len;
                }

                void operator()(const class_field& cf) {
                    e_[name].class_ = cf.str;
                }

                void operator()(const edge_field& ef) {
                    e_[name].u = ef.u;
                    e_[name].v = ef.v;
                }
            };

            struct visit_def {

                v_map& v_;
                e_map& e_;

                visit_def(v_map& vm, e_map& em) : v_(vm), e_(em)
                {}

                void operator()(const verts_definition& verts) {
                    for (const auto& v : verts) {
                        std::string vertex_name = v.name;
                        v_[vertex_name].name = vertex_name;
                        for (const auto& field : v.fields)
                            std::visit(visit_vert_field(v_, vertex_name), field);
                    }
                }
                void operator()(const edges_definition& edges) {
                    for (const auto& e : edges) {
                        std::string edge_name = e.name;
                        e_[edge_name].name = edge_name;
                        for (const auto& field : e.fields)
                            std::visit(visit_edge_field(e_, edge_name), field);
                    }
                }
            };

            for (const auto& def : defs) 
                std::visit(visit_def(v,e), def);
            
            return tess::tile_verts_and_edges(v, e);
        }
    }
}


std::variant<tess::tile_verts_and_edges, tess::parser::exception> tess::parser::parse_tile(const tess::text_range& input)
{
    tess::parser::ve_definitions output;
    bool success = false;
    auto iter = input.begin();

    try {
        success = x3::phrase_parse(iter, input.end(), x3::lit('{') >> tess::parser::ve_definitions_ >> x3::lit('}'), x3::space, output);
    } catch (x3::expectation_failure<std::string::const_iterator> ex_fail) {
        return exception("", ex_fail);
    }

    if (!success || iter != input.end())
        return exception("", "syntax error", iter);

    return tess::parser::unpack(output);
}


std::tuple<tess::expr_ptr, std::string::const_iterator> tess::parser::tile_def_::parse_aux(const text_range& input) const
{
    std::tuple<std::vector<std::string>, tess::parser::ve_definitions> output;
    bool success = false;
    auto iter = input.begin();

    try {
        success = x3::phrase_parse(iter, input.end(), tess::parser::tile_definition, x3::space, output);
    }
    catch (...)
    {
    }

    if (success) {
        auto [params, v_e] = output;
        auto maybe_ve = tess::parser::unpack(v_e);

        if (! std::holds_alternative<tile_verts_and_edges>(maybe_ve))
            return { tess::expr_ptr(), iter };

        tile_def tile(params, std::get<tile_verts_and_edges>(maybe_ve) );

        return { std::make_shared<function_def>(params, tile), iter};
    }
    return { tess::expr_ptr(), iter };
}