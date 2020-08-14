#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "allocator.h"
#include "variant_util.h"
#include "stack_machine.h"
#include "ops.h"
#include "geometry.h"
#include <variant>

namespace {

	std::vector<const tess::tile::impl_type*> get_neighbors(const tess::tile::impl_type* t) {
		int num_edges = static_cast<int>(t->edges().size());
		std::vector<const tess::tile::impl_type*> neighbors;
		for (int i = 0; i < num_edges; ++i) {
			const auto adj = t->get_adjacent_tile(i);
			if (adj != nullptr)
				neighbors.push_back(adj);
		}
		return neighbors;
	}

	bool has_broken_tile(const std::vector<tess::tile>& tiles) {
		return std::find_if(tiles.begin(), tiles.end(), 
			[](const tess::tile& tile) {
				const auto& edges = tile.edges();
				return std::find_if(edges.begin(), edges.end(),
					[](const tess::edge& e) {
						const auto* edge = tess::get_impl(e);
						return edge->has_property("broken");
					}
				) != edges.end();
			}
		) != tiles.end();
	}

	std::vector<tess::tile> join_broken_tiles(tess::allocator& a, const std::vector<tess::tile>& tiles) {
		if (!has_broken_tile(tiles))
			return tiles;
		return tiles;
	}
}

const std::vector<tess::tile>& tess::tile_patch::impl_type::tiles() const
{
    return tiles_;
}

void tess::tile_patch::impl_type::build_edge_table() const
{
	edge_tbl_.clear();
	for (const auto& tile : tiles_) {
		for (const auto& e : tile.edges()) {

			auto key = get_impl(e)->get_edge_location_indices();
			if (edge_tbl_.find(key) == edge_tbl_.end())
				edge_tbl_[key] = e; 
			else
				throw tess::error("invalid tile patch");
		}
	}
}

tess::tile_patch::impl_type::impl_type(obj_id id, std::vector<tess::tile>& tiles) : tessera_impl(id) {
	for ( auto& t : tiles)
		insert_tile(t);
}

void tess::tile_patch::impl_type::insert_tile( tess::tile& t )
{
	auto* tile = get_impl(t);
	tile->set_parent(this, static_cast<int>(tiles_.size()) );

	for (auto& v : tile->vertices()) {
		auto* vert = get_impl(v);
		int new_vert_index = vert_tbl_.insert(vert->pos());
		vert->set_location( new_vert_index );
	}

	tiles_.push_back(t);
}


void tess::tile_patch::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

tess::expr_value tess::tile_patch::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	auto iter = fields_.find(field);
	return (iter != fields_.end()) ?
		tess::expr_value{iter->second} :
		tess::expr_value{tess::error(std::string("referenced undefined tile patch field: ") + field)};
}

tess::expr_value tess::tile_patch::impl_type::get_ary_item(int i) const
{
	return { tiles_.at(i) };
}

int tess::tile_patch::impl_type::get_ary_count() const
{
	return static_cast<int>(tiles_.size());
}

void tess::tile_patch::impl_type::apply(const matrix& mat)
{
	vert_tbl_.apply_transformation(mat);
}

std::string tess::tile_patch::impl_type::debug() const
{
	std::stringstream ss;
	ss << "{\n";
	for (const auto& t : tiles_) {
		ss << "  " << get_impl(t)->debug() << "\n";
	}
	ss << "}\n\n";
	return ss.str();
}

tess::tile_patch tess::tile_patch::impl_type::flip(allocator& a) const {
	tess::tile_patch clone = a.create<tess::tile_patch>();
	std::unordered_map<obj_id, void*> tbl;
	clone_to(a, tbl, get_impl(clone));
	get_impl(clone)->flip();
	return clone;
}

void  tess::tile_patch::impl_type::flip()  {
	apply(flip_matrix());
	for (auto& tile : tiles_)
		for (edge& e : get_impl(tile)->edges())
			get_impl(e)->flip();
	edge_tbl_.clear();
}

std::optional<tess::edge> tess::tile_patch::impl_type::get_edge_on(int u, int v) const
{
	if (edge_tbl_.empty())
		build_edge_table();

	auto iter = edge_tbl_.find(edge_indices{ u, v });
	if (iter != edge_tbl_.end())
		return iter->second;
	else
		return std::nullopt;
}

tess::expr_value tess::tile_patch::impl_type::get_on(allocator& a, const std::variant<tess::edge, tess::cluster>& var) const
{
	return std::visit(
		overloaded{
			[&](const tess::edge& e) -> expr_value {
				auto maybe_edge = get_edge_on(
					vert_tbl_.get_index(e.u().pos()),
					vert_tbl_.get_index(e.v().pos())
				);
				if (maybe_edge.has_value()) {
					return { maybe_edge.value() };
				} else {
					return {};
				}
			},
			[&](const tess::cluster& c) -> expr_value {
				const auto& items = c.items();
				std::vector<tess::expr_value> on_edges(c.count());
				std::transform(items.begin(), items.end(), on_edges.begin(),
					[&](const expr_value& v) -> expr_value {
						std::variant<tess::edge, tess::cluster> edge_or_cluster = variant_cast(v);
						return this->get_on(a, edge_or_cluster);
					}
				);
				return { a.create<tess::cluster>(on_edges) };
			}
		},
		var
	);
}

void tess::tile_patch::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	for (const auto& tile : tiles_)
		expr_value{ tile }.get_all_referenced_allocations(alloc_set);

	for (const auto& [var, val] : fields_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::tile_patch::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, tile_patch::impl_type* clone) const
{
	for (const auto& t : tiles_) {
		clone->tiles_.push_back(std::get<tile>(expr_value{ t }.clone(allocator, orginal_to_clone)));
	}
	for (const auto& [var, val] : fields_) {
		clone->fields_[var] = val.clone(allocator, orginal_to_clone);
	}
	clone->vert_tbl_ = vert_tbl_;
}

tess::point tess::tile_patch::impl_type::get_vertex_location(int index) const {
	return vert_tbl_.get_location(index);
}

tess::tile tess::tile_patch::impl_type::join(tess::allocator& a) const
{
	auto points = tess::join(this);
	return a.create<tess::tile>(&a, points);
}

void tess::tile_patch::impl_type::dfs(tile_visitor visit) const
{
	std::unordered_set<const tile::impl_type*> visited;

	std::function<void(const tile& tile)> dfs_aux;
	dfs_aux = [&](const tile& tile) {
		const auto* t = get_impl(tile);
		if (visited.find(t) != visited.end())
			return;
		visited.insert(t);
		visit(tile);
		for (const auto* neighbor : get_neighbors(t)) {
			dfs_aux(make_tess_obj<tess::tile>(neighbor));
		}
	};
	dfs_aux(tiles_[0]);
}


/*---------------------------------------------------------------------------------------------*/

tess::cluster::impl_type::impl_type(obj_id id, const std::vector<expr_value>& values) :
	tessera_impl(id),
	values_(values)
{}


tess::expr_value tess::cluster::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	return expr_value();
}


void  tess::cluster::impl_type::insert_field(const std::string& var, const expr_value& val)
{
}

tess::expr_value tess::cluster::impl_type::get_ary_item(int i) const
{
	return values_.at(i);
}

void tess::cluster::impl_type::push_value(expr_value val)
{
	values_.push_back(val);
}

int tess::cluster::impl_type::get_ary_count() const
{
	return static_cast<int>(values_.size());
}

const std::vector<tess::expr_value>& tess::cluster::impl_type::values()
{
	return values_;
}

void tess::cluster::impl_type::get_all_referenced_allocations(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	for (const auto& val : values_)
		val.get_all_referenced_allocations(alloc_set);
}

void tess::cluster::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, cluster::impl_type* clone) const
{
	for (const auto& value : values_) {
		clone->values_.push_back(value.clone(allocator, orginal_to_clone));
	}
}

std::vector<tess::expr_value>::const_iterator tess::cluster::impl_type::begin() const
{
	return values_.cbegin();
}

std::vector<tess::expr_value>::const_iterator tess::cluster::impl_type::end() const
{
	return values_.cend();
}

int count_tiles(const std::vector<tess::expr_value>& tiles_and_patches) {
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

tess::tile_patch tess::flatten(tess::allocator& a, const std::vector<tess::expr_value>& tiles_and_patches) {
	int n = count_tiles(tiles_and_patches);
	std::vector<tess::tile> tiles;
	tiles.reserve(n);
	for (const auto& tile_or_patch : tiles_and_patches) {
		std::visit(
			overloaded{
				[&tiles](const tess::tile& t) { tiles.push_back(t); },
				[&tiles](const tess::tile_patch& patch) {
					for (auto t : patch.tiles()) {
						get_impl(t)->detach();
						tiles.push_back(t);
					}
				},
				[](auto) { throw tess::error("unknown error"); }
			},
			tile_or_patch
		);
	}
	tiles = join_broken_tiles(a, tiles);
	auto patch_impl = a.create_impl<tess::tile_patch>();
	for (const auto& tile : tiles) {
		auto copy = tess::clone(a, tile);
		patch_impl->insert_tile(copy);
	}
	return tess::make_tess_obj<tess::tile_patch>(patch_impl);
}
