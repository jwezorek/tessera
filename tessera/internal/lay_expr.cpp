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

    tess::matrix edge_to_edge_matrix(const tess::edge::impl_type& e1, const tess::edge::impl_type& e2)
    {
        //TODO: move lay's implementation into the operator class and get rid of this.
        struct vert_helper : public tess::tessera_impl {
            tess::vertex::impl_type* get(const tess::vertex& v) {
                return get_impl(v);
            }
        };
        auto vert = vert_helper();
        auto u1 = vert.get(e1.u());
        auto v1 = vert.get(e1.v());
        auto u2 = vert.get(e2.u());
        auto v2 = vert.get(e2.v());

        return tess::line_seg_to_line_seg({ u1->pos() , v1->pos() }, { v2->pos() , u2->pos() });
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

    std::optional<tess::error> apply_edge_mapping(const std::tuple<tess::edge::impl_type*, tess::edge::impl_type*>& mapping)
    {
        auto [ptr_edge1, ptr_edge2] = mapping;
        auto edge1 = *ptr_edge1;
        auto edge2 = *ptr_edge2;
        auto parent_1 = parent_of_edge(edge1);
        auto parent_2 = parent_of_edge(edge2);

        if (parent_1 == parent_2)
            return tess::error("edge mapping internal to a single tile");

        bool both_are_touched = !is_untouched(parent_1) && !is_untouched(parent_2);
        bool both_are_untouched = is_untouched(parent_1) && is_untouched(parent_2);
        bool just_parent2_is_untouched = !is_untouched(parent_1) && is_untouched(parent_2);

        if (both_are_touched)
            return tess::error("TODO: tessera currently doesnt handle this kind of lay statement");

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
}

std::vector<tess::tile> tess::flatten_tiles_and_patches(const std::vector<tess::expr_value>& tiles_and_patches) {
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

std::optional<tess::error> tess::apply_mapping(const std::vector<std::tuple<edge::impl_type*, edge::impl_type*>>& mappings)
{
    for (const auto& mapping : mappings) {
        auto result = apply_edge_mapping(mapping);
        if (result.has_value()) {
            return { result.value() };
        }
    }
    return std::nullopt;
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
