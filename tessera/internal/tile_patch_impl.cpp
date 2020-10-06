#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "allocator.h"
#include "variant_util.h"
#include "stack_machine.h"
#include "ops.h"
#include "geometry.h"
#include "boost/pending/disjoint_sets.hpp"
#include "boost/property_map/property_map.hpp"
#include <variant>
#include <unordered_map>

namespace {

	std::vector<tess::const_tile_handle> get_neighbors(tess::const_tile_handle t) {
		int num_edges = static_cast<int>(t->edges().size());
		std::vector<tess::const_tile_handle> neighbors;
		for (int i = 0; i < num_edges; ++i) {
			const auto adj = t->get_adjacent_tile(i);
			if (adj != nullptr)
				neighbors.push_back(adj);
		}
		return neighbors;
	}

	bool has_broken_tile(const std::vector<tess::tile_handle>& tiles) {
		return std::find_if(tiles.begin(), tiles.end(), 
			[](tess::const_tile_handle tile) {
				const auto& edges = tile->edges();
				return std::find_if(edges.begin(), edges.end(),
					[](tess::const_edge_handle e) {
						return e->has_property("broken");
					}
				) != edges.end();
			}
		) != tiles.end();
	}

	std::vector<tess::const_edge_handle> get_broken_edges(const std::vector<tess::tile_handle>& tiles) {
		std::vector<tess::const_edge_handle> edges;
		for (tess::const_tile_handle t : tiles) {
			for (tess::const_edge_handle e : t->edges()) {
				if (e->has_property("broken")) {
					edges.push_back(e);
				}
			}
		}
		return edges;
	}

	std::string pt_to_string(std::tuple<tess::number, tess::number> pt) {
		std::stringstream ss;
		auto [x, y] = pt;
		ss << "( " << x << " , " << y << " )";
		return ss.str();
	}

	void debugTile(const tess::tile& tile) {
		for (const auto& v : tile.vertices()) {
			auto [x, y] = v.pos();
			std::cout << "( " << x << " , " << y << " ) ";
		}
	}

	std::vector<std::tuple<tess::const_tile_handle, tess::const_tile_handle>> get_broken_tiles(const std::vector<tess::tile_handle>& tiles) {
		auto broken_edges = get_broken_edges(tiles);
		tess::vertex_location_table vert_tbl;
		tess::edge_table<tess::const_tile_handle> edge_tbl;
		std::vector<std::tuple<tess::const_tile_handle, tess::const_tile_handle>> output;
		for (const auto& e : broken_edges) {
			auto u = vert_tbl.insert(e->u()->pos());
			auto v = vert_tbl.insert(e->v()->pos());
			if (edge_tbl.find({ u,v }) != edge_tbl.end())
				throw tess::error("invalid tile patch while flattening (unable to auto join broekn tiles)");
			edge_tbl[{ u, v }] = e->parent();
			auto adj_edge = edge_tbl.find({ v,u });
			if (adj_edge != edge_tbl.end()) {
				output.emplace_back(tess::const_tile_handle(e->parent()), adj_edge->second  );
			} 
		}
		return output;
	}

	
	template<typename T>
	using assoc_map = boost::associative_property_map<T>;
	using rank_map = std::unordered_map<tess::const_tile_handle, int>;
	using parent_map = std::unordered_map<tess::const_tile_handle, tess::const_tile_handle>;
	using disjoint_sets = boost::disjoint_sets<assoc_map<rank_map>, assoc_map<parent_map>>;

	std::vector<std::vector<tess::const_tile_handle>> get_broken_tile_groups( const std::vector<tess::tile_handle>& tiles) {

		rank_map rank;
		parent_map parent;
		disjoint_sets ds(boost::make_assoc_property_map(rank), boost::make_assoc_property_map(parent));

		// insert all the vertices as single sets
		for (const auto* t : tiles) {
			ds.make_set( t );
		}

		// add each graph edge to the data structure
		auto broken_tiles = get_broken_tiles(tiles);
		for (const auto [u, v] : broken_tiles) {
			ds.link(u, v);
		}

		// build a map mapping representatives to a elements...
		std::unordered_map<tess::const_tile_handle, std::vector<tess::const_tile_handle>> sets;
		for (const auto* tile : tiles) {
			auto v = tile;
			auto parent = ds.find_set(v);
			sets[parent].push_back(v);
		}

		// return the values of the above, with the tile impelemntation wrapped in real tiles.
		std::vector<std::vector<tess::const_tile_handle>> output(sets.size());
		std::transform(sets.begin(), sets.end(), output.begin(),
			[](const std::unordered_map<tess::const_tile_handle, std::vector<tess::const_tile_handle>>::value_type& key_val) {
				const std::vector<tess::const_tile_handle>& val = key_val.second;
				return val;
			}
		);

		return output;
	}

	void debugTileGroup(const std::vector<tess::tile>& g) {
		std::cout << "[\n";
		for (auto t : g) {
			std::cout << "  ";
			debugTile(t);
			std::cout << "\n";
		}
		std::cout << "]\n";
	}

	std::vector<tess::tile_handle> join_broken_tiles(tess::allocator& a, const std::vector<tess::tile_handle>& tiles) {
		if (!has_broken_tile(tiles))
			return tiles;

		auto grouped_tiles = get_broken_tile_groups(tiles);
		if (grouped_tiles.size() == tiles.size())
			return tiles;

		std::vector<tess::tile_handle> output(grouped_tiles.size());
		std::transform(grouped_tiles.begin(), grouped_tiles.end(), output.begin(),
			[&a](const auto& tile_group)->tess::tile_handle {
				if (tile_group.size() == 1) {
					return const_cast<tess::tile_handle>( tile_group.front() );
				} else {
					return tess::join(a, tile_group);
				}
			}
		);

		return output;
	}

	/*
	void propagate_edge_fields(tess::tile tile, const_patch_handle patch)
	{
		for (auto& e : tess::get_impl(tile)->edges()) {
			auto maybe_on_edge = patch->get_edge_on(tess::get_impl(e.u())->pos(), tess::get_impl(e.v())->pos());
			if (maybe_on_edge.has_value()) {
				auto on_edge = maybe_on_edge.value();
				for (const auto& [var, val] : tess::get_impl(on_edge)->fields())
					if (val.is_simple_value())
						tess::get_impl(e)->insert_field(var, val);
			}
		}
	}
	*/

	void propagate_edge_fields(tess::tile_handle tile, tess::const_patch_handle patch)
	{
		tess::edge_location_table edges;
		for (const auto* t : patch->tiles())
			for (const auto* e : t->edges())
				edges.insert(e);
		for (auto* edge : tile->edges()) {
			auto joined_edges = edges.get(edge);

			std::unordered_map<std::string, tess::expr_value> fields;
			for (const auto* e : joined_edges) {
				for (const auto& [var, val] : e->fields()) {
					if (val.is_simple_value()) {
						if (fields.find(var) == fields.end()) {
							fields[var] = val;
						}
						else {
							if (fields[var] != val)
								fields.erase(var);
						}
					}
				}
			}
			for (const auto& [var, val] : fields) {
				edge->insert_field(var, val);
			}
		}
	}

	void propagate_fields(tess::tile_handle tile, tess::const_patch_handle patch)
	{
		std::unordered_map<std::string, tess::expr_value> fields;
		for (const auto& t : patch->tiles()) {
			for (const auto& [var, val] : t->fields()) {
				if (val.is_simple_value()) {
					if (fields.find(var) == fields.end()) {
						fields[var] = val;
					} else {
						if (fields[var] != val)
							fields.erase(var);
					}
				}
			}
		}
		for (const auto& [var, val] : fields) {
			tile->insert_field(var, val);
		}

		propagate_edge_fields(tile, patch);
	}
}

const std::vector<tess::tile_handle>& tess::tile_patch::impl_type::tiles() const
{
    return tiles_;
}

void tess::tile_patch::impl_type::build_edge_table() const
{
	edge_tbl_.clear();
	for (const auto* tile : tiles_) {
		for (const auto* e : tile->edges()) {

			auto key = e->get_edge_location_indices();
			if (edge_tbl_.find(key) == edge_tbl_.end())
				edge_tbl_[key] = e; 
			else
				throw tess::error("invalid tile patch");
		}
	}
}

tess::tile_patch::impl_type::impl_type(obj_id id, const std::vector<tess::tile_handle>& tiles) : tessera_impl(id) {
	for ( auto& t : tiles)
		insert_tile(t);
}

void tess::tile_patch::impl_type::insert_tile( tess::tile_handle tile )
{
	tile->set_parent(this, static_cast<int>(tiles_.size()) );

	for (auto& v : tile->vertices()) {
		auto* vert = v;
		int new_vert_index = vert_tbl_.insert(vert->pos());
		vert->set_location( new_vert_index );
	}

	tiles_.push_back(tile);
}


void tess::tile_patch::impl_type::insert_field(const std::string& var, const expr_value& val)
{
	fields_[var] = val;
}

tess::expr_value tess::tile_patch::impl_type::get_field(allocator& allocator, const std::string& field) const
{
	auto iter = fields_.find(field);

	if (iter == fields_.end())
		throw tess::error(std::string("referenced undefined tile patch field: ") + field);

	return iter->second;
}

tess::expr_value tess::tile_patch::impl_type::get_ary_item(int i) const
{
	return expr_value(tiles_.at(i));
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
		ss << "  " << t->debug() << "\n";
	}
	ss << "}\n\n";
	return ss.str();
}

tess::patch_handle tess::tile_patch::impl_type::flip(allocator& a) const {
	expr_value self = expr_value( this );
	auto* clone = std::get<tess::patch_handle>(self.clone(a));
	clone->flip();
	return clone;
}

void  tess::tile_patch::impl_type::flip()  {
	apply(flip_matrix());
	for (auto& tile : tiles_)
		for (auto* e : tile->edges())
			e->flip();
	edge_tbl_.clear();
}

tess::const_edge_handle tess::tile_patch::impl_type::get_edge_on(int u, int v) const
{
	if (edge_tbl_.empty())
		build_edge_table();

	auto iter = edge_tbl_.find(edge_indices{ u, v });
	if (iter != edge_tbl_.end())
		return iter->second;
	else
		return nullptr;
}


tess::const_edge_handle tess::tile_patch::impl_type::get_edge_on(tess::point u, tess::point v) const {
	auto u_index = vert_tbl_.get_index(u);
	auto v_index = vert_tbl_.get_index(v);
	return get_edge_on(u_index, v_index);
}

tess::expr_value tess::tile_patch::impl_type::get_on(allocator& a, const std::variant<tess::edge_handle, tess::cluster_handle>& var) const
{
	return std::visit(
		overloaded{
			[&](tess::const_edge_handle e) -> expr_value {
				auto maybe_edge = get_edge_on( e->u()->pos(), e->v()->pos());
				if (maybe_edge) {
					return expr_value(maybe_edge);
				} else {
					return {};
				}
			},
			[&](const tess::cluster_handle c) -> expr_value {
				const auto& items = c->items();
				std::vector<tess::expr_value> on_edges(c->items().size());
				std::transform(items.begin(), items.end(), on_edges.begin(),
					[&](const expr_value& v) -> expr_value {
						std::variant<tess::edge_handle, tess::cluster_handle> edge_or_cluster = variant_cast(v);
						return this->get_on(a, edge_or_cluster);
					}
				);
				return expr_value(a.create<tess::cluster_handle>(on_edges) );
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

void tess::tile_patch::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, patch_handle clone) const
{
	for (const auto& t : tiles_) {
		auto t_clone = std::get<tile_handle>(expr_value{ t }.clone(allocator, orginal_to_clone));
		clone->tiles_.push_back( t_clone );
	}
	for (const auto& [var, val] : fields_) {
		clone->fields_[var] = val.clone(allocator, orginal_to_clone);
	}
	clone->vert_tbl_ = vert_tbl_;
}

tess::point tess::tile_patch::impl_type::get_vertex_location(int index) const {
	return vert_tbl_.get_location(index);
}

tess::tile_handle tess::tile_patch::impl_type::join(tess::allocator& a) const
{
	auto points = tess::join(this);
	auto joined_patch = a.create<tess::tile_handle>(&a, points);
	propagate_fields(joined_patch, this);
	return joined_patch;
}

void tess::tile_patch::impl_type::dfs(tile_visitor visit) const
{
	std::unordered_set<tess::const_tile_handle> visited;

	std::function<void(const_tile_handle tile)> dfs_aux;
	dfs_aux = [&](const_tile_handle t) {
		if (visited.find(t) != visited.end())
			return;
		visited.insert(t);
		visit(t);
		for (const auto* neighbor : get_neighbors(t)) {
			dfs_aux(neighbor);
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
	if (i < 0 || i >= values_.size())
		throw tess::error("array ref out of bounds");
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

void tess::cluster::impl_type::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, cluster_handle clone) const
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


const std::vector<tess::expr_value>& tess::cluster::impl_type::items() const
{
	return values_;
}

int count_tiles(const std::vector<tess::expr_value>& tiles_and_patches) {
	int count = 0;
	for (const auto& tile_or_patch : tiles_and_patches)
		std::visit(
			overloaded{
				[&count]( tess::tile_handle) { ++count; },
				[&count]( tess::patch_handle patch) { count += patch->tiles().size(); },
				[](auto) { throw tess::error("unknown error"); }
			},
			tile_or_patch
		);
	return count;
}

tess::patch_handle tess::flatten(tess::allocator& a, const std::vector<tess::expr_value>& tiles_and_patches, bool should_join_broken_tiles) {
	for (const auto& v : tiles_and_patches)
		if (!std::holds_alternative<tess::tile_handle>(v) && !std::holds_alternative<tess::patch_handle>(v))
			throw tess::error("attempted to flatten a value that is not a tile or tile patch");
	int n = count_tiles(tiles_and_patches);
	std::vector<tess::tile_handle> tiles;
	tiles.reserve(n);
	for (const auto& tile_or_patch : tiles_and_patches) {
		std::visit(
			overloaded{
				[&tiles](tess::tile_handle t) { tiles.push_back(t); },
				[&tiles](tess::patch_handle patch) {
					for (auto* t : patch->tiles()) {
						t->detach();
						tiles.push_back(t);
					}
				},
				[](auto) { throw tess::error("unknown error"); }
			},
			tile_or_patch
		);
	}

	if (should_join_broken_tiles)
		tiles = join_broken_tiles(a, tiles);

	auto patch_impl = a.create<tess::patch_handle>();
	for (const auto& tile : tiles) {
		auto* copy = tess::clone(a, tile);
		patch_impl->insert_tile( copy );
	}

	return patch_impl;
}

tess::tile_handle tess::join(tess::allocator& a, const std::vector<tess::expr_value>& tiles_and_patches, bool should_join_broken_tiles) {
	auto patch = flatten(a, tiles_and_patches, should_join_broken_tiles);
	return patch->join(a);
}

tess::tile_handle tess::join(tess::allocator& a, const std::vector<const_tile_handle>& tiles) {
	std::vector<tess::expr_value> tiles_as_vals(tiles.size());
	std::transform(tiles.begin(), tiles.end(), tiles_as_vals.begin(),
		[](const_tile_handle t) -> tess::expr_value {
			return expr_value(t);
		}
	);
	return join(a, tiles_as_vals, false);
}
