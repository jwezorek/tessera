#include "lay_expr.h"
#include "where_expr.h"
#include "variant_util.h"
#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "tessera/error.h"
#include "allocator.h"
#include "execution_state.h"
#include "ops.h"
#include <variant>

namespace {
   
    std::optional<tess::error> compile_edge_mappings(const std::vector<std::tuple<tess::expr_ptr, tess::expr_ptr>>& mappings, tess::stack_machine::stack& stack)
    {
        for (auto i = mappings.rbegin(); i != mappings.rend(); ++i) {
            auto [lhs, rhs] = *i;
            rhs->compile(stack);
            lhs->compile(stack);
        }
        return std::nullopt;
    }
}

tess::lay_expr::lay_expr(const std::vector<std::tuple<tess::expr_ptr, tess::expr_ptr>>& edge_mappings) : 
    edge_mappings_(edge_mappings)
{
}

void tess::lay_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<tess::lay_op>(static_cast<int>(edge_mappings_.size())));
    auto result = compile_edge_mappings(edge_mappings_, stack);
}

void tess::lay_expr::get_dependencies( std::unordered_set<std::string>& dependencies ) const
{
    for (auto [e1, e2] : edge_mappings_){
        e1->get_dependencies(dependencies);
        e2->get_dependencies(dependencies);
    }
}

tess::expr_ptr tess::lay_expr::simplify() const
{
    std::vector<std::tuple<expr_ptr, expr_ptr>> mappings_simplified(edge_mappings_.size());
    std::transform(edge_mappings_.begin(), edge_mappings_.end(), mappings_simplified.begin(),
        [](const auto& e_e)->std::tuple<expr_ptr, expr_ptr> {
            const auto& [e1, e2] = e_e;
            return { e1->simplify(), e2->simplify() };
        }
    );

    return std::make_shared<lay_expr>(mappings_simplified);
}

std::string tess::lay_expr::to_string() const
{
    std::stringstream ss;
    ss << "( lay ";
    for (const auto& [e1, e2] : edge_mappings_) {
        ss << "( <-> " << e1->to_string() << " " << e2->to_string() << ")";
    };
    ss << ") )";
    return ss.str();
}

