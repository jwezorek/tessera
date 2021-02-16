#include "tile_impl.h"
#include "tile_patch_impl.h"
#include "lambda_impl.h"
#include "gc_heap.h"
#include "variant_util.h"
#include "stack_machine.h"
#include "ops.h"
#include "geometry.h"
#include "boost/pending/disjoint_sets.hpp"
#include "boost/property_map/property_map.hpp"
#include <variant>
#include <unordered_map>

namespace {

	std::vector<tess::const_tile_root_ptr> get_neighbors(const tess::const_tile_root_ptr& t) {
		int num_edges = static_cast<int>(t->end_edges() - t->begin_edges());
		std::vector<tess::const_tile_root_ptr> neighbors;
		for (int i = 0; i < num_edges; ++i) {
			const auto adj = t->get_adjacent_tile(i);
			if (adj)
				neighbors.push_back(adj);
		}
		return neighbors;
	}

	bool has_broken_tile(const std::vector<tess::tile_root_ptr>& tiles) {
		return std::find_if(tiles.begin(), tiles.end(), 
			[](tess::tile_root_ptr tile) {
				return std::find_if(tile->begin_edges(), tile->end_edges(),
					[](const auto& e) {
						return e->has_property("broken");
					}
				) != tile->end_edges();
			}
		) != tiles.end();
	}

	std::vector<tess::edge_root_ptr> get_broken_edges(const std::vector<tess::tile_root_ptr>& tiles) {
		std::vector<tess::edge_root_ptr> edges;
		for (tess::tile_root_ptr t : tiles) {
			for (auto iter = t->begin_edges(); iter != t->end_edges(); ++iter) {
				auto e = to_root_ptr(*iter);
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

	std::vector<std::tuple<tess::tile_root_ptr, tess::tile_root_ptr>> get_broken_tiles(const std::vector<tess::tile_root_ptr>& tiles) {
		auto broken_edges = get_broken_edges(tiles);
		tess::vertex_location_table vert_tbl;
		tess::edge_table<tess::tile_root_ptr> edge_tbl;
		std::vector<std::tuple<tess::tile_root_ptr, tess::tile_root_ptr>> output;
		for ( auto& e : broken_edges) {
			auto u = vert_tbl.insert(e->u()->pos());
			auto v = vert_tbl.insert(e->v()->pos());
			if (edge_tbl.find({ u,v }) != edge_tbl.end())
				throw tess::error("invalid tile patch while flattening (unable to auto join broekn tiles)");
			edge_tbl[{ u, v }] = e->parent();
			auto adj_edge = edge_tbl.find({ v,u });
			if (adj_edge != edge_tbl.end()) {
				output.emplace_back(tess::tile_root_ptr(e->parent()), adj_edge->second  );
			} 
		}
		return output;
	}
	
	template<typename T>
	using assoc_map = boost::associative_property_map<T>;
	using rank_map = std::unordered_map<tess::obj_id, int>;
	using parent_map = std::unordered_map<tess::obj_id, tess::obj_id>;
	using disjoint_sets = boost::disjoint_sets<assoc_map<rank_map>, assoc_map<parent_map>>;

	std::vector<std::vector<tess::tile_root_ptr>> get_broken_tile_groups( const std::vector<tess::tile_root_ptr>& tiles) {

	    std::unordered_map<tess::obj_id, tess::tile_root_ptr> id_to_tile;
	    id_to_tile.reserve((tiles.size()));

		rank_map rank;
		parent_map parent;
		disjoint_sets ds(boost::make_assoc_property_map(rank), boost::make_assoc_property_map(parent));

		// insert all the vertices as single sets, and build id -> tile map
		for ( auto t : tiles) {
			ds.make_set( t->get_id() );
            id_to_tile[t->get_id()] = t;
		}

		// add each graph edge to the data structure
		auto broken_tiles = get_broken_tiles(tiles);
		for ( auto [u, v] : broken_tiles) {
			ds.link(u->get_id(), v->get_id());
		}

		// build a map mapping representatives to a elements...
		std::unordered_map<tess::obj_id, std::vector<tess::obj_id>> sets;
		for ( auto tile : tiles) {
			auto v = tile->get_id();
			auto parent = ds.find_set(v);
			sets[parent].push_back(v);
		}

		// return the values of the above, with the tile impelemntation wrapped in real tiles.
		std::vector<std::vector<tess::tile_root_ptr>> output(sets.size());
		std::transform(sets.begin(), sets.end(), output.begin(),
			[&id_to_tile](const std::unordered_map<tess::obj_id, std::vector<tess::obj_id>>::value_type& key_val) {
				const std::vector<tess::obj_id>& val = key_val.second;
                std::vector<tess::tile_root_ptr> tiles(val.size());

                std::transform(val.begin(), val.end(), tiles.begin(),
                    [&id_to_tile](tess::obj_id id)->tess::tile_root_ptr {
                        return id_to_tile.at(id);
                    }
                );

				return tiles;
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

	std::vector<tess::tile_root_ptr> join_broken_tiles(tess::gc_heap& a, const std::vector<tess::tile_root_ptr>& tiles) {
		if (!has_broken_tile(tiles))
			return tiles;

		auto grouped_tiles = get_broken_tile_groups(tiles);
		if (grouped_tiles.size() == tiles.size())
			return tiles;

		std::vector<tess::tile_root_ptr> output(grouped_tiles.size());
		std::transform(grouped_tiles.begin(), grouped_tiles.end(), output.begin(),
			[&a]( auto& tile_group)->tess::tile_root_ptr {
				if (tile_group.size() == 1) {
					return tess::clone_object(a, to_const(tile_group.front()) );
				} else {
					return tess::join(a, tile_group);
				}
			}
		);

		return output;
	}

	void propagate_edge_fields(tess::tile_root_ptr tile, tess::const_patch_root_ptr patch)
	{
		tess::edge_location_table edges;
		for (auto j = patch->begin_tiles(); j != patch->end_tiles(); ++j) {
			auto& tile = *j;
			for (auto i = tile->begin_edges(); i != tile->end_edges(); ++i) {
				edges.insert( to_const(to_root_ptr(*i)) );
			}
		}

		for (auto iter = tile->begin_edges(); iter != tile->end_edges(); ++iter) {
			auto edge = to_root_ptr(*iter);
			auto joined_edges = edges.get(to_const(edge));

			std::unordered_map<std::string, tess::field_value> fields;
			for (const auto e : joined_edges) {
				for (const auto& [var, val] : e->fields()) {
					if (tess::is_simple_value(val)) {
						if (fields.find(var) == fields.end()) {
							fields[var] = copy_field(val);
						} else {
							if (fields[var] != val)
								fields.erase(var);
						}
					}
				}
			}
			for (const auto& [var, val] : fields) {
				edge->insert_field( var, from_field_value(val) );
			}
		}
	}

	void propagate_fields(tess::tile_root_ptr tile, tess::const_patch_root_ptr patch)
	{
		std::unordered_map<std::string, tess::field_value> fields;
		for (auto iter = patch->begin_tiles(); iter != patch->end_tiles(); ++iter) {
			auto& t = *iter;
			for (const auto& [var, val] : t->fields()) {
				if (tess::is_simple_value(val)) {
					if (fields.find(var) == fields.end()) {
						fields[var] = copy_field(val);
					} else {
						if (fields[var] != val)
							fields.erase(var);
					}
				}
			}
		}
		for (const auto& [var, val] : fields) {
			tile->insert_field(var, from_field_value(val));
		}

		propagate_edge_fields(tile, patch);
	}
}

tess::detail::patch_impl::tile_iterator tess::detail::patch_impl::begin_tiles() {
	return tiles_.begin();
}

tess::detail::patch_impl::tile_iterator tess::detail::patch_impl::end_tiles() {
	return tiles_.end();
}

tess::detail::patch_impl::const_tile_iterator tess::detail::patch_impl::begin_tiles() const {
	return tiles_.begin();
}

tess::detail::patch_impl::const_tile_iterator tess::detail::patch_impl::end_tiles() const {
	return tiles_.end();
}

void tess::detail::patch_impl::build_edge_table() const
{
	edge_tbl_.clear();
	for (const auto& tile : tiles_) {
		for (auto iter = tile->begin_edges(); iter != tile->end_edges(); ++iter) {
			const auto& e = *iter;
			auto key = e->get_edge_location_indices();
			if (edge_tbl_.find(key) == edge_tbl_.end())
				edge_tbl_[key] = edge_graph_ptr(self_graph_ptr(), e);
			else
				throw tess::error("invalid tile patch");
		}
	}
}

void tess::detail::patch_impl::initialize( tess::gc_heap& a, const std::vector<tess::tile_root_ptr>& tiles) {
	for ( auto& t : tiles)
		insert_tile(t);
}

void tess::detail::patch_impl::insert_tile( tess::tile_root_ptr tile )
{
	tile->set_parent(to_root_ptr(self_graph_ptr()), static_cast<int>( tiles_.size()) );

	for (auto iter = tile->begin_vertices(); iter != tile->end_vertices(); ++iter) {
		auto& vert = *iter;
		int new_vert_index = vert_tbl_.insert(vert->pos());
		vert->set_location( new_vert_index );
	}

	tiles_.push_back(
		tile_graph_ptr(self_graph_ptr(), tile)
	);
}

int tess::detail::patch_impl::count() const
{
	return static_cast<int>(tiles_.size());
}


void tess::detail::patch_impl::insert_field(const std::string& var, const value_& val)
{
	fields_[var] = variant_cast(val);
}

tess::value_ tess::detail::patch_impl::get_field(gc_heap& allocator, const std::string& field) const
{
	auto iter = fields_.find(field);

	if (iter == fields_.end())
		throw tess::error(std::string("referenced undefined tile patch field: ") + field);

	return from_field_value(iter->second);
}

tess::value_ tess::detail::patch_impl::get_ary_item(int i) const
{
	return tess::make_value(to_root_ptr(tiles_.at(i)));
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

tess::patch_root_ptr tess::detail::patch_impl::flip(gc_heap& a) const {
	value_ self = to_const(to_root_ptr(self_graph_ptr()));
	auto clone = get_mutable<tess::const_patch_root_ptr>(tess::clone_value(a, self));
	clone->flip();
	return clone;
}

void  tess::detail::patch_impl::flip()  {
	apply(flip_matrix());
	for (auto& tile : tiles_) {
		for (auto iter = tile->begin_edges(); iter != tile->end_edges(); ++iter) {
			auto& e = *iter;
			e->flip();
		}
	}
	edge_tbl_.clear();
}

tess::const_edge_root_ptr tess::detail::patch_impl::get_edge_on(int u, int v) const
{
	if (edge_tbl_.empty())
		build_edge_table();

	auto iter = edge_tbl_.find(edge_indices{ u, v });
	if (iter != edge_tbl_.end())
		return to_const(to_root_ptr(iter->second));
	else
		return {};
}

tess::const_edge_root_ptr tess::detail::patch_impl::get_edge_on(tess::point u, tess::point v) const {
	auto u_index = vert_tbl_.get_index(u);
	auto v_index = vert_tbl_.get_index(v);
	return get_edge_on(u_index, v_index);
}

tess::value_ tess::detail::patch_impl::get_on(gc_heap& a, const std::variant<tess::const_edge_root_ptr, tess::const_cluster_root_ptr>& var) const
{
	return std::visit(
		overloaded{
			[&](tess::const_edge_root_ptr e) -> value_ {
				auto maybe_edge = get_edge_on( e->u()->pos(), e->v()->pos());
				if (maybe_edge) {
					return value_(maybe_edge);
				} else {
					return {};
				}
			},
			[&](const tess::const_cluster_root_ptr c) -> value_ {
				std::vector<tess::value_> on_edges(c->get_ary_count());
				std::transform(c->begin(), c->end(), on_edges.begin(),
					[&](const auto& v) -> value_ {
						std::variant<tess::const_edge_root_ptr, tess::const_cluster_root_ptr> edge_or_cluster = variant_cast(from_field_value(v));
						return this->get_on(a, edge_or_cluster);
					}
				);
				return value_( a.make_const<const_cluster_root_ptr>(on_edges) );
			}
		},
		var
	);
}

void tess::detail::patch_impl::clone_to(tess::gc_heap& a, std::unordered_map<obj_id, std::any>& orginal_to_clone, patch_raw_ptr mutable_clone) const
{
	for (const auto& t : tiles_) { // clone tiles
		mutable_clone->tiles_.push_back( 
			clone_object( self_graph_ptr(), a, orginal_to_clone, t )
		);
	}
	for (const auto& [var, val] : fields_) { // clone fields
		mutable_clone->fields_[var] = tess::clone_value(self_graph_ptr(), a, orginal_to_clone, val);
	}
	mutable_clone->vert_tbl_ = vert_tbl_;
}

tess::point tess::detail::patch_impl::get_vertex_location(int index) const {
	return vert_tbl_.get_location(index);
}

tess::tile_root_ptr tess::detail::patch_impl::join(tess::gc_heap& a) const
{
	auto self_ptr = to_const(to_root_ptr( self_graph_ptr() ));
	auto points = tess::join(self_ptr);
	auto joined_patch = a.make_mutable<tess::const_tile_root_ptr>(points);
	propagate_fields(joined_patch, self_ptr);
	return joined_patch;
}

void tess::detail::patch_impl::dfs(tile_visitor visit) const
{
	std::unordered_set<tess::obj_id> visited;

	std::function<void( const const_tile_root_ptr& tile)> dfs_aux;
	dfs_aux = [&](const const_tile_root_ptr& t) {
		if (visited.find(t->get_id()) != visited.end())
			return;
		visited.insert(t->get_id());
		visit(t);
		for (const auto& neighbor : get_neighbors(t)) {
			dfs_aux(neighbor);
		}
	};
	dfs_aux(to_const(to_root_ptr(tiles_[0])));
}

/*---------------------------------------------------------------------------------------------*/

void tess::detail::cluster_impl::initialize(gc_heap& a, const std::vector<value_>& values)
{
	const auto& self = self_graph_ptr();
    std::transform(
		values.begin(), values.end(), std::back_inserter(values_),
		[&self](const value_ v) {
			return to_field_value(self, v);
		}
	);
}


tess::value_ tess::detail::cluster_impl::get_field(gc_heap& allocator, const std::string& field) const
{
	return value_(); // TODO
}


void  tess::detail::cluster_impl::insert_field(const std::string& var, const value_& val)
{
}

tess::value_ tess::detail::cluster_impl::get_ary_item(int i) const
{
	if (i < 0 || i >= values_.size())
		throw tess::error("array ref out of bounds");
	return from_field_value(values_.at(i));
}

void tess::detail::cluster_impl::push_value(value_ val)
{
	values_.push_back(to_field_value(self_graph_ptr(), val));
}

int tess::detail::cluster_impl::get_ary_count() const
{
	return static_cast<int>(values_.size());
}

void tess::detail::cluster_impl::clone_to(tess::gc_heap& allocator, std::unordered_map<obj_id, std::any>& orginal_to_clone, cluster_raw_ptr mutable_clone) const
{
	for (const auto& value : values_) {
		mutable_clone->values_.push_back(
			tess::clone_value(self_graph_ptr(), allocator, orginal_to_clone, value)
		); // items
	}
}

std::vector<tess::field_value>::const_iterator tess::detail::cluster_impl::begin() const
{
	return values_.cbegin();
}

std::vector<tess::field_value>::const_iterator tess::detail::cluster_impl::end() const
{
	return values_.cend();
}

int count_tiles(const std::vector<tess::value_>& tiles_and_patches) {
	int count = 0;
	for (const auto& tile_or_patch : tiles_and_patches)
		std::visit(
			overloaded{
				[&count]( tess::const_tile_root_ptr) { ++count; },
				[&count]( tess::const_patch_root_ptr patch) { count += patch->count(); },
				[](auto) { throw tess::error("unknown error"); }
			},
			tile_or_patch
		);
	return count;
}

tess::patch_root_ptr tess::flatten(tess::gc_heap& a, const std::vector<tess::value_>& tiles_and_patches, bool should_join_broken_tiles) {
	for (const auto& v : tiles_and_patches)
		if (!std::holds_alternative<tess::const_tile_root_ptr>(v) && !std::holds_alternative<tess::const_patch_root_ptr>(v))
			throw tess::error("attempted to flatten a value that is not a tile or tile patch");
	int n = count_tiles(tiles_and_patches);
	std::vector<tess::tile_root_ptr> tiles;
	tiles.reserve(n);
	for (const auto& tile_or_patch : tiles_and_patches) {
		std::visit(
			overloaded{
				[&](tess::const_tile_root_ptr t) { tiles.push_back( tess::clone_object(a, t) ); },
				[&](tess::const_patch_root_ptr patch) {
					for (auto i = patch->begin_tiles(); i != patch->end_tiles(); ++i) {
						auto clone = (*i)->clone_detached(a);
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

	auto patch_impl = a.make_blank<tess::const_patch_root_ptr>();
	for (const auto& tile : tiles) {
		patch_impl->insert_tile( tile );
	}

	return patch_impl;
}

tess::tile_root_ptr tess::join(tess::gc_heap& a, const std::vector<tess::value_>& tiles_and_patches, bool should_join_broken_tiles) {
	auto patch = flatten(a, tiles_and_patches, should_join_broken_tiles);
	return patch->join(a);
}

tess::tile_root_ptr tess::join(tess::gc_heap& a, const std::vector<tile_root_ptr>& tiles) {
	std::vector<tess::value_> tiles_as_vals(tiles.size());
	std::transform(tiles.begin(), tiles.end(), tiles_as_vals.begin(),
		[](tile_root_ptr t) -> tess::value_ {
			return tess::make_value(t);
		}
	);
	return join(a, tiles_as_vals, false);
}
