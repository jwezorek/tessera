#include "boost/functional/hash.hpp"
#include "geometry.h"
#include "tessera/error.h"
#include "tessera/tile_patch.h"
#include "tessera_impl.h"
#include "tile_impl.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <unordered_set>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using rtree_point = bg::model::point<double, 2, bg::cs::cartesian>;
using rtree_box = bg::model::box<rtree_point>;
using rtree_value = std::pair< rtree_point, int>;
using rtree = bgi::rtree<rtree_value, bgi::rstar<8>>; //bgi::rtree<rtree_value, bgi::quadratic<16>>;

using bg_point = bg::model::d2::point_xy<double>;
using bg_polygon = bg::model::polygon<bg_point, false>;

namespace {

	rtree_point make_rtree_point(tess::number x, tess::number y) {
		return rtree_point(
			static_cast<double>(x),
			static_cast<double>(y)
		);
	}

	rtree_box box_from_points(tess::number x1, tess::number y1, tess::number x2, tess::number y2) {
		if (x2 < x1)
			std::swap(x1, x2);
		if (y2 < y1)
			std::swap(y1, y2);
		return rtree_box(make_rtree_point(x1, y1), make_rtree_point(x2, y2));
	}

	rtree_box pad_point(const std::tuple<tess::number, tess::number>& pt, tess::number eps) {
		auto [x, y] = pt;
		tess::number padding = eps / tess::number(2);
		tess::number x1 = x - padding;
		tess::number y1 = y - padding;
		tess::number x2 = x + padding;
		tess::number y2 = y + padding;

		return box_from_points(x1, y1, x2, y2);
	}

	class point_to_index_tbl
	{
	public:
		virtual std::optional<int> get(const tess::point& pt) const = 0;
		virtual int insert(const tess::point& pt) = 0;
		virtual void clear() = 0;
	};

	class basic_tbl : public point_to_index_tbl
	{
	public:
		basic_tbl(tess::number eps) : eps_(eps) {}

		std::optional<int> get(const tess::point& pt) const override {
			auto [x, y] = pt;
			auto box = pad_point(pt, eps_);
			for (const auto& [tbl_pt, index] : impl_) {
				auto [tbl_pt_x, tbl_pt_y] = tbl_pt;
				if (bg::within(make_rtree_point(tbl_pt_x,tbl_pt_y), box))
					return index;
			}
			return std::nullopt;
		}
		int insert(const  tess::point& pt) override {
			auto maybe_index = get(pt);
			if (maybe_index.has_value())
				return maybe_index.value();
			int new_index = static_cast<int>(impl_.size());
			impl_.push_back(item{ pt,new_index });
			return new_index;
		}
		void clear() override {
			impl_.clear();
		}
	private:
		using item = std::tuple<tess::point, int>;
		tess::number eps_;
		std::vector<item> impl_;
	};

	class rtree_tbl : public point_to_index_tbl
	{
	public:
		rtree_tbl(tess::number eps) : eps_(eps) {}

		std::optional<int> get(const tess::point& pt) const override {
			std::vector<rtree_value> items;
			tree_.query(bgi::within(pad_point(pt, eps_)), std::back_inserter(items));
			if (items.empty())
				return std::nullopt;
			if (items.size() == 1)
				return items[0].second;
			throw tess::error("invalid vertex table");
		}

		int insert(const tess::point& pt) override {
			auto maybe_index = get(pt);
			if (maybe_index.has_value())
				return maybe_index.value();
			int new_index = static_cast<int>(tree_.size());
			auto [x, y] = pt;
			tree_.insert(
				rtree_value(make_rtree_point(x, y), new_index)
			);
			return new_index;
		}
		void clear() override {
			tree_.clear();
		}

	private:
		tess::number eps_;
		rtree tree_;
	};

	std::optional<bg_polygon> join_polygons(const std::vector<bg_polygon>& polygons) {

		if (polygons.empty())
			return std::nullopt;

		if (polygons.size() == 1)
			return polygons.front();

		bg_polygon joined_so_far = polygons.front();
		for (auto i = polygons.begin() + 1; i != polygons.end(); ++i) {
			const auto& polygon = *i;
			std::vector<bg_polygon> joined;
			bg::union_(joined_so_far, polygon, joined);
			if (joined.size() != 1)
				return std::nullopt;
			joined_so_far = joined.front();
		}

		return joined_so_far;
	}

	bg_polygon vertices_to_polygon(const std::vector<tess::vertex>& vertices) {
		bg_polygon poly;
		for (const auto& vertex : vertices) {
			const auto [x, y] = tess::get_impl(vertex)->pos();
			bg::append(poly, bg::make<bg_point>(x, y));
		}
		auto [x_1, y_1] = tess::get_impl(vertices[0])->pos();
		bg::append(poly, bg::make<bg_point>(x_1, y_1));
		return poly;
	}

	bg_polygon tile_to_polygon(const tess::tile::impl_type* tile) {
		std::vector<tess::vertex> ordered_vertices;
		auto first = tile->vertices()[0];
		auto v = first;
		do {
			ordered_vertices.push_back(v);
			v = v.out_edge().v();
		} while (tess::get_impl(v) != tess::get_impl(first));

		return vertices_to_polygon(ordered_vertices);
	}

	bg_polygon tile_to_polygon(const tess::tile& tile) {
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

	std::vector<bg_polygon> tile_patch_to_polygons(std::vector<const tess::tile::impl_type*> tiles) {
		std::vector<bg_polygon> polygons(tiles.size());
		std::transform(tiles.begin(), tiles.end(), polygons.begin(),
			[](const auto& tile)->bg_polygon {
				return tile_to_polygon(tile);
			}
		);
		return polygons;
	}

	bool is_closed_poly(const std::vector<tess::point>& poly) {
		return tess::equals(poly.front(), poly.back());
	}
}

class tess::vertex_location_table::impl_type {
public:
	rtree_tbl pt_to_index;
	std::vector<tess::point> index_to_pt;

	impl_type(tess::number eps) : pt_to_index(eps)
	{}
};

tess::vertex_location_table::vertex_location_table() :
	impl_(std::make_shared<impl_type>(std::numeric_limits<float>::epsilon()))
{}

int tess::vertex_location_table::get_index(const tess::point& pt) const
{
	auto maybe_index = impl_->pt_to_index.get(pt);
	if (!maybe_index.has_value())
		throw tess::error("Bad vertex table look up");
	return maybe_index.value();
}

tess::point tess::vertex_location_table::get_location(int index) const
{
	return impl_->index_to_pt.at(index);
}

int tess::vertex_location_table::insert(const tess::point& pt)
{
	int index = impl_->pt_to_index.insert(pt);
	int n = static_cast<int>(impl_->index_to_pt.size());
	if (index < n)
		return index; // TODO: possibly make the point in the table the average of pt and what was already there
	if (index > n)
		throw error("Corrupt vertex table.");
	impl_->index_to_pt.push_back(pt);
	return n;
}

void tess::vertex_location_table::apply_transformation(const matrix& mat)
{
	impl_->pt_to_index.clear();
	for (auto& pt : impl_->index_to_pt) {
		pt = tess::apply_matrix(mat, pt);
		impl_->pt_to_index.insert(pt);
	}
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
		[](bg_point pt)->tess::point {
			return { pt.x(),pt.y() };
		}
	);

	// if the last point and the first point are the same, pop the last point
	if (is_closed_poly(points))
		points.pop_back();

	return points;
}

