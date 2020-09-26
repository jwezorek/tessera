#include "boost/functional/hash.hpp"
#include "geometry.h"
#include "tessera/error.h"
#include "tessera/tile_patch.h"
#include "tessera_impl.h"
#include "tile_impl.h"
#include <unordered_set>

namespace geom = tess::geometry;

namespace {

	geom::rtree_point make_rtree_point(tess::number x, tess::number y) {
		return geom::rtree_point(
			static_cast<double>(x),
			static_cast<double>(y)
		);
	}

	geom::rtree_box box_from_points(tess::number x1, tess::number y1, tess::number x2, tess::number y2) {
		if (x2 < x1)
			std::swap(x1, x2);
		if (y2 < y1)
			std::swap(y1, y2);
		return geom::rtree_box(make_rtree_point(x1, y1), make_rtree_point(x2, y2));
	}

	geom::rtree_box pad_point(const std::tuple<tess::number, tess::number>& pt, tess::number eps) {
		auto [x, y] = pt;
		tess::number padding = eps / tess::number(2);
		tess::number x1 = x - padding;
		tess::number y1 = y - padding;
		tess::number x2 = x + padding;
		tess::number y2 = y + padding;

		return box_from_points(x1, y1, x2, y2);
	}

	std::optional<geom::polygon> join_polygons(const std::vector<geom::polygon>& polygons) {

		if (polygons.empty())
			return std::nullopt;

		if (polygons.size() == 1)
			return polygons.front();

		geom::polygon joined_so_far = polygons.front();
		for (auto i = polygons.begin() + 1; i != polygons.end(); ++i) {
			const auto& polygon = *i;
			std::vector<geom::polygon> joined;
			geom::bg::union_(joined_so_far, polygon, joined);
			if (joined.size() != 1)
				return std::nullopt;
			joined_so_far = joined.front();
		}

		return joined_so_far;
	}

	geom::polygon vertices_to_polygon(const std::vector<tess::vertex>& vertices) {
		geom::polygon poly;
		for (const auto& vertex : vertices) {
			const auto [x, y] = tess::get_impl(vertex)->pos();
			geom::bg::append(poly, geom::bg::make<geom::point>(x, y));
		}
		auto [x_1, y_1] = tess::get_impl(vertices[0])->pos();
		geom::bg::append(poly, geom::bg::make<geom::point>(x_1, y_1));
		return poly;
	}

	geom::polygon tile_to_polygon(const tess::tile::impl_type* tile) {
		std::vector<tess::vertex> ordered_vertices;
		auto first = tile->vertices()[0];
		auto v = first;
		do {
			ordered_vertices.push_back(v);
			v = v.out_edge().v();
		} while (tess::get_impl(v) != tess::get_impl(first));

		return vertices_to_polygon(ordered_vertices);
	}

	geom::polygon tile_to_polygon(const tess::tile& tile) {
		return tile_to_polygon(get_impl(tile));
	}

	std::vector<const tess::tile::impl_type*> topological_sort_tiles(const tess::tile_patch::impl_type* patch) {
		std::vector<const tess::tile::impl_type*> tiles;
		patch->dfs(
			[&tiles](const tess::tile& t) {
				tiles.push_back(get_impl(t));
			}
		);
		return tiles;
	}

	std::vector<geom::polygon> tile_patch_to_polygons(std::vector<const tess::tile::impl_type*> tiles) {
		std::vector<geom::polygon> polygons(tiles.size());
		std::transform(tiles.begin(), tiles.end(), polygons.begin(),
			[](const auto& tile)->geom::polygon {
				return tile_to_polygon(tile);
			}
		);
		return polygons;
	}

	bool is_closed_poly(const std::vector<tess::point>& poly) {
		return tess::equals(poly.front(), poly.back());
	}
}

geom::rtree_tbl::rtree_tbl(tess::number eps) : eps_(eps) {}

std::optional<int> geom::rtree_tbl::get(const tess::point& pt) const {
	std::vector<geom::rtree_value> items;
	tree_.query(geom::bgi::within(pad_point(pt, eps_)), std::back_inserter(items));
	if (items.empty())
		return std::nullopt;
	if (items.size() == 1)
		return items[0].second;
	throw tess::error("invalid vertex table");
}

int geom::rtree_tbl::insert(const tess::point& pt) {
	auto maybe_index = get(pt);
	if (maybe_index.has_value())
		return maybe_index.value();
	int new_index = static_cast<int>(tree_.size());
	auto [x, y] = pt;
	tree_.insert(
		geom::rtree_value(make_rtree_point(x, y), new_index)
	);
	return new_index;
}

void geom::rtree_tbl::clear() {
	tree_.clear();
}



tess::vertex_location_table::vertex_location_table() :
	pt_to_index_(std::numeric_limits<float>::epsilon())
{}

int tess::vertex_location_table::get_index(const tess::point& pt) const
{
	auto maybe_index = pt_to_index_.get(pt);
	if (!maybe_index.has_value())
		throw tess::error("Bad vertex table look up");
	return maybe_index.value();
}

tess::point tess::vertex_location_table::get_location(int index) const
{
	return index_to_pt_.at(index);
}

int tess::vertex_location_table::insert(const tess::point& pt)
{
	int index = pt_to_index_.insert(pt);
	int n = static_cast<int>(index_to_pt_.size());
	if (index < n)
		return index; // TODO: possibly make the point in the table the average of pt and what was already there
	if (index > n)
		throw error("Corrupt vertex table.");
	index_to_pt_.push_back(pt);
	return n;
}

void tess::vertex_location_table::apply_transformation(const matrix& mat)
{
	std::vector<tess::point> new_index_to_pt_(index_to_pt_.size());
	std::transform(index_to_pt_.begin(), index_to_pt_.end(), new_index_to_pt_.begin(),
		[&mat](const auto& p) { return tess::apply_matrix(mat, p);  }
	);
	index_to_pt_ = new_index_to_pt_;

	pt_to_index_.clear();
	for (const auto& p : index_to_pt_)
		pt_to_index_.insert(p);
}

std::size_t tess::edge_hash::operator()(const edge_indices& key) const
{
    return boost::hash_value(key);
}

std::vector<tess::point> tess::join(const tess::tile_patch::impl_type* tiles)
{
	auto ordered_tiles = topological_sort_tiles(tiles);
	auto maybe_polygon = join_polygons(tile_patch_to_polygons(ordered_tiles));

	if (!maybe_polygon.has_value())
		return {};

	const auto& polygon = maybe_polygon.value().outer();
	std::vector<tess::point> points(polygon.size());
	std::transform(polygon.begin(), polygon.end(), points.begin(),
		[](geom::point pt)->tess::point {
			return { pt.x(),pt.y() };
		}
	);

	// if the last point and the first point are the same, pop the last point
	if (is_closed_poly(points))
		points.pop_back();

	return points;
}

