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

	std::vector<tess::const_tile_ptr> get_neighbors(tess::const_tile_ptr t) {
		int num_edges = static_cast<int>(t->edges().size());
		std::vector<tess::const_tile_ptr> neighbors;
		for (int i = 0; i < num_edges; ++i) {
			const auto adj = t->get_adjacent_tile(i);
			if (adj != nullptr)
				neighbors.push_back(adj);
		}
		return neighbors;
	}

	bool has_broken_tile(const std::vector<tess::tile_ptr>& tiles) {
		return std::find_if(tiles.begin(), tiles.end(), 
			[](tess::const_tile_ptr tile) {
				const auto& edges = tile->edges();
				return std::find_if(edges.begin(), edges.end(),
					[](tess::const_edge_ptr e) {
						return e->has_property("broken");
					}
				) != edges.end();
			}
		) != tiles.end();
	}

	std::vector<tess::edge_ptr> get_broken_edges(const std::vector<tess::tile_ptr>& tiles) {
		std::vector<tess::edge_ptr> edges;
		for (tess::tile_ptr t : tiles) {
			for (tess::edge_ptr e : t->edges()) {
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

	std::vector<std::tuple<tess::tile_ptr, tess::tile_ptr>> get_broken_tiles(const std::vector<tess::tile_ptr>& tiles) {
		auto broken_edges = get_broken_edges(tiles);
		tess::vertex_location_table vert_tbl;
		tess::edge_table<tess::tile_ptr> edge_tbl;
		std::vector<std::tuple<tess::tile_ptr, tess::tile_ptr>> output;
		for ( auto& e : broken_edges) {
			auto u = vert_tbl.insert(e->u()->pos());
			auto v = vert_tbl.insert(e->v()->pos());
			if (edge_tbl.find({ u,v }) != edge_tbl.end())
				throw tess::error("invalid tile patch while flattening (unable to auto join broekn tiles)");
			edge_tbl[{ u, v }] = e->parent();
			auto adj_edge = edge_tbl.find({ v,u });
			if (adj_edge != edge_tbl.end()) {
				output.emplace_back(tess::tile_ptr(e->parent()), adj_edge->second  );
			} 
		}
		return output;
	}
	
	template<typename T>
	using assoc_map = boost::associative_property_map<T>;
	using rank_map = std::unordered_map<tess::tile_ptr, int>;
	using parent_map = std::unordered_map<tess::tile_ptr, tess::tile_ptr>;
	using disjoint_sets = boost::disjoint_sets<assoc_map<rank_map>, assoc_map<parent_map>>;

	std::vector<std::vector<tess::tile_ptr>> get_broken_tile_groups( const std::vector<tess::tile_ptr>& tiles) {

		rank_map rank;
		parent_map parent;
		disjoint_sets ds(boost::make_assoc_property_map(rank), boost::make_assoc_property_map(parent));

		// insert all the vertices as single sets
		for ( auto* t : tiles) {
			ds.make_set( t );
		}

		// add each graph edge to the data structure
		auto broken_tiles = get_broken_tiles(tiles);
		for ( auto [u, v] : broken_tiles) {
			ds.link(u, v);
		}

		// build a map mapping representatives to a elements...
		std::unordered_map<tess::tile_ptr, std::vector<tess::tile_ptr>> sets;
		for ( auto* tile : tiles) {
			auto v = tile;
			auto parent = ds.find_set(v);
			sets[parent].push_back(v);
		}

		// return the values of the above, with the tile impelemntation wrapped in real tiles.
		std::vector<std::vector<tess::tile_ptr>> output(sets.size());
		std::transform(sets.begin(), sets.end(), output.begin(),
			[](const std::unordered_map<tess::tile_ptr, std::vector<tess::tile_ptr>>::value_type& key_val) {
				const std::vector<tess::tile_ptr>& val = key_val.second;
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

	std::vector<tess::tile_ptr> join_broken_tiles(tess::allocator& a, const std::vector<tess::tile_ptr>& tiles) {
		if (!has_broken_tile(tiles))
			return tiles;

		auto grouped_tiles = get_broken_tile_groups(tiles);
		if (grouped_tiles.size() == tiles.size())
			return tiles;

		std::vector<tess::tile_ptr> output(grouped_tiles.size());
		std::transform(grouped_tiles.begin(), grouped_tiles.end(), output.begin(),
			[&a]( auto& tile_group)->tess::tile_ptr {
				if (tile_group.size() == 1) {
					return tess::clone(a, tile_group.front() );
				} else {
					return tess::join(a, tile_group);
				}
			}
		);

		return output;
	}

	void propagate_edge_fields(tess::tile_ptr tile, tess::const_patch_ptr patch)
	{
		tess::edge_location_table edges;
		for (const auto* t : patch->tiles())
			for (const auto* e : t->edges())
				edges.insert(e);
		for (auto* edge : tile->edges()) {
			auto joined_edges = edges.get(edge);

			std::unordered_map<std::string, tess::value_> fields;
			for (const auto* e : joined_edges) {
				for (const auto& [var, val] : e->fields()) {
					if (tess::is_simple_value(val)) {
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

	void propagate_fields(tess::tile_ptr tile, tess::const_patch_ptr patch)
	{
		std::unordered_map<std::string, tess::value_> fields;
		for (const auto& t : patch->tiles()) {
			for (const auto& [var, val] : t->fields()) {
				if (tess::is_simple_value(val)) {
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

const std::vector<tess::tile_ptr>& tess::detail::patch_impl::tiles()
{
	return tiles_;
}

std::vector<tess::const_tile_ptr> tess::detail::patch_impl::tiles() const
{
	std::vector<tess::const_tile_ptr> const_tiles(tiles_.size());
	std::transform(tiles_.begin(), tiles_.end(), const_tiles.begin(),
		[](auto t) { return t; }
	);
    return const_tiles;
}

void tess::detail::patch_impl::build_edge_table() const
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

tess::detail::patch_impl::patch_impl(obj_id id, const std::vector<tess::tile_ptr>& tiles) : tessera_impl(id) {
	for ( auto& t : tiles)
		insert_tile(t);
}


void tess::detail::patch_impl::insert_tile( tess::tile_ptr tile )
{
	tile->set_parent(this, static_cast<int>( tiles_.size()) );

	for (auto& v : tile->vertices()) {
		auto* vert = v;
		int new_vert_index = vert_tbl_.insert(vert->pos());
		vert->set_location( new_vert_index );
	}

	tiles_.push_back(tile);
}

int tess::detail::patch_impl::count() const
{
	return static_cast<int>(tiles_.size());
}


void tess::detail::patch_impl::insert_field(const std::string& var, const value_& val)
{
	fields_[var] = val;
}

tess::value_ tess::detail::patch_impl::get_field(allocator& allocator, const std::string& field) const
{
	auto iter = fields_.find(field);

	if (iter == fields_.end())
		throw tess::error(std::string("referenced undefined tile patch field: ") + field);

	return iter->second;
}

tess::value_ tess::detail::patch_impl::get_ary_item(int i) const
{
	return value_(tiles_.at(i));
}

int tess::detail::patch_impl::get_ary_count() const
{
	return static_cast<int>(tiles_.size());
}

void tess::detail::patch_impl::apply(const matrix& mat)
{ 
	vert_tbl_.apply_transformation(mat);
}

std::string tess::detail::patch_impl::debug() const
{
	std::stringstream ss;
	ss << "{\n";
	for (const auto& t : tiles_) {
		ss << "  " << t->debug() << "\n";
	}
	ss << "}\n\n";
	return ss.str();
}

tess::patch_ptr tess::detail::patch_impl::flip(allocator& a) const {
	value_ self = value_( this );
	auto* clone = get_mutable<tess::const_patch_ptr>(tess::clone_value(a, self));
	clone->flip();
	return clone;
}

void  tess::detail::patch_impl::flip()  {
	apply(flip_matrix());
	for (auto& tile : tiles_)
		for (auto* e : tile->edges())
			e->flip();
	edge_tbl_.clear();
}

tess::const_edge_ptr tess::detail::patch_impl::get_edge_on(int u, int v) const
{
	if (edge_tbl_.empty())
		build_edge_table();

	auto iter = edge_tbl_.find(edge_indices{ u, v });
	if (iter != edge_tbl_.end())
		return iter->second;
	else
		return nullptr;
}

tess::const_edge_ptr tess::detail::patch_impl::get_edge_on(tess::point u, tess::point v) const {
	auto u_index = vert_tbl_.get_index(u);
	auto v_index = vert_tbl_.get_index(v);
	return get_edge_on(u_index, v_index);
}

tess::value_ tess::detail::patch_impl::get_on(allocator& a, const std::variant<tess::const_edge_ptr, tess::const_cluster_ptr>& var) const
{
	return std::visit(
		overloaded{
			[&](tess::const_edge_ptr e) -> value_ {
				auto maybe_edge = get_edge_on( e->u()->pos(), e->v()->pos());
				if (maybe_edge) {
					return value_(maybe_edge);
				} else {
					return {};
				}
			},
			[&](const tess::const_cluster_ptr c) -> value_ {
				const auto& items = c->items();
				std::vector<tess::value_> on_edges(c->items().size());
				std::transform(items.begin(), items.end(), on_edges.begin(),
					[&](const value_& v) -> value_ {
						std::variant<tess::const_edge_ptr, tess::const_cluster_ptr> edge_or_cluster = variant_cast(v);
						return this->get_on(a, edge_or_cluster);
					}
				);
				return value_(a.create<tess::const_cluster_ptr>(on_edges) );
			}
		},
		var
	);
}

void tess::detail::patch_impl::get_references(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;

	alloc_set.insert(key); // self

	for (const auto& tile : tiles_) // tiles
        tess::get_references(value_{tile}, alloc_set);

	for (const auto& [var, val] : fields_) // fields
        tess::get_references(val, alloc_set);
}

void tess::detail::patch_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, patch_ptr mutable_clone) const
{
	for (const auto& t : tiles_) { // clone tiles
		auto t_clone = get_mutable<const_tile_ptr>(tess::clone_value(allocator, orginal_to_clone, value_{ t }));
		mutable_clone->tiles_.push_back( t_clone );
	}
	for (const auto& [var, val] : fields_) { // clone fields
		mutable_clone->fields_[var] = tess::clone_value(allocator, orginal_to_clone, val);
	}
	mutable_clone->vert_tbl_ = vert_tbl_;
}

tess::point tess::detail::patch_impl::get_vertex_location(int index) const {
	return vert_tbl_.get_location(index);
}

tess::tile_ptr tess::detail::patch_impl::join(tess::allocator& a) const
{
	auto points = tess::join(this);
	auto joined_patch = a.create<tess::const_tile_ptr>(&a, points);
	propagate_fields(joined_patch, this);
	return joined_patch;
}

void tess::detail::patch_impl::dfs(tile_visitor visit) const
{
	std::unordered_set<tess::const_tile_ptr> visited;

	std::function<void(const_tile_ptr tile)> dfs_aux;
	dfs_aux = [&](const_tile_ptr t) {
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

tess::detail::cluster_impl::cluster_impl(obj_id id, const std::vector<value_>& values) :
	tessera_impl(id),
	values_(values)
{}


tess::value_ tess::detail::cluster_impl::get_field(allocator& allocator, const std::string& field) const
{
	return value_();
}


void  tess::detail::cluster_impl::insert_field(const std::string& var, const value_& val)
{
}

tess::value_ tess::detail::cluster_impl::get_ary_item(int i) const
{
	if (i < 0 || i >= values_.size())
		throw tess::error("array ref out of bounds");
	return values_.at(i);
}

void tess::detail::cluster_impl::push_value(value_ val)
{
	values_.push_back(val);
}

int tess::detail::cluster_impl::get_ary_count() const
{
	return static_cast<int>(values_.size());
}

const std::vector<tess::value_>& tess::detail::cluster_impl::values() const
{
	return values_;
}

void tess::detail::cluster_impl::get_references(std::unordered_set<obj_id>& alloc_set) const
{
	auto key = get_id();
	if (alloc_set.find(key) != alloc_set.end())
		return;
	alloc_set.insert(key);

	for (const auto& val : values_)
        tess::get_references(val, alloc_set);
}

void tess::detail::cluster_impl::clone_to(tess::allocator& allocator, std::unordered_map<obj_id, void*>& orginal_to_clone, cluster_ptr mutable_clone) const
{
	for (const auto& value : values_) {
		mutable_clone->values_.push_back(tess::clone_value(allocator, orginal_to_clone, value));
	}
}

std::vector<tess::value_>::const_iterator tess::detail::cluster_impl::begin() const
{
	return values_.cbegin();
}

std::vector<tess::value_>::const_iterator tess::detail::cluster_impl::end() const
{
	return values_.cend();
}


const std::vector<tess::value_>& tess::detail::cluster_impl::items() const
{
	return values_;
}

int count_tiles(const std::vector<tess::value_>& tiles_and_patches) {
	int count = 0;
	for (const auto& tile_or_patch : tiles_and_patches)
		std::visit(
			overloaded{
				[&count]( tess::const_tile_ptr) { ++count; },
				[&count]( tess::const_patch_ptr patch) { count += patch->count(); },
				[](auto) { throw tess::error("unknown error"); }
			},
			tile_or_patch
		);
	return count;
}

tess::patch_ptr tess::flatten(tess::allocator& a, const std::vector<tess::value_>& tiles_and_patches, bool should_join_broken_tiles) {
	for (const auto& v : tiles_and_patches)
		if (!std::holds_alternative<tess::const_tile_ptr>(v) && !std::holds_alternative<tess::const_patch_ptr>(v))
			throw tess::error("attempted to flatten a value that is not a tile or tile patch");
	int n = count_tiles(tiles_and_patches);
	std::vector<tess::tile_ptr> tiles;
	tiles.reserve(n);
	for (const auto& tile_or_patch : tiles_and_patches) {
		std::visit(
			overloaded{
				[&](tess::const_tile_ptr t) { tiles.push_back( tess::clone(a, t) ); },
				[&](tess::const_patch_ptr patch) {
					for (auto* t : patch->tiles()) {
						auto clone = tess::clone(a, t);
						clone->detach();
						tiles.push_back(clone);
					}
				},
				[](auto) { throw tess::error("unknown error"); }
			},
			tile_or_patch
		);
	}

	if (should_join_broken_tiles)
		tiles = join_broken_tiles(a, tiles);

	auto patch_impl = a.create<tess::const_patch_ptr>();
	for (const auto& tile : tiles) {
		patch_impl->insert_tile( tile );
	}

	return patch_impl;
}

tess::tile_ptr tess::join(tess::allocator& a, const std::vector<tess::value_>& tiles_and_patches, bool should_join_broken_tiles) {
	auto patch = flatten(a, tiles_and_patches, should_join_broken_tiles);
	return patch->join(a);
}

tess::tile_ptr tess::join(tess::allocator& a, const std::vector<tile_ptr>& tiles) {
	std::vector<tess::value_> tiles_as_vals(tiles.size());
	std::transform(tiles.begin(), tiles.end(), tiles_as_vals.begin(),
		[](tile_ptr t) -> tess::value_ {
			return value_(t);
		}
	);
	return join(a, tiles_as_vals, false);
}
