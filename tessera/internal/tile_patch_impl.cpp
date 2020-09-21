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

	std::vector<const tess::edge::impl_type*> get_broken_edges(const std::vector<tess::tile>& tiles) {
		std::vector<const tess::edge::impl_type*> edges;
		for (const tess::tile& t : tiles) {
			for (const tess::edge& e : t.edges()) {
				const auto* edge = tess::get_impl(e);
				if (edge->has_property("broken")) {
					edges.push_back(edge);
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

	using tile_ptr = const tess::tile::impl_type*;
	std::vector<std::tuple<tile_ptr, tile_ptr>> get_broken_tiles(const std::vector<tess::tile>& tiles) {
		auto broken_edges = get_broken_edges(tiles);
		tess::vertex_location_table vert_tbl;
		tess::edge_table<tile_ptr> edge_tbl;
		std::vector<std::tuple<tile_ptr, tile_ptr>> output;
		for (const auto& e : broken_edges) {
			auto u = vert_tbl.insert(e->u().pos());
			auto v = vert_tbl.insert(e->v().pos());
			if (edge_tbl.find({ u,v }) != edge_tbl.end())
				throw tess::error("invalid tile patch while flattening (unable to auto join broekn tiles)");
			edge_tbl[{ u, v }] = e->parent();
			auto adj_edge = edge_tbl.find({ v,u });
			if (adj_edge != edge_tbl.end()) {
				output.emplace_back( tile_ptr(e->parent()), adj_edge->second  );
			} 
		}
		return output;
	}

	
	template<typename T>
	using assoc_map = boost::associative_property_map<T>;
	using rank_map = std::unordered_map<tile_ptr, int>;
	using parent_map = std::unordered_map<tile_ptr, tile_ptr>;
	using disjoint_sets = boost::disjoint_sets<assoc_map<rank_map>, assoc_map<parent_map>>;

	std::vector<std::vector<tess::tile>> get_broken_tile_groups( const std::vector<tess::tile>& tiles) {

		rank_map rank;
		parent_map parent;
		disjoint_sets ds(boost::make_assoc_property_map(rank), boost::make_assoc_property_map(parent));

		// insert all the vertices as single sets
		for (const auto& t : tiles) {
			ds.make_set( tess::get_impl(t) );
		}

		// add each graph edge to the data structure
		auto broken_tiles = get_broken_tiles(tiles);
		for (const auto [u, v] : broken_tiles) {
			ds.link(u, v);
		}

		// build a map mapping representatives to a elements...
		std::unordered_map<tile_ptr, std::vector<tile_ptr>> sets;
		for (const auto& tile : tiles) {
			auto v = tess::get_impl(tile);
			auto parent = ds.find_set(v);
			sets[parent].push_back(v);
		}

		// return the values of the above, with the tile impelemntation wrapped in real tiles.
		std::vector<std::vector<tess::tile>> output(sets.size());
		std::transform(sets.begin(), sets.end(), output.begin(),
			[](const std::unordered_map<tile_ptr, std::vector<tile_ptr>>::value_type& key_val) {
				const auto& tile_impls = key_val.second;
				std::vector<tess::tile> group(tile_impls.size());
				std::transform(tile_impls.begin(), tile_impls.end(), group.begin(),
					[](tile_ptr tile_impl) -> tess::tile {
						return tess::make_tess_obj<tess::tile>(tile_impl);
					}
				);
				return group;
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

	std::vector<tess::tile> join_broken_tiles(tess::allocator& a, const std::vector<tess::tile>& tiles) {
		if (!has_broken_tile(tiles))
			return tiles;

		auto grouped_tiles = get_broken_tile_groups(tiles);
		if (grouped_tiles.size() == tiles.size())
			return tiles;

		std::vector<tess::tile> output(grouped_tiles.size());
		std::transform(grouped_tiles.begin(), grouped_tiles.end(), output.begin(),
			[&a](const std::vector<tess::tile>& tile_group)->tess::tile {
				if (tile_group.size() == 1) {
					return tile_group.front();
				} else {
					return tess::join(a, tile_group);
				}
			}
		);

		return output;
	}

	void apply_fields(tess::tile tile, const tess::tile_patch::impl_type* patch)
	{
		std::unordered_map<std::string, tess::expr_value> fields;
		for (const auto& t : patch->tiles()) {
			for (const auto& [var, val] : tess::get_impl(t)->fields()) {
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
			tess::get_impl(tile)->insert_field(var, val);
		}
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
			//else
			//	throw tess::error("invalid tile patch");
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
	auto joined_patch = a.create<tess::tile>(&a, points);
	apply_fields(joined_patch, this);
	return joined_patch;
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

tess::tile_patch tess::flatten(tess::allocator& a, const std::vector<tess::expr_value>& tiles_and_patches, bool should_join_broken_tiles) {
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

	if (should_join_broken_tiles)
		tiles = join_broken_tiles(a, tiles);

	auto patch_impl = a.create_impl<tess::tile_patch>();
	for (const auto& tile : tiles) {
		auto copy = tess::clone(a, tile);
		patch_impl->insert_tile(copy);
	}

	return tess::make_tess_obj<tess::tile_patch>(patch_impl);
}

tess::tile tess::join(tess::allocator& a, const std::vector<tess::expr_value>& tiles_and_patches, bool should_join_broken_tiles) {
	auto patch = flatten(a, tiles_and_patches, should_join_broken_tiles);
	return tess::get_impl(patch)->join(a);
}

tess::tile tess::join(tess::allocator& a, const std::vector<tess::tile>& tiles) {
	std::vector<tess::expr_value> tiles_as_vals(tiles.size());
	std::transform(tiles.begin(), tiles.end(), tiles_as_vals.begin(),
		[](const tess::tile& t) -> tess::expr_value {
			return { t };
		}
	);
	return join(a, tiles_as_vals, false);
}
