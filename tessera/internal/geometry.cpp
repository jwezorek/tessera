#include "boost/functional/hash.hpp"
#include "geometry.h"
#include "tessera/error.h"
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <unordered_set>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

namespace {

	using bg_point = bg::model::point<double, 2, bg::cs::cartesian>;
	using bg_box = bg::model::box<bg_point>;
	using rtree_value = std::pair< bg_point, int>;
	using point_rtree = bgi::rtree<rtree_value, bgi::rstar<8>>; //bgi::rtree<rtree_value, bgi::quadratic<16>>;

	bg_point make_bg_point(tess::number x, tess::number y) {
		return bg_point(
			static_cast<double>(x),
			static_cast<double>(y)
		);
	}

	bg_box box_from_points(tess::number x1, tess::number y1, tess::number x2, tess::number y2) {
		if (x2 < x1)
			std::swap(x1, x2);
		if (y2 < y1)
			std::swap(y1, y2);
		return bg_box(make_bg_point(x1, y1), bg_point((double)x2, (double)y2));
	}

	bg_box pad_point(const std::tuple<tess::number, tess::number>& pt, tess::number eps) {
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
				if (bg::within(make_bg_point(tbl_pt_x,tbl_pt_y), box))
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
				rtree_value(make_bg_point(x, y), new_index)
			);
			return new_index;
		}
		void clear() override {
			tree_.clear();
		}

	private:
		tess::number eps_;
		point_rtree tree_;
	};

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
