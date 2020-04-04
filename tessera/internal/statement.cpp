#include "statement.h"
#include "script_impl.h"
#include "tile_patch_impl.h"
#include "tessera/error.h"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; }; 
template<class... Ts> overloaded(Ts...)->overloaded<Ts...>; 

template<typename T, typename... Ts, typename... Vs>
bool is_one_of(const std::variant<Vs...>& v) {
	if (std::holds_alternative<T>(v))
		return true;
	if constexpr (sizeof...(Ts) != 0)
		return is_one_of<Ts...>(v);
	else
		return false;
}

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

tess::lay_statement::lay_statement(const lay_params& params) :
    tiles_(params.tiles),
    such_that_clauses_( params.such_that_clauses )
{
}

tess::lay_statement::lay_statement(const std::vector<obj_ref_ptr>& tiles) :
    tiles_(tiles)
{
}

/*
tess::expr_value tess::lay_statement::execute( tess::execution_ctxt& ctxt ) const
{
    const auto& script = ctxt.script();
    auto tile_def = script.get_tile_prototype("triangle");
    auto val = tile_def->eval(ctxt);
    auto patch = make_tess_obj<tess::tile_patch> (
        std::make_shared<tess::tile_patch_impl>(
            std::vector<tess::tile>{ std::get<tess::tile>(val) }
        )
    );
    return { patch };
}
*/

tess::expr_value tess::lay_statement::execute(tess::execution_ctxt& ctxt) const
{
    std::vector<expr_value> pieces(tiles_.size());
    std::transform(tiles_.begin(), tiles_.end(), pieces.begin(),
        [&ctxt](const auto& piece) {
            return piece->eval(ctxt);
        }
    );

	for (const auto& val : pieces)
		if (!is_one_of<tile, tile_patch>(val))
			return std::holds_alternative<error>(val) ? 
				val : expr_value{ error("Can only lay tiles or patches.") };

    if (such_that_clauses_.empty()) {
		return tess::expr_value{
			flatten(pieces)
        };
    }



    return { nil_val() };
}

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
		std::make_shared<tile_patch_impl>(
			flatten_tiles_and_patches(tiles_n_patches)
		)
	);
}
