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
    using edge_parent_type = std::variant<tess::tile::impl_type*, tess::tile_patch::impl_type*>;

    edge_parent_type parent_of_edge(const tess::edge::impl_type& e) {
        auto tile = e.parent();
        if (!tile->has_parent())
            return tile;
        else
            return tile->parent();
    }

    bool is_untouched(const edge_parent_type& ep) {
        return std::visit([](auto ptr) {return ptr->is_untouched(); }, ep);
    }

    void apply_edge_matrix(const edge_parent_type& ep, const tess::matrix& mat) {
        return std::visit([&mat](auto ptr) { ptr->apply(mat); }, ep);
    }

    int countTiles(const std::vector<tess::expr_value>& tiles_and_patches) {
        int count = 0;
        for (const auto& tile_or_patch : tiles_and_patches)
            std::visit(
                overloaded{
                    [&count](const tess::tile&) { ++count; },
                    [&count](const tess::tile_patch& patch) { count += patch.count(); },
                    [](auto) { throw tess::error("unknown error"); }
                },
                tile_or_patch
            );
        return count;
    }

    std::vector<tess::tile> flatten_tiles_and_patches(const std::vector<tess::expr_value>& tiles_and_patches) {
        int n = countTiles(tiles_and_patches);
        std::vector<tess::tile> tiles;
        tiles.reserve(n);
        for (const auto& tile_or_patch : tiles_and_patches)
            std::visit(
                overloaded{
                    [&tiles](const tess::tile& t) { tiles.push_back(t); },
                    [&tiles](const tess::tile_patch& patch) {
                        std::copy(patch.tiles().begin(), patch.tiles().end(), std::back_inserter(tiles));
                    },
                    [](auto) { throw tess::error("unknown error"); }
                },
                tile_or_patch
                        );
        return tiles;
    }

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

tess::lay_expr::piece_result tess::lay_expr::eval_pieces(evaluation_context& ctxt) const
{
    std::vector<expr_value> pieces(tiles_.size());
    std::transform(tiles_.begin(), tiles_.end(), pieces.begin(),
        [&ctxt](const auto& piece) {
            return piece->eval(ctxt);
        }
    );

    for (const auto& val : pieces) {
        if (!is_one_of<tile, tile_patch>(val)) {
            return std::holds_alternative<error>(val) ?
                std::get<error>(val) : error("Can only lay tiles or patches.");
        }
    }

    return pieces;
}

tess::lay_expr::edge_mapping_result tess::lay_expr::eval_edge_mappings(evaluation_context& ctxt) const
{
    std::vector< std::tuple<expr_value, expr_value>> edge_mapping_val_exprs(edge_mappings_.size());
    std::transform(edge_mappings_.begin(), edge_mappings_.end(), edge_mapping_val_exprs.begin(),
        [&ctxt](const auto& exprs)->std::tuple<expr_value, expr_value> {
            auto [e1, e2] = exprs;
            return {
                e1->eval(ctxt),
                e2->eval(ctxt)
            };
        }
    );

    edge_mapping_values mappings;
    mappings.reserve(edge_mappings_.size());

    for (const auto& [e1, e2] : edge_mapping_val_exprs) {
        if (!is_one_of<edge, nil_val>(e1))
            return std::holds_alternative<error>(e1) ?
            std::get<error>(e1) : error("Can only lay tiles or patches.");
        if (!is_one_of<edge, nil_val>(e2))
            return std::holds_alternative<error>(e2) ?
            std::get<error>(e2) : error("Can only lay tiles or patches.");

        if (!std::holds_alternative<edge>(e1))
            continue;
        if (!std::holds_alternative<edge>(e2))
            continue;

        mappings.emplace_back(std::get<edge>(e1), std::get<edge>(e2));
    }

    return mappings;
}

std::optional<tess::error> tess::lay_expr::apply_mapping(const edge_mapping_value& mapping, evaluation_context& ctxt) const
{
    auto [e1, e2] = mapping;
    auto& edge1 = *get_impl(e1);
    auto& edge2 = *get_impl(e2);
    auto parent_1 = parent_of_edge(edge1);
    auto parent_2 = parent_of_edge(edge2);

    if (parent_1 == parent_2)
        return error("edge mapping internal to a single tile");

    bool both_are_touched = !is_untouched(parent_1) && !is_untouched(parent_2);
    bool both_are_untouched = is_untouched(parent_1) && is_untouched(parent_2);
    bool just_parent2_is_untouched = !is_untouched(parent_1) && is_untouched(parent_2);

    if (both_are_touched)
        return error("TODO: tessera currently doesnt handle this kind of lay statement");

    if (both_are_untouched || just_parent2_is_untouched) {
        //move tile2
        apply_edge_matrix(parent_2, edge_to_edge_matrix(edge2, edge1));
    }
    else {
        //move tile1
        apply_edge_matrix(parent_1, edge_to_edge_matrix(edge1, edge2));
    }

    return std::nullopt;
}

tess::matrix tess::lay_expr::edge_to_edge_matrix(const edge::impl_type& e1, const edge::impl_type& e2) const
{
    auto u1 = get_impl(e1.u());
    auto v1 = get_impl(e1.v());
    auto u2 = get_impl(e2.u());
    auto v2 = get_impl(e2.v());

    return line_seg_to_line_seg({ u1->pos() , v1->pos() }, { v2->pos() , u2->pos() });
}

tess::lay_expr::lay_expr(const lay_params& params) :
    tiles_(params.tiles),
    edge_mappings_(params.edge_mappings)
{
}

tess::lay_expr::lay_expr(const std::vector<expr_ptr>& tiles) :
    tiles_(tiles)
{
}

tess::lay_expr::lay_expr(const std::vector<expr_ptr>& tiles, const std::vector<std::tuple<expr_ptr, expr_ptr>>& edge_mappings) :
    tiles_(tiles),
    edge_mappings_(edge_mappings)
{
}

tess::expr_value tess::lay_expr::eval(evaluation_context& ctxt) const
{
    piece_result maybe_pieces;
    if (maybe_pieces = eval_pieces(ctxt); std::holds_alternative<error>(maybe_pieces))
        return { std::get<error>(maybe_pieces) };
    auto pieces = std::move(std::get<expr_vals>(maybe_pieces));

    if (!edge_mappings_.empty()) {

        // push "placeholders' to the pieces on the stack
        lex_scope scope(ctxt, scope_frame(pieces));

        edge_mapping_result maybe_mappings;
        if (maybe_mappings = eval_edge_mappings(ctxt); std::holds_alternative<error>(maybe_mappings))
            return { std::get<error>(maybe_mappings) };
        auto mappings = std::move(std::get<edge_mapping_values>(maybe_mappings));
        for (const auto& mapping : mappings) {
            auto result = apply_mapping(mapping, ctxt);
            if (result.has_value()) {
                return { result.value() };
            }
        }
    }

    return {
        ctxt.allocator().create<tile_patch>(
            flatten_tiles_and_patches(pieces)
        )
    };
}

void tess::lay_expr::compile(stack_machine::stack& stack) const
{
    stack.push(std::make_shared<lay_op>(edge_mappings_.size()));
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
