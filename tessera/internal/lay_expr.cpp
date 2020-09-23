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

    class lay_expr_aux : public tess::expression {
    public:
        lay_expr_aux(const std::vector<std::tuple<tess::expr_ptr, tess::expr_ptr>>& edge_mappings) : edge_mappings_(edge_mappings)
        {}

        void compile(tess::stack_machine::stack& stack) const override {
            stack.push(std::make_shared<tess::lay_op>(static_cast<int>(edge_mappings_.size())));
            auto result = compile_edge_mappings(edge_mappings_, stack);
        }
        std::string to_string() const override { return {}; }
        tess::expr_ptr simplify() const override { return {}; }
        void get_dependencies(std::unordered_set<std::string>& dependencies) const override {}

    private:
        std::vector<std::tuple<tess::expr_ptr, tess::expr_ptr>> edge_mappings_;

    };

    tess::assignment_block get_placeholder_assignments(const std::vector<tess::expr_ptr> vals) {
        std::vector<tess::var_assignment> assignments(vals.size());
        for (int i = 0; i < vals.size(); i++) {
            auto var = std::to_string(i + 1);
            assignments[i] = { std::vector<std::string>{var}, vals[i] };
        }
        return tess::assignment_block(assignments);
    }

}

tess::lay_expr::lay_expr(const lay_params& params) :
    tiles_(params.tiles),
    edge_mappings_(params.edge_mappings)
{
}

tess::lay_expr::lay_expr(const full_lay_params& params) :
    tiles_(params.tiles),
    edge_mappings_(params.edge_mappings),
    with_(params.with)
{
}

tess::lay_expr::lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings) :
    tiles_(tiles),
    edge_mappings_(edge_mappings)
{
}

tess::lay_expr::lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings, const field_definitions& with) :
    tiles_(tiles),
    edge_mappings_(edge_mappings),
    with_(with)
{
}

void tess::lay_expr::compile(stack_machine::stack& stack) const
{
    auto assignments = get_placeholder_assignments(tiles_);
    auto lay = std::make_shared<lay_expr_aux>(edge_mappings_);
    expr_ptr body = (with_.empty()) ? lay : expr_ptr{ std::make_shared<with_expr>(with_, lay) };
    auto where_wrapper = std::make_shared<where_expr>(assignments, body);
    
    where_wrapper->compile(stack);
}

void tess::lay_expr::get_dependencies( std::unordered_set<std::string>& dependencies ) const
{
    for (auto tile : tiles_)
        tile->get_dependencies(dependencies);
    for (auto [e1, e2] : edge_mappings_){
        e1->get_dependencies(dependencies);
        e2->get_dependencies(dependencies);
    }
    with_.get_dependencies(dependencies);
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

    return std::make_shared<lay_expr>(tiles_simplified, mappings_simplified, with_.simplify());
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

