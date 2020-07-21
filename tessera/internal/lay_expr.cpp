#include "lay_expr.h"
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

tess::lay_expr::lay_expr(const lay_params& params) :
    tiles_(params.tiles),
    edge_mappings_(params.edge_mappings)
{
}


tess::lay_expr::lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings) :
    tiles_(tiles),
    edge_mappings_(edge_mappings)
{
}

void tess::lay_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<lay_op>(static_cast<int>(edge_mappings_.size())));
    auto result = compile_edge_mappings(edge_mappings_, stack);

    int index = 1;
    for (auto tile : tiles_) {
        stack.push(std::make_shared<assign_op>(1));
        stack.push(stack_machine::variable(index++));
        tile->compile(stack);
    }
    stack.push(std::make_shared<push_frame_op>());
}

void tess::lay_expr::get_dependencies( std::unordered_set<std::string>& dependencies ) const
{
    for (auto tile : tiles_)
        tile->get_dependencies(dependencies);
    for (auto [e1, e2] : edge_mappings_){
        e1->get_dependencies(dependencies);
        e2->get_dependencies(dependencies);
    }
}

tess::expr_ptr tess::lay_expr::simplify() const
{
    std::vector<expr_ptr> tiles_simplified( tiles_.size() );
    std::transform(tiles_.begin(), tiles_.end(), tiles_simplified.begin(),
        [](const auto& e) {return e->simplify(); }
    );

    std::vector<std::tuple<expr_ptr, expr_ptr>> mappings_simplified(edge_mappings_.size());
    std::transform(edge_mappings_.begin(), edge_mappings_.end(), mappings_simplified.begin(),
        [](const auto& e_e)->std::tuple<expr_ptr, expr_ptr> {
            const auto& [e1, e2] = e_e;
            return { e1->simplify(), e2->simplify() };
        }
    );

    return std::make_shared<lay_expr>(tiles_simplified, mappings_simplified);
}

std::string tess::lay_expr::to_string() const
{
    std::stringstream ss;
    ss << "( lay ( ";
    for (const auto& t : tiles_)
        ss << t->to_string() << " ";
    ss << ") ( ";
    for (const auto& [e1, e2] : edge_mappings_) {
        ss << "( <-> " << e1->to_string() << " " << e2->to_string() << ")";
    };
    ss << ") )";
    return ss.str();
}

