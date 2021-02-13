#include "boost/functional/hash.hpp"
#include "geometry.h"
#include "number.h"
#include "tessera/error.h"
#include "tessera/tile_patch.h"
#include "tessera_impl.h"
#include "tile_impl.h"
#include <unordered_set>

namespace geom = tess::geometry;

namespace {

	std::tuple<double, double> to_doubles(tess::point p) {
		auto [x, y] = p;
		return { static_cast<double>(x) , static_cast<double>(y) };
	}

	geom::rtree_point make_rtree_point(tess::number x, tess::number y) {
		return geom::rtree_point(
			static_cast<double>(x),
			static_cast<double>(y)
		);
	}

	tess::point slope_vector(tess::point pt1, tess::point pt2)
	{
		if (pt1 >= pt2)
			std::swap(pt1, pt2);

		auto [x1, y1] = pt1;
		auto [x2, y2] = pt2;
		auto length = tess::distance(pt1, pt2);
		auto x_diff = (x2 - x1) / length;
		auto y_diff = (y2 - y1) / length;

		return { x_diff,y_diff };
	}

	geom::rtree_point make_rtree_point(tess::point p) {
		auto [x, y] = p;
		return make_rtree_point(x, y);
	}

	geom::segment edge_to_seg(tess::const_edge_root_ptr e) {
		auto [x1,y1] = e->u()->pos();
		auto [x2, y2] = e->v()->pos();
		return geom::segment{
			{static_cast<double>(x1),static_cast<double>(y1) },
			{static_cast<double>(x2),static_cast<double>(y2) }
		};
	}

	std::tuple<double, double> rotate(const std::tuple<double, double>& pt, double theta) {
		auto [x, y] = pt;
		auto length = std::sqrt(x * x + y * y);
		auto new_angle = std::atan2(y, x) + theta;
		return { length * std::cos(new_angle) , length * std::sin(new_angle) };
	}

	std::tuple<double, double> translate(const std::tuple<double, double>& pt, const std::tuple<double, double>& offset) {
		auto [x, y] = pt;
		auto [dx, dy] = offset;
		return { x + dx , y + dy };
	}

	std::tuple<double, double> center_of_line_segment(const std::tuple<double, double>& pt1, const std::tuple<double, double>& pt2) {
		auto [x1, y1] = pt1;
		auto [x2, y2] = pt2;
		return {
			(x1 + x2) / 2.0f,
			(y1 + y2) / 2.0f
		};
	}

	double distance(const std::tuple<double, double>& pt1, const std::tuple<double, double>& pt2)
	{
		auto [x1, y1] = pt1;
		auto [x2, y2] = pt2;
		auto x_diff = x2 - x1;
		auto y_diff = y2 - y1;
		return std::sqrt(x_diff * x_diff + y_diff * y_diff);
	}

	double slope_angle(const std::tuple<double, double>& pt1, const std::tuple<double, double>& pt2)
	{
		auto [x1, y1] = pt1;
		auto [x2, y2] = pt2;
		auto x_diff = x2 - x1;
		auto y_diff = y2 - y1;
		return std::atan2(y_diff, x_diff);
	}

	geom::polygon fat_line_segment(const std::tuple<double, double>& pt1, const std::tuple<double, double>& pt2, float padding) {
		auto half_length = distance(pt1, pt2) / 2.0f;

		using pt = std::tuple<double, double>;
		std::array<pt, 5> vertices = {
			pt{ half_length + padding, -padding},
			pt{ half_length + padding, padding},
			pt{ -half_length - padding, padding},
			pt{ -half_length - padding, -padding},
			pt{ half_length + padding, -padding},
		};

		auto theta = slope_angle(pt1, pt2);
		auto location = center_of_line_segment(pt1, pt2);
		geom::polygon poly;
		for (const auto& v : vertices) {
			auto [x, y] = translate(rotate(v, theta), location);
			geom::bg::append(poly, geom::bg::make<geom::point>(x, y));
		}

		return poly;
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

	geom::polygon vertices_to_polygon(const std::vector<tess::const_vertex_root_ptr>& vertices) {
		geom::polygon poly;
		for (const auto& vertex : vertices) {
			const auto [x, y] = vertex->pos();
			geom::bg::append(poly, geom::bg::make<geom::point>(static_cast<double>(x), static_cast<double>(y)));
		}
		auto [x_1, y_1] = vertices[0]->pos();
		geom::bg::append(poly, geom::bg::make<geom::point>(static_cast<double>(x_1), static_cast<double>(y_1)));
		return poly;
	}

	geom::polygon tile_to_polygon(tess::const_tile_root_ptr tile) {
		std::vector<tess::const_vertex_root_ptr> ordered_vertices;
		auto first = to_root_ptr(*(tile->begin_vertices()));
		auto v = first;
		do {
			ordered_vertices.push_back(v);
			v = v->out_edge()->v();
		} while (v != first);

		return vertices_to_polygon(ordered_vertices);
	}


	std::vector<tess::const_tile_root_ptr> topological_sort_tiles(tess::const_patch_root_ptr patch) {
		std::vector<tess::const_tile_root_ptr> tiles;
		patch->dfs(
			[&tiles](tess::const_tile_graph_ptr t) {
				tiles.push_back(to_root_ptr(t));
			}
		);
		return tiles;
	}

	std::vector<geom::polygon> tile_patch_to_polygons(std::vector<tess::const_tile_root_ptr> tiles) {
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

	std::vector<tess::point> shrink_wrap(const std::vector<tess::point>& points) {
		const int n = static_cast<int>(points.size());
		auto prev = [&points,n](int i) { return (i > 0) ? i - 1 : n-1; };
		auto next = [&points,n](int i) { return (i != n-1) ? i + 1 : 0; };

		int first_corner = -1;
		for (int i = 0; i < n; ++i) {
			if (!tess::are_collinear(points[prev(i)], points[i], points[next(i)])) {
				first_corner = i;
				break;
			}
		}
		if (first_corner == -1)
			return {};

		std::vector<tess::point> shrink_wrapped_points = { points[first_corner] };
		auto corner = first_corner;
		int i = next(corner);
		while (i != first_corner) {
			if (tess::are_collinear(points[prev(i)], points[i], points[next(i)])) {
				i = next(i);
				continue;
			}
			shrink_wrapped_points.push_back(points[i]);
			corner = i;
			i = next(i);
		}

		return shrink_wrapped_points;
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

std::vector<tess::point> tess::join(tess::const_patch_root_ptr tiles)
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

	return shrink_wrap(points);
}

bool tess::are_collinear(const tess::point& a, const tess::point& b, const tess::point& c, tess::number eps)
{
	auto [a_x, a_y] = a;
	auto [b_x, b_y] = b;
	auto [c_x, c_y] = c;

	auto area = tess::abs(a_x * (b_y - c_y) + b_x * (c_y - a_y) + c_x * (a_y - b_y));
	return area < eps;
}

bool tess::are_parallel(const tess::point& a1, const tess::point& a2, const tess::point& b1, const tess::point& b2, tess::number eps)
{
	auto slope1 = slope_vector(a1, a2);
	auto slope2 = slope_vector(b1, b2);
	return tess::distance(slope1, slope2) < eps;
}

tess::edge_location_table::edge_location_table(number eps) :
	eps_(eps)
{
}

void tess::edge_location_table::insert(tess::const_edge_root_ptr edge)
{
	geometry::segment_rtree_value pair{ edge_to_seg(edge), edge };
	impl_.insert(pair);
};

std::vector<tess::const_edge_root_ptr> tess::edge_location_table::get(tess::point a, tess::point b)
{
	std::vector<geom::segment_rtree_value> edges;
	impl_.query(geom::bgi::intersects(fat_line_segment(to_doubles(a), to_doubles(b), tess::eps)), std::back_inserter(edges));

	std::vector<tess::const_edge_root_ptr> output;
	for (auto [key, val] : edges) {
		if (tess::are_parallel(a, b, val->u()->pos(), val->v()->pos(), tess::eps))
			output.push_back(val);
	}

	return output;
}

std::vector<tess::const_edge_root_ptr>  tess::edge_location_table::get(tess::const_edge_root_ptr edge)
{
	return get(edge->u()->pos(), edge->v()->pos());
}