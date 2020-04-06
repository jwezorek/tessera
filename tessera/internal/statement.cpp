#include "util.h"
#include "statement.h"
#include "script_impl.h"
#include "tile_patch_impl.h"
#include "tessera/error.h"

namespace {

    int countTiles(const std::vector<tess::expr_value>& tiles_and_patches) {
        int count = 0;
        for (const auto& tile_or_patch : tiles_and_patches)
            std::visit(
                overloaded{
                    [&count](const tess::tile& ) { ++count; },
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

}

tess::lay_statement::piece_result tess::lay_statement::eval_pieces(execution_ctxt& ctxt) const
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

tess::lay_statement::edge_mapping_result tess::lay_statement::eval_edge_mappings(execution_ctxt& ctxt) const
{
	std::vector< std::tuple<expr_value, expr_value>> edge_mapping_val_exprs( edge_mappings_.size() );
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
	mappings.reserve( edge_mappings_.size() );

	for (const auto& [val1, val2] : edge_mapping_val_exprs) {
		if (! is_one_of<edge, nil_val>(val1))
			return std::holds_alternative<error>(val1) ?
				std::get<error>(val1) : error("Can only lay tiles or patches.");
		if (!is_one_of<edge, nil_val>(val2))
			return std::holds_alternative<error>(val2) ?
				std::get<error>(val2) : error("Can only lay tiles or patches.");

		auto e1 = std::holds_alternative<edge>(val1) ? 
			opt_edge{std::get<edge>(val1)} :
			std::nullopt;
		auto e2 = std::holds_alternative<edge>(val2) ?
			opt_edge{ std::get<edge>(val2) } :
			std::nullopt;

		mappings.emplace_back(e1, e2);
	}

	
	return mappings;
}

std::optional<tess::error> tess::lay_statement::apply_mapping(const edge_mapping_value& mapping, execution_ctxt& ctxt) const
{
	auto [e1, e2] = mapping;
	return {};
}

tess::lay_statement::lay_statement(const lay_params& params) :
    tiles_(params.tiles),
    edge_mappings_( params.edge_mappings )
{
}

tess::lay_statement::lay_statement(const std::vector<obj_ref_ptr>& tiles) :
    tiles_(tiles)
{
}

tess::expr_value tess::lay_statement::execute( tess::execution_ctxt& ctxt ) const
{
    const auto& script = ctxt.script();
    auto tile_def = script.get_tile_prototype("triangle");
    auto val = tile_def->eval(ctxt);
    auto patch = make_tess_obj<tess::tile_patch> (
            std::vector<tess::tile>{ std::get<tess::tile>(val) }
    );
    return { patch };
}

/*
tess::expr_value tess::lay_statement::execute(tess::execution_ctxt& ctxt) const
{
	piece_result maybe_pieces;
	if ( maybe_pieces = eval_pieces(ctxt); std::holds_alternative<error>(maybe_pieces)) 
		return { std::get<error>(maybe_pieces) };
	auto pieces = std::move(std::get<expr_vals>(maybe_pieces));

    if (edge_mappings_.empty()) {
		return tess::expr_value{
			flatten(pieces)
        };
    }

	// push "placeholders' to the pieces on the stack
	scope scope(ctxt, scope_frame(pieces));
	
	edge_mapping_result maybe_mappings;
	if (maybe_mappings = eval_edge_mappings(ctxt); std::holds_alternative<error>(maybe_mappings))
		return { std::get<error>(maybe_mappings) };
	auto mappings = std::move(std::get<edge_mapping_values>(maybe_mappings));
	for (const auto& mapping : mappings) {
		auto result = apply_mapping(mapping, ctxt);
	}

    return { nil_val() };
}
*/

tess::if_statement::if_statement(const if_params& params) :
    condition_(params.condition),
    then_clause_(params.then_clause),
    else_clause_(params.else_clause)
{
}

tess::expr_value tess::if_statement::execute(tess::execution_ctxt& ctxt) const
{
    auto condition_val = condition_->eval(ctxt);
    if (std::holds_alternative<error>(condition_val))
        return condition_val;

    if (!std::holds_alternative<bool>(condition_val))
		return tess::expr_value{ error("if condition must evaluate to a boolean") };

    if (std::get<bool>(condition_val))
        return then_clause_->execute(ctxt);
    else
        return else_clause_->execute(ctxt);
}

tess::let_statement::let_statement(const std::tuple<std::string, expr_ptr> params) :
	lhs_( std::get<0>(params) ),
	rhs_( std::get<1>(params) )
{
}

tess::expr_value tess::let_statement::execute(execution_ctxt&) const
{
	return tess::expr_value{ nil_val() };
}

std::string tess::let_statement::lhs() const
{
	return lhs_;
}

tess::expr_ptr tess::let_statement::rhs() const
{
	return rhs_;
}

tess::tile_patch tess::statement::flatten(const std::vector<expr_value>& tiles_n_patches) const
{
	return make_tess_obj<tile_patch>(
		flatten_tiles_and_patches(tiles_n_patches)
	);
}
